#include <grpc++/grpc++.h>
#include "nfsfuse.grpc.pb.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using grpc::ClientReader;
using nfsfuse::NFS;
using nfsfuse::String;
using nfsfuse::SerializeByte;
using nfsfuse::Dirent;

using namespace std;

struct sdata{
    int a;
    char b[10];
};


class NfsClient {
 public:
  NfsClient(std::shared_ptr<Channel> channel)
      : stub_(NFS::NewStub(channel)) {}

  int function1(const int n, struct sdata* out) {
    sdata s_data;
	s_data.a = n;
	strcpy(s_data.b,"hello");
	SerializeByte sb;
	char* ch = reinterpret_cast<char*>(&s_data);
	sb.set_buffer(ch, sizeof(sdata));
	
	SerializeByte rbyte;

    ClientContext context;
    Status status = stub_->function1(&context, sb, &rbyte);
    if (status.ok()) {
	  *out = *reinterpret_cast<const sdata*> (rbyte.buffer().c_str());
	  return 0;
    } else {
      std::cout << "error " << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
  }
  
  int rpc_lstat(string path, struct stat* output){
	SerializeByte result;
	ClientContext context;
	String p;
	p.set_str(path);
	Status status = stub_->rpc_lstat(&context, p, &result);
	if (status.ok()) {
      *output = *reinterpret_cast<const struct stat*> (result.buffer().c_str());	
	  return 0;
    } else {
      std::cout << "error " << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
  }
/*
  DIR* rpc_opendir(string path){
	SerializeByte result;
	ClientContext context;
	String p;
	p.set_str(path);
	Status status = stub_->rpc_opendir(&context, p, &result);
	if(status.ok()){
		DIR* p = new DIR();
		*p = *reinterpret_cast<const DIR*>(result.buffer().c_str());
		return p;
	}
	else{
	   std::cout << "error " << status.error_code() << ": " << status.error_message()
                << std::endl;
      return nullptr;
	}
  }

  dirent* rpc_readdir(DIR* dp){
      Dirent res;
	  SerializeByte req;
	  ClientContext ctx;

	  req->set_buffer(reinterpret_cast<const char*>(dp), sizeof(*dp));
	  Status status = stub_->readdir(&context, req, &res);
      if (status.ok()) {
		  dirent* de = new struct dirent;
		  de->d_ino = res.d_ino();
		  de->d_name = res.d_name;
          return de;
      } else {
          std::cout << "error " << status.error_code() << ": " << status.error_message()
                << std::endl;
          return nullptr;
    }
	
  }
*/
 private:
  std::unique_ptr<NFS::Stub> stub_;
};











