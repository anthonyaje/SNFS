#include <grpc++/grpc++.h>
#include "nfsfuse.grpc.pb.h"

using grpc::Channel;
using grpc::Status;
using grpc::ClientContext;
using grpc::ClientReader;
using nfsfuse::NFS;
using nfsfuse::MyString;
using nfsfuse::SerializeByte;
//using nfsfuse::

class NfsClient {
 public:
  NfsClient(std::shared_ptr<Channel> channel)
      : stub_(NFS::NewStub(channel)) {}

  std::string function1(const int n) {
	MyString str;    
    str.set_msg(std::to_string(n));
	
	SerializeByte sbyte;

    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->function1(&context, str, &sbyte);

    // Act upon its status.
    if (status.ok()) {
      return sbyte.buffer();
    } else {
      std::cout << "error " << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<NFS::Stub> stub_;
};

