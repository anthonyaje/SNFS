#include <grpc++/grpc++.h>
#include "nfsfuse.grpc.pb.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using grpc::ClientReader;

using namespace nfsfuse;

using namespace std;

struct sdata{
    int a;
    char b[10];
};


class NfsClient {
 public:
  NfsClient(std::shared_ptr<Channel> channel)
      : stub_(NFS::NewStub(channel)) {}

    int rpc_getattr(string path, struct stat* output){
        Stat result;
        ClientContext context;
        String p;
        p.set_str(path);

        Status status = stub_->nfsfuse_getattr(&context, p, &result);
        memset(output, 0, sizeof(struct stat));
        output->st_ino = result.ino();
        output->st_mode = result.mode();
        output->st_nlink = result.nlink();
        output->st_uid = result.uid();
        output->st_gid = result.gid();
        //output->st_rdev = result.rdev();
        output->st_size = result.size();
        output->st_blksize = result.blksize();
        output->st_blocks = result.blocks();
        output->st_atime = result.atime();
        output->st_mtime = result.mtime();
        output->st_ctime = result.ctime();
        
        if (result.err() != 0) {
                std::cout << "error " << result.err() << std::endl;
        }
        
        return -result.err();
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

        return result.err();
    }

    int rpc_open(const char* path, struct fuse_file_info* fi){
        ClientContext ctx;
        FuseFileInfo fi_res, fi_req;
        Status status; 

        fi_req.set_path(path);
        fi_req.set_flags(fi->flags);
        
        status = stub_->nfsfuse_open(&ctx, fi_req, &fi_res);
        if(status.ok())
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
        if(status.ok()){
            if(rres.bytesread() < 0)
                return -errno;

            strcpy(buf, rres.buffer().c_str());
            return rres.bytesread();
        }else{
            return -errno;
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
        
        WriteResult wres;

        Status status = stub_->nfsfuse_write(&ctx, wreq, &wres);
        if(status.ok()){
            if(wres.err() < 0)
                return wres.err(); 

            return wres.nbytes();

        } else{
            return -errno; 
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
        if(status.ok())
            fi->fh = cres.fh();   
 
        return -cres.err();
    }


	
 private:
    std::unique_ptr<NFS::Stub> stub_;
};











