
#ifndef _GITFS_FS_H
#define _GITFS_FS_H

#include <fuse.h>

int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
	off_t offset, __attribute__((unused)) struct fuse_file_info *fi);

int getattr_callback(const char *path, struct stat *stbuf);

int open_callback(__attribute__((unused)) const char *path, __attribute__((unused)) struct fuse_file_info *fi);

int read_callback(const char *path, char *buf, size_t size, off_t offset,
	__attribute__((unused)) struct fuse_file_info *fi);

#endif
