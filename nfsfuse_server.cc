#include <iostream>
#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#include <grpc++/grpc++.h>

#include "nfsfuse.grpc.pb.h"

#define READ_MAX    10000000 //10MB

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
		reply->set_ino(st.st_ino);
		reply->set_mode(st.st_mode);
		reply->set_nlink(st.st_nlink);
		reply->set_uid(st.st_uid);
		reply->set_gid(st.st_gid);
		//reply->set_rdev(st.st_rdev);
		reply->set_size(st.st_size);
		reply->set_blksize(st.st_blksize);
		reply->set_blocks(st.st_blocks);
		reply->set_atime(st.st_atime);
		reply->set_mtime(st.st_mtime);
		reply->set_ctime(st.st_ctime);
			
        if(res == -1){
		    perror(strerror(errno));
		    reply->set_err(errno);
		    //return Status::CANCELLED; 
		}
		else{
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
			return Status::CANCELLED;
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
            return Status::CANCELLED;
        }
        else{
            fi_reply->set_fh(fh);            
            fi_reply->set_err(0);
            close(fh);
            return Status::OK;
        }
    }

    Status nfsfuse_read(ServerContext* context, const ReadRequest* rr, 
            ReadResult* reply) override {
        char path[512];
        char *buf = new char[rr->size()];
        translatePath(rr->path().c_str(), path);
		cout<<"[DEBUG] : nfsfuse_read: "<<path<<endl;


        int fd = open(path, O_RDONLY);
		cout<<"[DEBUG] : nfsfuse_read: fd "<<fd<<endl;
        if (fd == -1){
            reply->set_bytesread(-1);
		    perror(strerror(errno));
            return Status::CANCELLED;
        }

        int res = pread(fd, buf, rr->size(), rr->offset());
        if (res == -1){
            reply->set_bytesread(-1);
		    perror(strerror(errno));
            return Status::CANCELLED;
        }

        reply->set_bytesread(res);
        reply->set_buffer(buf);
        
        if(fd>0)
            close(fd);
        free(buf);
        return Status::OK;

    }


    Status nfsfuse_write(ServerContext* context, const WriteRequest* wr, 
            WriteResult* reply) override {
        char path[512] = {0};
        translatePath(wr->path().c_str(), path);
        int fd = open(path, O_WRONLY);
		cout<<"[DEBUG] : nfsfuse_write: path "<<path<<endl;
		cout<<"[DEBUG] : nfsfuse_write: fd "<<fd<<endl;
        if(fd == -1){
            reply->set_err(-errno);
            perror(strerror(errno));
            Status::CANCELLED;
        } 

        int res = pwrite(fd, wr->buffer().c_str(), wr->size(), wr->offset());
		cout<<"[DEBUG] : nfsfuse_write: res"<<res<<endl;
        if(res == -1){
            reply->set_err(-errno);
            perror(strerror(errno));
            Status::CANCELLED;
        }
        
        reply->set_nbytes(res);
        reply->set_err(0);
    
        if(fd>0)
            close(fd);

        return Status::OK;
    }
    Status nfsfuse_mkdir(ServerContext* context, const Mkdir* input,
                                         MkdirOutput* reply) override {
            cout<<"[DEBUG] : mkdir: " << endl;
		
            char server_path[512]={0};
            translatePath(input->s().c_str(), server_path);
            cout << "server path: " << server_path << endl;
            int res = mkdir(server_path, input->mode());
		

            if (res == -1) {
                perror(strerror(errno)); 
                reply->set_err(-1);
                reply->set_str("Mkdir fail");
                return Status::CANCELLED; 
            } else {
	        reply->set_err(0);
                reply->set_str("Mkdir succeed");
            }

            return Status::OK;
    }

    Status nfsfuse_rmdir(ServerContext* context, const String* input,
                                         MkdirOutput* reply) override {
            cout<<"[DEBUG] : rmdir: " << endl;

            char server_path[512]={0};
            translatePath(input->str().c_str(), server_path);
            cout << "server path: " << server_path << endl;
            int res = rmdir(server_path);


            if (res == -1) {
                perror(strerror(errno));
                reply->set_err(-1);
                reply->set_str("unlink fail");
                return Status::CANCELLED;
            } else {
                reply->set_err(0);
                reply->set_str("unlink succeed");
            }

            return Status::OK;
    }


<<<<<<< HEAD
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
            return Status::CANCELLED;
        }
        else{
            reply->set_fh(fh);
            reply->set_err(0);
            close(fh);
            return Status::OK;
        }
    }


};
=======
>>>>>>> 587442fde93d1073a101350e1e8b6b6751ecc7b9

    Status nfsfuse_unlink(ServerContext* context, const String* input,
                                         MkdirOutput* reply) override {
            cout<<"[DEBUG] : unlink " << endl;

            char server_path[512]={0};
            translatePath(input->str().c_str(), server_path);
            cout << "server path: " << server_path << endl;
            int res = unlink(server_path);


            if (res == -1) {
                perror(strerror(errno));
                reply->set_err(-1);
                reply->set_str("rmdir fail");
                return Status::CANCELLED;
            } else {
                reply->set_err(0);
                reply->set_str("rmdir succeed");
            }

            return Status::OK;
    }
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

