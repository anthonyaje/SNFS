/** @file
 *
 * minimal example filesystem using high-level API
 *
 * Compile with:
 *
 *     gcc -Wall client.c `pkg-config fuse3 --cflags --libs` -o client
 *
 * ## Source code ##
 * \include client.c
 */

#define FUSE_USE_VERSION 30

#include <config.h>

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
	
	//todo RPC
	res = lstat(path, stbuf);
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
	
	//todo
    //dp = opendir(path);
    dp = opendir("/home/a3anthon/cs798-2/");
    if (dp == NULL)
        return -errno;

    while ((de = readdir(dp)) != NULL) {
        struct stat st;
        memset(&st, 0, sizeof(st));
        st.st_ino = de->d_ino;
        st.st_mode = de->d_type << 12;
        if (filler(buf, de->d_name, &st, 0, 0))
            break;
    }

    closedir(dp);
    return 0;
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

static struct fuse_operations client_oper = {
	.init			= client_init,
	.getattr	= client_getattr,
	.readdir	= client_readdir,
	.open		= client_open,
	.read 		= client_read,
};

int main(int argc, char* argv[]){
	umask(0);

	return fuse_main(argc, argv, &client_oper, NULL);
}

