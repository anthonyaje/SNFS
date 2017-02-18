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

  int rpc_lstat(string path, struct stat* output){
	Stat result;
	ClientContext context;
	String p;
	p.set_str(path);

	Status status = stub_->nfsfuse_lstat(&context, p, &result);
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
	
	return result.err();
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
	
 private:
  std::unique_ptr<NFS::Stub> stub_;
};











