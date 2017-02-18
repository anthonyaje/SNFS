/** @file
 *
 * minimal example filesystem using high-level API
 *
 * Compile with:
 *
 *     g++ -Wall client.cc `pkg-config fuse3 --cflags --libs` -o client -std=c++11
 *
 * ## Source code ##
 * \include client.c
 */

#define FUSE_USE_VERSION 30

//#include <config.h>

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

#include "NfsClient.h"

NfsClient nfsclient(grpc::CreateChannel(
  "0.0.0.0:50051", grpc::InsecureChannelCredentials()));

static void *client_init(struct fuse_conn_info *conn, 
				struct fuse_config *cfg){
	(void) conn;
    //cfg->kernel_cache = 1;
    return NULL;
}

static int client_getattr(const char *path, struct stat *stbuf,
             struct fuse_file_info *fi)
{
    (void) fi;
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));		
	res = nfsclient.rpc_lstat(path, stbuf);
	//res = lstat(path, stbuf);
    if (res == -1)
        return -errno;
	
    return 0;
}

static int client_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
               off_t offset, struct fuse_file_info *fi,
               enum fuse_readdir_flags flags)
{
    DIR *dp;
    struct dirent *de;
    (void) offset;
    (void) fi;
    (void) flags;
	
    return nfsclient.rpc_readdir(path, buf, filler);
}

static int client_open(const char *path, struct fuse_file_info *fi)
{
	int res;

	//todo
    res = open(path, fi->flags);
    if (res == -1)
        return -errno;

    close(res);
    return 0;

}

static int client_read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi)
{
    int fd;
    int res;

    (void) fi;
	//todo
    fd = open(path, O_RDONLY);
    if (fd == -1)
        return -errno;

    res = pread(fd, buf, size, offset);
    if (res == -1)
        res = -errno;

    close(fd);
    return res;
}

static struct fuse_operations client_oper;



int main(int argc, char* argv[]){
	client_oper.init = client_init;
	client_oper.getattr = client_getattr;
	client_oper.readdir = client_readdir;
	client_oper.open = client_open;
	client_oper.read = client_read;
	umask(0);

    //struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
   	/* 
	argv[argc-2] = argv[argc-1];
    argv[argc-1] = NULL;
    argc--;
	*/
	

	
	int input = 5;	
	sdata reply;
	int ret = nfsclient.function1(input, &reply);
	std::cout<<"a "<<reply.a<<std::endl;
	std::cout<<"b "<<reply.b<<std::endl;
	//std::free(reply);

	return fuse_main(argc, argv, &client_oper, NULL);
}
















