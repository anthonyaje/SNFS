#include <grpc++/grpc++.h>
#include "nfsfuse.grpc.pb.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <vector>
#include <sys/time.h>


using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using grpc::ClientReader;

using namespace nfsfuse;

using namespace std;

struct timeval tv, tv2;
unsigned long t1, t2;
float elapse_time;


struct sdata{
    int a;
    char b[10];
};

vector<WriteRequest> PendingWrites;

class NfsClient;

struct Options {
    NfsClient* nfsclient;
    int show_help;
} options;

class NfsClient {
 public:
  NfsClient(std::shared_ptr<Channel> channel)
      : stub_(NFS::NewStub(channel)) {}

    int rpc_getattr(string path, struct stat* output){
        Stat result;
        ClientContext context;
        String p;
        p.set_str(path);
        memset(output, 0, sizeof(struct stat));

        Status status = stub_->nfsfuse_getattr(&context, p, &result);

        while (!status.ok()) {

            options.nfsclient = new NfsClient(grpc::CreateChannel("0.0.0.0:50051",
                                              grpc::InsecureChannelCredentials()));
            ClientContext ctx2;
            status = stub_->nfsfuse_getattr(&ctx2, p, &result);

        }

        if(result.err() != 0){
            std::cout << "errno: " << result.err() << std::endl;
            return -result.err();
        }

        output->st_ino = result.ino();
        output->st_mode = result.mode();
        output->st_nlink = result.nlink();
        output->st_uid = result.uid();
        output->st_gid = result.gid();
        output->st_size = result.size();
        output->st_blksize = result.blksize();
        output->st_blocks = result.blocks();
        output->st_atime = result.atime();
        output->st_mtime = result.mtime();
        output->st_ctime = result.ctime();
        
        return 0;
    }
  
    int rpc_readdir(string p, void *buf, fuse_fill_dir_t filler){
        String path;
        path.set_str(p);
        Dirent result;
        dirent de;
        Status status;
        ClientContext ctx;
        
        std::unique_ptr<ClientReader<Dirent> >reader(
            stub_->nfsfuse_readdir(&ctx, path));



        while(reader->Read(&result)){
            struct stat st;
            memset(&st, 0, sizeof(st));

            de.d_ino = result.dino();
            strcpy(de.d_name, result.dname().c_str());
            de.d_type = result.dtype();

            st.st_ino = de.d_ino;
            st.st_mode = de.d_type << 12;

            if (filler(buf, de.d_name, &st, 0, static_cast<fuse_fill_dir_flags>(0)))
                break;
            }

        status = reader->Finish();	

        return -result.err(); 
    }

    int rpc_open(const char* path, struct fuse_file_info* fi){
        ClientContext ctx;
        FuseFileInfo fi_res, fi_req;
        Status status; 

        fi_req.set_path(path);
        fi_req.set_flags(fi->flags);
        
        status = stub_->nfsfuse_open(&ctx, fi_req, &fi_res);

        while (!status.ok()) {

            options.nfsclient = new NfsClient(grpc::CreateChannel("0.0.0.0:50051",
                                              grpc::InsecureChannelCredentials()));
            ClientContext ctx2;
            status = stub_->nfsfuse_open(&ctx2, fi_req, &fi_res);

        }

        if(fi_res.err() == 0)
            fi->fh = fi_res.fh();

        return -fi_res.err();
    }
	
    int rpc_read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi){
        ClientContext clientContext;
        ReadRequest rr;
        rr.set_path(path);
        rr.set_size(size);
        rr.set_offset(offset);

        ReadResult rres;

        Status status = stub_->nfsfuse_read(&clientContext, rr, &rres);

        while (!status.ok()) {

            options.nfsclient = new NfsClient(grpc::CreateChannel("0.0.0.0:50051",
                                              grpc::InsecureChannelCredentials()));
            ClientContext ctx2;
            status = stub_->nfsfuse_read(&ctx2, rr, &rres);

        }



        if(rres.err() == 0){
            strcpy(buf, rres.buffer().c_str());
            return rres.bytesread();
        }else{
            return -rres.err();
        }        
    }

    int rpc_write(const char *path, const char *buf, size_t size,
             off_t offset, struct fuse_file_info *fi){
        ClientContext ctx;
        WriteRequest wreq;
        wreq.set_path(path);
        wreq.set_size(size);
        wreq.set_offset(offset);
        wreq.set_buffer(buf);
        
        PendingWrites.push_back(wreq);

        WriteResult wres;



        Status status = stub_->nfsfuse_write(&ctx, wreq, &wres);

        while (!status.ok()) {

            options.nfsclient = new NfsClient(grpc::CreateChannel("0.0.0.0:50051",
                                              grpc::InsecureChannelCredentials()));

			ClientContext ctx2;
            status = stub_->nfsfuse_write(&ctx2, wreq, &wres);


        }



        if(wres.err() == 0){
            return wres.nbytes();
        } else{
            return -wres.err(); 
        }
    }
    
    int rpc_create(const char *path, mode_t mode, struct fuse_file_info *fi)
    {
        ClientContext ctx; 
        CreateResult cres;
        CreateRequest creq;
        creq.set_path(path);
        creq.set_mode(mode);
        creq.set_flags(fi->flags);
        
        Status status = stub_->nfsfuse_create(&ctx, creq, &cres);

        while (!status.ok()) {

            options.nfsclient = new NfsClient(grpc::CreateChannel("0.0.0.0:50051",
                                              grpc::InsecureChannelCredentials()));
            ClientContext ctx2;
            status = stub_->nfsfuse_create(&ctx2, creq, &cres);

        }





        if(cres.err() == 0)
            fi->fh = cres.fh();   
 
        return -cres.err();
    }

  int rpc_mkdir(string path, mode_t mode){
      MkdirRequest input;
      ClientContext context;
      input.set_s(path);
      input.set_mode(mode);
      OutputInfo result;

      Status status = stub_->nfsfuse_mkdir(&context, input, &result);

      while (!status.ok()) {

          options.nfsclient = new NfsClient(grpc::CreateChannel("0.0.0.0:50051",
                                              grpc::InsecureChannelCredentials()));
          ClientContext ctx2;
          status = stub_->nfsfuse_mkdir(&context, input, &result);

      }


    
      if (result.err() != 0) {
          std::cout << "error: nfsfuse_mkdir() fails" << std::endl;
      }

      return -result.err();
  }

  int rpc_rmdir(string path) {
      String input;
      ClientContext context;
      input.set_str(path);
      OutputInfo result;

      Status status = stub_->nfsfuse_rmdir(&context, input, &result);

      while (!status.ok()) {

          options.nfsclient = new NfsClient(grpc::CreateChannel("0.0.0.0:50051",
                                              grpc::InsecureChannelCredentials()));
          ClientContext ctx2;
          status = stub_->nfsfuse_rmdir(&ctx2, input, &result);

      }



      if (result.err() != 0) {
          std::cout << "error: nfsfuse_rmdir() fails" << std::endl;
      }
      return -result.err();
  }


	
  int rpc_unlink(string path) {
      String input;
      ClientContext context;
      input.set_str(path);
      OutputInfo result;

      Status status = stub_->nfsfuse_unlink(&context, input, &result);

      while (!status.ok()) {

          options.nfsclient = new NfsClient(grpc::CreateChannel("0.0.0.0:50051",
                                              grpc::InsecureChannelCredentials()));
          ClientContext ctx2;
          status = stub_->nfsfuse_unlink(&ctx2, input, &result);

      }



      if (result.err() != 0) {
          std::cout << "error: nfsfuse_unlink() fails" << std::endl;
      }
      return -result.err();
  }

  int rpc_rename(const char *from, const char *to, unsigned int flags) {
      RenameRequest input;
      ClientContext context;
      input.set_fp(from);
      input.set_tp(to);
      input.set_flag(flags);
      OutputInfo result;

      Status status = stub_->nfsfuse_rename(&context, input, &result);

      while (!status.ok()) {

          options.nfsclient = new NfsClient(grpc::CreateChannel("0.0.0.0:50051",
                                              grpc::InsecureChannelCredentials()));
          ClientContext ctx2;
          status = stub_->nfsfuse_rename(&ctx2, input, &result);

      }



      if (result.err() != 0) {
          std::cout << "error: nfsfuse_rename() fails" << std::endl;
      }
      return -result.err();
  }

  int rpc_utimens(const char *path, const struct timespec *ts, struct fuse_file_info *fi) {
      UtimensRequest input;
      ClientContext context;
      input.set_sec(ts[0].tv_sec);
      input.set_nsec(ts[0].tv_nsec);
      input.set_sec2(ts[1].tv_sec);
      input.set_nsec2(ts[1].tv_nsec);

      input.set_path(path);


      OutputInfo result;
      Status status = stub_->nfsfuse_utimens(&context, input, &result);

      while (!status.ok()) {

          options.nfsclient = new NfsClient(grpc::CreateChannel("0.0.0.0:50051",
                                              grpc::InsecureChannelCredentials()));
          ClientContext ctx2;
          status = stub_->nfsfuse_utimens(&ctx2, input, &result);

      }



      if (result.err() != 0) {
          std::cout << "error: nfsfuse_utimens fails" << std::endl;
      }
      return -result.err();
  }


  int rpc_mknod(const char *path, mode_t mode, dev_t rdev) {
      MknodRequest input;
      ClientContext context;
      input.set_path(path);
      input.set_mode(mode);
      input.set_rdev(rdev);
      OutputInfo result;

      Status status = stub_->nfsfuse_mknod(&context, input, &result);

      while (!status.ok()) {

            options.nfsclient = new NfsClient(grpc::CreateChannel("0.0.0.0:50051",
                                              grpc::InsecureChannelCredentials()));
            ClientContext ctx2;
            status = stub_->nfsfuse_mknod(&ctx2, input, &result);

      }



      if (result.err() != 0) {
          std::cout << "error: nfsfuse_mknod() fails" << std::endl;
      }
      return -result.err();

  }

    int retransmission(int end_offset){

        if(PendingWrites.size() == 0){
            printf("vector is empty\n");
            return -1;
        }

        vector<WriteRequest>::const_iterator it = PendingWrites.begin();
        while(it != PendingWrites.end()){

            WriteResult wres;
            ClientContext ctx;
            if(it->offset() == end_offset)
                break; //don't need to send. This is the first entry in server. 
            Status status = stub_->nfsfuse_retranswrite(&ctx, *it, &wres);
            it++;
        }
        return 0;
    }

    int retransmissionall(){
        if(PendingWrites.size() == 0){
            printf("vector is empty\n");
            return -1;
        }

        vector<WriteRequest>::const_iterator it = PendingWrites.begin();
        while(it != PendingWrites.end()){
            WriteResult wres;
            ClientContext ctx; 
            Status status = stub_->nfsfuse_retranswrite(&ctx, *it, &wres);
            it++;
        }
        return 0;
    }


    int rpc_commit(int fh, int first_off, int last_off)
    {

	    gettimeofday(&tv, NULL);
	    t1 = 1000000*tv.tv_sec + tv.tv_usec;	

        ClientContext ctx;
        CommitRequest commitReq;
        CommitResult commitRes;
        

        commitReq.set_fh(fh);
        commitReq.set_firstoff(PendingWrites.front().offset());
        commitReq.set_endoff(PendingWrites.back().offset());

        Status status = stub_->nfsfuse_commit(&ctx, commitReq, &commitRes);
        while (!status.ok()) {

            options.nfsclient = new NfsClient(grpc::CreateChannel("0.0.0.0:50051",
                                              grpc::InsecureChannelCredentials()));
            ClientContext ctx2;
            status = stub_->nfsfuse_commit(&ctx2, commitReq, &commitRes);

        }

        if(commitRes.err() != 0){

            int server_off = commitRes.serveroff();
            int res;

            if(commitRes.err() == 2)
                res = this->retransmissionall();
            else
                res = this->retransmission(server_off);

	        gettimeofday(&tv, NULL);
	        t2 = 1000000*tv.tv_sec + tv.tv_usec;	
            elapse_time = (float) (t2-t1) / (1000);
	    cout << "Commit and Retransmittion time: " << elapse_time << " ms" << endl;    
            if(res != 0){
                cout<<"erro: rpc_commit() retransmission fails"<<endl;
                perror(strerror(errno));
                return -1;
            }
            
            //res = this->rpc_commit(fh, first_off, server_off);
            res = this->rpc_commit(fh, first_off, last_off);
            if(res != 0){
                cout<<"erro: this.rpc_commit() retransmission fails"<<endl;
                perror(strerror(errno));
                return -1;
            }
    
            return 0;

        }else{
            //assumming that if the code reach this point then commit is already acked      
            int bound = PendingWrites.size();
            for(int i=0; i<bound; i++){
                PendingWrites.pop_back();
            }
            return 0;
        }
        
      }


    int rpc_kill(){
        String input;
        ClientContext context;
        input.set_str("a");
        Errno result;

        Status status = stub_->nfsfuse_kill(&context, input, &result);
	    if (!status.ok())
            return 0;
    }

 private:
    std::unique_ptr<NFS::Stub> stub_;
};











