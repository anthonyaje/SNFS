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
using nfsfuse::Stat;

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
	Stat result;
	ClientContext context;
	String p;
	p.set_str(path);

	Status status = stub_->nfsfuse_lstat(&context, p, &result);
	memset(output, 0, sizeof(struct stat));
//	output->st_ino = result.ino();
	output->st_mode = result.mode();
	output->st_nlink = result.nlink();
//	output->st_uid = result.uid();
//	output->st_gid = result.gid();
//	output->st_rdev = result.rdev();
	output->st_size = result.size();
//	output->st_blksize = result.blksize();
//	output->st_blocks = result.blocks();
//	output->st_atime = result.atime();
//	output->st_mtime = result.mtime();
//	output->st_ctime = result.ctime();
	
	if (result.err() != 0) {
            std::cout << "error " << result.err() << std::endl;
        }
	
	return result.err();
  }
  
  int rpc_readdir(string p, void *buf, fuse_fill_dir_t filler){
	String path;
	path.set_str(p);
	Dirent result;
	dirent de;
	Status status;
	ClientContext ctx;
    
	status = stub_->nfsfuse_readdir(&ctx, path, &result);
	while (status.ok()) {
            struct stat st;
            memset(&st, 0, sizeof(st));

	    de.d_ino = result.dino();
	    strcpy(de.d_name, result.dname().c_str());
	    de.d_type = result.dtype();

            st.st_ino = de.d_ino;
            st.st_mode = de.d_type << 12;

            if (filler(buf, result.dname().c_str(), &st, 0, static_cast<fuse_fill_dir_flags>(0)))
               break;
	    Dirent result;	
	    status = stub_->nfsfuse_readdir(&ctx, path, &result);
        }
	return 0;
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











