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
            //reply->set_rdev(st.st_rdev);
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
		cout<<"[DEBUG] : nfsfuse_read: "<<path<<endl;


        int fd = open(path, O_RDONLY);
		cout<<"[DEBUG] : nfsfuse_read: fd "<<fd<<endl;
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
        char path[512] = {0};
        translatePath(wr->path().c_str(), path);
        int fd = open(path, O_WRONLY);
		cout<<"[DEBUG] : nfsfuse_write: path "<<path<<endl;
		cout<<"[DEBUG] : nfsfuse_write: fd "<<fd<<endl;
        if(fd == -1){
            reply->set_err(errno);
            perror(strerror(errno));
            return Status::OK;
        } 

        int res = pwrite(fd, wr->buffer().c_str(), wr->size(), wr->offset());
		cout<<"[DEBUG] : nfsfuse_write: res"<<res<<endl;
        if(res == -1){
            reply->set_err(errno);
            perror(strerror(errno));
            return Status::OK;
        }
        
        reply->set_nbytes(res);
        reply->set_err(0);
    
        if(fd>0)
            close(fd);

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
            cout << "server path: " << server_path << endl;
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
            cout << "server path: " << server_path << endl;
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
            cout << "server path: " << server_path << endl;
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
        cout << "from path: " << from_path << endl;
        cout << "to path: " << to_path << endl;

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
	cout << "long size: " << sizeof(long) << endl;
	cout << "time_t size: " << sizeof(time_t) << endl;
        
	char server_path[512]={0};
        translatePath(input->path().c_str(), server_path);
        cout << "server path: " << server_path << endl;



        struct timespec ts[2];
	long oo;
	int ii;

	cout << "sec type: " << typeid(ts[0].tv_sec).name() << endl;
        cout << "nsec type: " << typeid(ts[0].tv_nsec).name() << endl;
	cout << "long type: " << typeid(oo).name() << endl;
	cout << "int type: " << typeid(ii).name() << endl;	

	ts[0].tv_sec = input->sec();
	ts[0].tv_nsec = input->nsec();
	//ts[0].tv_nsec = UTIME_NOW;
	ts[1].tv_sec = input->sec2();
	ts[1].tv_nsec = input->nsec2();
	//ts[1].tv_nsec = UTIME_NOW;

	//ts[0].tv_sec = 100;
	//ts[0].tv_nsec = 1000;
	//ts[1].tv_sec = 200;
	//ts[1].tv_nsec = 2000;

	cout << "ts[0]:" << ts[0].tv_sec << "  " << ts[0].tv_nsec << endl;
        cout << "ts[1]:" << ts[1].tv_sec << "  " << ts[1].tv_nsec << endl;

        int res = utimensat(AT_FDCWD, server_path, ts, AT_SYMLINK_NOFOLLOW);
        cout << "done  res:" << res << endl;

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
        //fixme do u forget res = open()?
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

