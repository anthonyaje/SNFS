#include <iostream>
#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <vector>
#include <grpc++/grpc++.h>
#include <time.h>
#include <stdlib.h>

#include "nfsfuse.grpc.pb.h"

#define READ_MAX    10000000 

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerWriter;
using grpc::Status;

using namespace nfsfuse;


using namespace std;

struct sdata{
	int a;
	char b[10]={};
};

vector<WriteRequest> PendingWrites;
vector<WriteRequest> RetransWrites;
bool flag_read = false;

void translatePath(const char* client_path, char* server_path){
    strcat(server_path, "./server");
    strcat(server_path+8, client_path);
    server_path[strlen(server_path)] = '\0';
}

class NfsServiceImpl final : public NFS::Service {
	Status nfsfuse_getattr(ServerContext* context, const String* s, 
					 Stat* reply) override {
		cout<<"[DEBUG] : lstat: "<<s->str().c_str()<<endl;

		struct stat st;
		char server_path[512]={0};
		translatePath(s->str().c_str(), server_path);
		int res = lstat(server_path, &st);
        if(res == -1){
		    perror(strerror(errno));
            cout<<"errno: "<<errno<<endl;
		    reply->set_err(errno);
		}
		else{
            reply->set_ino(st.st_ino);
            reply->set_mode(st.st_mode);
            reply->set_nlink(st.st_nlink);
            reply->set_uid(st.st_uid);
            reply->set_gid(st.st_gid);
            reply->set_size(st.st_size);
            reply->set_blksize(st.st_blksize);
            reply->set_blocks(st.st_blocks);
            reply->set_atime(st.st_atime);
            reply->set_mtime(st.st_mtime);
            reply->set_ctime(st.st_ctime);
			
		    reply->set_err(0);
		}
		
        return Status::OK;
	
	}
	
	Status nfsfuse_readdir(ServerContext* context, const String* s,
						ServerWriter<Dirent>* writer) override {
		cout<<"[DEBUG] : readdir: "<<s->str().c_str()<<endl;

		DIR *dp;
		struct dirent *de;
		Dirent directory;
		char server_path[512]={0};
		translatePath(s->str().c_str(), server_path);

		dp = opendir(server_path);
		if (dp == NULL){
			cout<<"[DEBUG] : readdir: "<<"dp == NULL"<<endl;
			perror(strerror(errno));
			directory.set_err(errno);
            return Status::OK;
		}
			
		while((de = readdir(dp)) != NULL){
		    directory.set_dino(de->d_ino);
		    directory.set_dname(de->d_name);
		    directory.set_dtype(de->d_type);
		    writer->Write(directory);
		}
		directory.set_err(0);

		closedir(dp);

		return Status::OK;
	}

    Status nfsfuse_open(ServerContext* context, const FuseFileInfo* fi_req,
            FuseFileInfo* fi_reply) override {
        
        char server_path[512] = {0};
        
        translatePath(fi_req->path().c_str(), server_path);
		cout<<"[DEBUG] : nfsfuse_open: path "<<server_path<<endl;
		cout<<"[DEBUG] : nfsfuse_open: flag "<<fi_req->flags()<<endl;

        int fh = open(server_path, fi_req->flags());

		cout<<"[DEBUG] : nfsfuse_open: fh"<<fh<<endl;
        if(fh == -1){
            fi_reply->set_err(errno);            
        }
        else{
            fi_reply->set_fh(fh);            
            fi_reply->set_err(0);
            close(fh);
        }
        
        return Status::OK;
    }

    Status nfsfuse_read(ServerContext* context, const ReadRequest* rr, 
            ReadResult* reply) override {
        char path[512];
        char *buf = new char[rr->size()];
        translatePath(rr->path().c_str(), path);


        int fd = open(path, O_RDONLY);

        if (fd == -1){
            reply->set_err(errno);
		    perror(strerror(errno));
            return Status::OK;
        }

        int res = pread(fd, buf, rr->size(), rr->offset());
        if (res == -1){
            reply->set_err(errno);
		    perror(strerror(errno));
            return Status::OK;
        }

        flag_read = true;
        reply->set_bytesread(res);
        reply->set_buffer(buf);
        reply->set_err(0);
        
        if(fd>0)
            close(fd);
        free(buf);
        
        return Status::OK;
    }

    Status nfsfuse_write(ServerContext* context, const WriteRequest* wr, 
            WriteResult* reply) override {
        
		cout<<"[DEBUG] : nfsfuse_write:"<<endl;
        WriteRequest WR(*wr);
        PendingWrites.push_back(WR);

        reply->set_nbytes(wr->size());
        reply->set_err(0);

        return Status::OK;
    }


    Status nfsfuse_retranswrite(ServerContext* context, const WriteRequest* wr,
            WriteResult* reply) override {

        cout<<"[DEBUG] : nfsfuse_retranswrite:"<<endl;
        WriteRequest WR(*wr);
        RetransWrites.push_back(WR);
        reply->set_nbytes(wr->size());
        reply->set_err(0);

        return Status::OK;
    }



    Status nfsfuse_create(ServerContext* context, const CreateRequest* req,
            CreateResult* reply) override {

        char server_path[512] = {0};
        translatePath(req->path().c_str(), server_path);

        cout<<"[DEBUG] : nfsfuse_create: path "<<server_path<<endl;
        cout<<"[DEBUG] : nfsfuse_create: flag "<<req->flags()<<endl;

        int fh = open(server_path, req->flags(), req->mode());

        cout<<"[DEBUG] : nfsfuse_create: fh"<<fh<<endl;
        if(fh == -1){
            reply->set_err(errno);
            return Status::OK;
        }
        else{
            reply->set_fh(fh);
            reply->set_err(0);
            close(fh);
            return Status::OK;
        }
    }


    Status nfsfuse_mkdir(ServerContext* context, const MkdirRequest* input,
                                         OutputInfo* reply) override {
            cout<<"[DEBUG] : mkdir: " << endl;
		
            char server_path[512]={0};
            translatePath(input->s().c_str(), server_path);

            int res = mkdir(server_path, input->mode());
		

            if (res == -1) {
                perror(strerror(errno)); 
                reply->set_err(errno);
                return Status::OK;
            } else {
	            reply->set_err(0);
            }

            return Status::OK;
    }

    Status nfsfuse_rmdir(ServerContext* context, const String* input,
                                         OutputInfo* reply) override {
            cout<<"[DEBUG] : rmdir: " << endl;

            char server_path[512]={0};
            translatePath(input->str().c_str(), server_path);

            int res = rmdir(server_path);


            if (res == -1) {
                perror(strerror(errno));
                reply->set_err(errno);
                return Status::OK;
            } else {
                reply->set_err(0);
            }
            
            return Status::OK;
    }


    Status nfsfuse_unlink(ServerContext* context, const String* input,
                                         OutputInfo* reply) override {
            cout<<"[DEBUG] : unlink " << endl;

            char server_path[512]={0};
            translatePath(input->str().c_str(), server_path);

            int res = unlink(server_path);
            if (res == -1) {
                perror(strerror(errno));
                reply->set_err(errno);
                return Status::OK;
            } else {
                reply->set_err(0);
            }
            return Status::OK;
    }

    Status nfsfuse_rename(ServerContext* context, const RenameRequest* input,
                                         OutputInfo* reply) override {
            cout<<"[DEBUG] : rename " << endl;
	    
	    if (input->flag()) {
            perror(strerror(errno));
            reply->set_err(EINVAL);
            reply->set_str("rename fail");	        
	        return Status::OK;
	    }

        char from_path[512]={0};
        char to_path[512] = {0};
        translatePath(input->fp().c_str(), from_path);
 	    translatePath(input->tp().c_str(), to_path);
	    int res = rename(from_path, to_path);
        if (res == -1) {
            perror(strerror(errno));
            reply->set_err(errno);
            return Status::OK;
        } else {
            reply->set_err(0);
        }
        
        return Status::OK;
    }

    Status nfsfuse_utimens(ServerContext* context, const UtimensRequest* input,
                                         OutputInfo* reply) override {
        cout<<"[DEBUG] : utimens " << endl;

	char server_path[512]={0};
        translatePath(input->path().c_str(), server_path);



        struct timespec ts[2];
	long oo;
	int ii;
	

	ts[0].tv_sec = input->sec();
	ts[0].tv_nsec = input->nsec();

	ts[1].tv_sec = input->sec2();
	ts[1].tv_nsec = input->nsec2();


        int res = utimensat(AT_FDCWD, server_path, ts, AT_SYMLINK_NOFOLLOW);
     

	if (res == -1) {
            perror(strerror(errno));
            reply->set_err(errno);
            return Status::OK;
        } 
        reply->set_err(0);
        return Status::OK;
    }

    Status nfsfuse_mknod(ServerContext* context, const MknodRequest* input,
                                         OutputInfo* reply) override {
        cout<<"[DEBUG] : mknod " << endl;

        char server_path[512]={0};
        translatePath(input->path().c_str(), server_path);
        cout << "server path: " << server_path << endl;

	    mode_t mode = input->mode();
	    dev_t rdev = input->rdev();

	    int res;

	    if (S_ISFIFO(mode))
		    res = mkfifo(server_path, mode);
	    else
		    res = mknod(server_path, mode, rdev);
	    
        if (res == -1) {
	        reply->set_err(errno);
            return Status::OK;
	    }

        reply->set_err(0);
        return Status::OK;
    }

    Status nfsfuse_commit(ServerContext* context, const CommitRequest* input,
                        CommitResult* reply) override {
        cout<<"[DEBUG] server: nfsfuse_commit " << endl;

        char path[512] = {0};
        string accumulate;
        int start_offset;
        unsigned int total_size = 0; 

        if ((PendingWrites.size() == 0) && flag_read) {
        cout<<"[DEBUG] server: nfsfuse_commit: pending 0 after read" << endl;
            close(input->fh());
            reply->set_err(0);
            flag_read = false;
            return Status::OK;
        }


        if ((PendingWrites.size() == 0) && (RetransWrites.size() == 0) ){
        cout<<"[DEBUG] server: nfsfuse_commit: pending size is 0 " << endl;
            reply->set_serveroff(input->endoff());
            reply->set_err(2);
            return Status::OK;
        }
        else if(RetransWrites.size() != 0){
        cout<<"[DEBUG] server: nfsfuse_commit: retrans size != 0" << endl;
            translatePath(RetransWrites.begin()->path().c_str(), path);
            start_offset = RetransWrites.begin()->offset();
            for(int i=0; i<RetransWrites.size(); i++){    
                accumulate.append(RetransWrites.at(i).buffer());
                total_size += RetransWrites.at(i).size();
            }
        }
        else if(input->firstoff() != PendingWrites.begin()->offset()){
        cout<<"[DEBUG] server: nfsfuse_commit: offset not equal " << endl;
 
            reply->set_serveroff(PendingWrites.begin()->offset());
            reply->set_err(-1);
            return Status::OK;
        } 
        else{
        cout<<"[DEBUG] server: nfsfuse_commit: offset is equal " << endl;
            start_offset = PendingWrites.begin()->offset();
            translatePath(PendingWrites.begin()->path().c_str(), path);
            
        }

        for(int i=0; i<PendingWrites.size(); i++){    
            accumulate.append(PendingWrites.at(i).buffer());
            total_size += PendingWrites.at(i).size();
        }
        
        int fd = open(path, O_WRONLY);
        cout<<"[DEBUG] : release open fd: "<<fd<< endl;
        if(fd == -1){
            cout<<"[DEBUG] : batch open fails!"<<endl;
            reply->set_err(errno);
            perror(strerror(errno));
            return Status::OK;
        } 
        
        int res = pwrite(fd, accumulate.c_str(), total_size, start_offset);
		cout<<"[DEBUG] : nfsfuse_write: BATCH res"<<res<<endl;
        if(res == -1){
            reply->set_err(errno);
            perror(strerror(errno));
            return Status::OK;
        }

        fsync(fd);
        close(fd);

        int bound = PendingWrites.size();
        for(int i=0; i<bound; i++){
            PendingWrites.pop_back();
        }
        
        bound = RetransWrites.size();
        for(int i=0; i<bound; i++){
            RetransWrites.pop_back();
        }
        

	    reply->set_err(0);
        return Status::OK;
    }

  //----------------------------------------
  // test server crash
  Status nfsfuse_kill(ServerContext* context, const String* input,
                        Errno* reply) override {
      exit(0);
  }
  //----------------------------------------



};

void RunServer() {
  std::string server_address("0.0.0.0:50051");
  NfsServiceImpl service;

  ServerBuilder builder;
  // Listen on the given address without any authentication mechanism.
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  // Register "service" as the instance through which we'll communicate with
  // clients. In this case it corresponds to an *synchronous* service.
  builder.RegisterService(&service);
  // Finally assemble the server.
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "Server listening on " << server_address << std::endl;

  // Wait for the server to shutdown. Note that some other thread must be
  // responsible for shutting down the server for this call to ever return.
  server->Wait();
}

int main(int argc, char** argv) {
  RunServer();

  return 0;
}

