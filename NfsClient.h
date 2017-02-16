#include <grpc++/grpc++.h>
#include "nfsfuse.grpc.pb.h"
#include <sys/stat.h>

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using grpc::ClientReader;
using nfsfuse::NFS;
using nfsfuse::String;
using nfsfuse::SerializeByte;

using namespace std;

struct sdata{
    int a;
    char b[10];
};


class NfsClient {
 public:
  NfsClient(std::shared_ptr<Channel> channel)
      : stub_(NFS::NewStub(channel)) {}

  const sdata* function1(const int n) {
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
	  return reinterpret_cast<const sdata*> (rbyte.buffer().c_str());
    } else {
      std::cout << "error " << status.error_code() << ": " << status.error_message()
                << std::endl;
      return nullptr;
    }
  }
  /*
  int rpc_lstat(string path, struct stat* output){
	Serializable result;
	ClientContext context;
	Status status = stub_->rpc_lstat(&context, path, &result);
	if (status.ok()) {
      output = reinterpret_cast<const struct stat*> (result.buffer().c_str());	
	  return 0;
    } else {
      std::cout << "error " << status.error_code() << ": " << status.error_message()
                << std::endl;
      return -1;
    }
	
	
  }*/

 private:
  std::unique_ptr<NFS::Stub> stub_;
};

