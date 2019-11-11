#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <fuse.h>
#include <lmdb.h>

#include "repo.h"
#include "fs.h"
#include "conf.h"

int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
	off_t offset, __attribute__((unused)) struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	MDB_cursor *cursor;
	MDB_val key, data;
	repo *r;
	int rc;
	struct fuse_context *fctx = fuse_get_context();
	gfsconf *gfs = fctx->private_data;
	if (strncmp(path, "/", 2) == 0) {
		if ((rc = mdb_cursor_open(gfs->txn, gfs->dbio, &cursor)) != 0) {
			fprintf(stderr, "failed to create cursor: (%d) %s\n", rc, mdb_strerror(rc));
			return -ENOENT;
		}
		while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0)
			filler(buf, (char *) key.mv_data, NULL, 0);
		mdb_cursor_close(cursor);
	} else {
		if ((rc = mdb_cursor_open(gfs->txn, gfs->dbir, &cursor)) != 0) {
			fprintf(stderr, "failed to create cursor: (%d) %s\n", rc, mdb_strerror(rc));
			return -ENOENT;
		}
		while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
			r = data.mv_data;
			if (strcmp(path, r->path[0]) == 0) {
				filler(buf, r->name, NULL, 0);
				printf("name: %s\n", r->name);
			}
		}
		mdb_cursor_close(cursor);
	}

	return 0;
}

int getattr_callback(const char *path, struct stat *stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));

	if (strncmp(path, "/", 2) == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}

	int rc;
	MDB_cursor *cursor;
	MDB_val key, data;
	struct fuse_context *fctx = fuse_get_context();
	gfsconf *gfs = fctx->private_data;
	if ((rc = mdb_cursor_open(gfs->txn, gfs->dbir, &cursor)) != 0) {
		fprintf(stderr, "failed to create cursor: (%d) %s\n", rc, mdb_strerror(rc));
		return -ENOENT;
	}
	repo *r;
	while ((rc = mdb_cursor_get(cursor, &key, &data, MDB_NEXT)) == 0) {
		r = data.mv_data;
		if (strcmp(path, r->path[0]) == 0) {
			stbuf->st_mode = S_IFDIR | 0755;
			stbuf->st_nlink = 2;
			break;
		}
		if (strcmp(path, r->path[1]) == 0) {
			stbuf->st_mode = S_IFREG | 0444;
			stbuf->st_nlink = 1;
			stbuf->st_size = strlen(repo_string(r));
			break;
		}
	}
	mdb_cursor_close(cursor);

	return 0;
}

int open_callback(__attribute__((unused)) const char *path, __attribute__((unused)) struct fuse_file_info *fi) {
	return 0;
}

int read_callback(const char *path, char *buf, size_t size, off_t offset,
	__attribute__((unused)) struct fuse_file_info *fi)
{
	MDB_val key, data;
	repo *r;
	int rc;
	char *ptr = strdup(path);
	ptr++;

	key.mv_data = strdup(ptr);
	key.mv_size = strlen(ptr) + 1;
	struct fuse_context *fctx = fuse_get_context();
	gfsconf *gfs = fctx->private_data;
	if ((rc = mdb_get(gfs->txn, gfs->dbir, &key, &data)) == 0) {
		r = data.mv_data;
		char *content = repo_string(r);
		size_t len = strlen(content);
		if ((size_t)offset >= len)
			return 0;

		if (offset + size > len) {
			memcpy(buf, content + offset, len - offset);
			return len - offset;
		}

		memcpy(buf, content + offset, size);
		return size;
	}
	fprintf(stderr, "key not found: (%d) %s\n", rc, mdb_strerror(rc));

	return -ENOENT;
}
