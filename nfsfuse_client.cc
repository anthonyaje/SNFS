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

//struct Options options;

#define OPTION(t, p)                           \
    { t, offsetof(struct Options, p), 1 }

int kill_times = 0;

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
    //cfg->kernel_cache = 1;
    return NULL;
}

static int client_getattr(const char *path, struct stat *stbuf,
             struct fuse_file_info *fi)
{
    memset(stbuf, 0, sizeof(struct stat));		
	return options.nfsclient->rpc_getattr(path, stbuf);
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

static int client_write(const char *path, const char *buf, size_t size,
             off_t offset, struct fuse_file_info *fi)
{
    return options.nfsclient->rpc_write(path, buf, size, offset, fi);
}

static int client_mkdir(const char *path, mode_t mode) {
    return options.nfsclient->rpc_mkdir(path, mode);
}

static int client_rmdir(const char *path)
{
    return options.nfsclient->rpc_rmdir(path);
}


static int client_create(const char *path, mode_t mode, struct fuse_file_info *fi)
{
    return options.nfsclient->rpc_create(path, mode, fi);
}

static int client_unlink(const char *path)
{
    return options.nfsclient->rpc_unlink(path);    
}

static int client_rename(const char *from, const char *to, unsigned int flags)
{
    return options.nfsclient->rpc_rename(from, to, flags);
}


static int client_utimens(const char *path, const struct timespec ts[2],
		       struct fuse_file_info *fi)
{
    cout << "ts0 sec:" << ts[0].tv_sec << endl;
    cout << "ts0 nsec:" << ts[0].tv_nsec << endl;
    cout << "ts1 sec:" << ts[1].tv_sec << endl;
    cout << "ts1 nsec:" << ts[1].tv_nsec << endl;

    return options.nfsclient->rpc_utimens(path, ts, fi);
}

static int client_mknod(const char *path, mode_t mode, dev_t rdev)
{
    return options.nfsclient->rpc_mknod(path, mode, rdev);
}

static int client_flush(const char *path, struct fuse_file_info *fi)
{
    int res;

    (void) path;
    /* This is called from every close on an open file, so call the
       close on the underlying filesystem.  But since flush may be
       called multiple times for an open file, this must not really
       close the file.  This is important if used on a network
       filesystem like NFS which flush the data/metadata on close() */
/*    res = close(dup(fi->fh));
    if (res == -1)
        return -errno;
*/
    cout<<"FLUSH is called !!!!!!!!!!!!!!!!!!!!!!!!! "<<endl;
    return 0;
}

static int client_release(const char *path, struct fuse_file_info *fi)
{
    (void) path;
    cout<<"RELEASE is called !!!!!!!!!!!!!!!!!!!!!!!!! "<<endl;
//    close(fi->fh);
//-------------------------------
//	test server crash upon commit
    if (kill_times == 0) {
        kill_times++;
        options.nfsclient->rpc_kill();
    }
//-------------------------------

    return options.nfsclient->rpc_commit(fi->fh, PendingWrites.begin()->offset(), 
                PendingWrites.end()->offset());
}

static struct client_operations : fuse_operations {
    client_operations(){
        init = client_init;
        getattr = client_getattr;
        readdir = client_readdir;
        open = client_open;
        read = client_read;
        write = client_write;
        create  = client_create;
        mkdir	= client_mkdir;
        rmdir = client_rmdir;
        unlink  = client_unlink;
        rename = client_rename;
        utimens = client_utimens;
        mknod = client_mknod;
        flush   = client_flush;
        release = client_release;
	
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
















