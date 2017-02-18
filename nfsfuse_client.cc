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

static struct options {	
	NfsClient* nfsclient;
	int show_help;
} options;

#define OPTION(t, p)                           \
    { t, offsetof(struct options, p), 1 }
static const struct fuse_opt option_spec[] = {
	OPTION("-h", show_help),
	OPTION("--help", show_help),
	FUSE_OPT_END
};

static void show_help(const char *progname)
{
	std::cout<<"usage: "<<progname<<" [-s -d] <mountpoint>\n\n";
}

static void *client_init(struct fuse_conn_info *conn, 
				struct fuse_config *cfg){
    (void) conn;
    cfg->kernel_cache = 1;
    return NULL;
}

static int client_getattr(const char *path, struct stat *stbuf,
             struct fuse_file_info *fi)
{
    (void) fi;
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));		
	res = options.nfsclient->rpc_lstat(path, stbuf);
	//res = lstat(path, stbuf);
    if (res == -1)
        return -errno;
	
    return 0;
}

static int client_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
               off_t offset, struct fuse_file_info *fi,
               enum fuse_readdir_flags flags)
{
    return options.nfsclient->rpc_readdir(path, buf, filler);
}

static int client_open(const char *path, struct fuse_file_info *fi)
{
    return options.nfsclient->rpc_open(path, fi);
}

static int client_read(const char *path, char *buf, size_t size, off_t offset,
            struct fuse_file_info *fi)
{
    return options.nfsclient->rpc_read(path, buf, size, offset, fi);
}

static struct client_operations : fuse_operations {
    client_operations(){
        init = client_init;
        getattr = client_getattr;
        readdir = client_readdir;
        open = client_open;
        read = client_read;
	
	
	/*
	mkdir	= client_mkdir;
        unlink  = client_unlink;
        write   = client_write;
        flush   = client_flush;
        rename  = client_rename;
        rmdir   = client_rmdir;
        release = client_release;
        create  = client_create;
        utimens   = client_utimens;
	*/
    }

} client_oper;



int main(int argc, char* argv[]){

	struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
	//umask(0);

	options.nfsclient = new NfsClient(grpc::CreateChannel(
  "0.0.0.0:50051", grpc::InsecureChannelCredentials()));


    if (fuse_opt_parse(&args, &options, option_spec, NULL) == -1)
	return 1;

    if (options.show_help) {
	show_help(argv[0]);
	assert(fuse_opt_add_arg(&args, "--help") == 0);
	args.argv[0] = (char*) "";
    }


return fuse_main(argc, argv, &client_oper, &options);
}
















