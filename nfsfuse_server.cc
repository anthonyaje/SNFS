#include <iostream>
#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <grpc++/grpc++.h>

#include "nfsfuse.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using nfsfuse::NFS;
using nfsfuse::String;
using nfsfuse::SerializeByte;
using nfsfuse::Dirent;
using nfsfuse::Stat;

using namespace std;

struct sdata{
	int a;
	char b[10]={};
};

class NfsServiceImpl final : public NFS::Service {
	Status function1(ServerContext* context, const SerializeByte* request,
					 SerializeByte* reply) override {
		const sdata* r_data = reinterpret_cast<const sdata*>(request->buffer().c_str());
	    std::cout<<r_data->a<<std::endl;
	    std::cout<<r_data->b<<std::endl;
		
    	sdata s_sdata;
		s_sdata.a = 100;
		strcpy(s_sdata.b,"hello");
		
		cout<<sizeof(sdata)<<endl;
		reply->set_buffer(reinterpret_cast<const char*>(&s_sdata), sizeof(sdata));

	  	return Status::OK;
	}

	Status nfsfuse_lstat(ServerContext* context, const String* s, 
					 Stat* reply) override {
		struct stat st;
		int res = lstat(s->str().c_str(), &st);
	    if(res == -1)
			return Status::CANCELLED; 
		//reply->set_buffer(reinterpret_cast<const char*>(&st), sizeof(struct stat));	
		reply->set_ino(st.st_ino);
		reply->set_mode(st.st_mode);
		reply->set_nlink(st.st_nlink);
		reply->set_uid(st.st_uid);
		reply->set_gid(st.st_gid);
		//`reply->set_rdev(st.st_rdev);
		reply->set_size(st.st_size);
		reply->set_blksize(st.st_blksize);
		reply->set_blocks(st.st_blocks);
		reply->set_atime(st.st_atime);
		reply->set_mtime(st.st_mtime);
		reply->set_ctime(st.st_ctime);
		
		cout<<"ino"<<reply->ino()<<endl;

		return Status::OK;
	}
	
	Status nfsfuse_readdir(ServerContext* context, const String* s,
						Dirent* reply) override {
		DIR *dp;
		struct dirent *de;

		dp = opendir(s->str().c_str());
		if (dp == NULL)
			return Status::CANCELLED;
			
		de = readdir(dp);
		reply->set_d_ino(de->d_ino);
		reply->set_d_name(de->d_name);

		closedir(dp);
		return Status::OK;
		
	}

/*
	Status rpc_opendir(ServerContext* context, const String* s, SerializeByte* reply){
		DIR *dp;
		dp = opendir(s->str().c_str());
		if(dp == NULL)
			return Status::CANCELLED;
		reply->set_buffer(reinterpret_cast<const char*>(dp), sizeof(*dp));
		return Status::OK;
	}
	
	Status rpc_readdir(ServerContext* context, const SerializeByte* dir, Dirent* reply){
		DIR *dp;
		dirent* de;
		dp = reinterpret_cast<const DIR*>(dir.buffer().c_str());
		readdir(dp);	
		
	}
*/

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

