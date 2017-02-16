#include <iostream>
#include <memory>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <grpc++/grpc++.h>

#include "nfsfuse.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using nfsfuse::NFS;
using nfsfuse::String;
using nfsfuse::SerializeByte;

using namespace std;

struct sdata{
	int a;
	char b[10]={};
};

class NfsServiceImpl final : public NFS::Service {
	Status function1(ServerContext* context, const SerializeByte* request,
					 SerializeByte* reply) override {
    	sdata s_sdata;
		s_sdata.a = 100;
		strcpy(s_sdata.b,"hello");
		
		const sdata* r_data = reinterpret_cast<const sdata*>(request->buffer().c_str());
	    std::cout<<r_data->a<<std::endl;
	    std::cout<<r_data->b<<std::endl;
		
		cout<<sizeof(sdata)<<endl;
		reply->set_buffer(reinterpret_cast<const char*>(&s_sdata), sizeof(sdata));

	  	return Status::OK;
	}
/*
	Status rpc_lstat(ServerContext* context, const String* s, 
					 SerializeByte* reply) override {
		struct stat st;
		int res = lstat();
		
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

