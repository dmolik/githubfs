#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64

#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <fuse.h>
#include <lmdb.h>

#include "repo.h"
#include "fetch.h"

struct list *repos;

char **dup;
int   dup_length = 1;

void dedup_list(struct list *li)
{
	struct el *elm = li->first;
	repo *r;
	while (elm != NULL) {
		r = elm->data;
		char *tmp = strdup(r->path[0]);
		tmp++;
		for (int i = 0; i < dup_length; i++) {
			if (i == 0 && dup_length == 1) {
				// list is uninitialized
				dup[i] = malloc(strlen(tmp));
				dup[i] = strdup(tmp);
				dup_length++;
				break;
			} else if (i == dup_length - 1) { // last element
				dup[i] = malloc(strlen(tmp));
				dup[i] = strdup(tmp);
				dup_length++;
				break;
			} else if (strcmp(tmp, dup[i]) == 0) {
				break;
			}
		}
		elm = elm->next;
	}
	dup_length--;
}

static int readdir_callback(const char *path, void *buf, fuse_fill_dir_t filler,
	off_t offset, __attribute__((unused)) struct fuse_file_info *fi)
{
	(void) offset;
	(void) fi;

	filler(buf, ".", NULL, 0);
	filler(buf, "..", NULL, 0);

	if (strncmp(path, "/", 2) == 0) {
		for (int i = 0; i < dup_length; i++) {
			filler(buf, dup[i], NULL, 0);
		}
	} else {
		struct el *elm = repos->first;
		repo *r;
		while (elm != NULL) {
			r = elm->data;
			if (strcmp(path, r->path[0]) == 0) {
				filler(buf, r->name, NULL, 0);
			}
			elm = elm->next;
		}
	}

	return 0;
}

static int getattr_callback(const char *path, struct stat *stbuf)
{
	memset(stbuf, 0, sizeof(struct stat));

	if (strncmp(path, "/", 2) == 0) {
		stbuf->st_mode = S_IFDIR | 0755;
		stbuf->st_nlink = 2;
		return 0;
	}

	struct el *elm = repos->first;
	repo *r;
	while (elm != NULL) {
		r = elm->data;
		if (strcmp(path, r->path[0]) == 0) {
			stbuf->st_mode = S_IFDIR | 0755;
			stbuf->st_nlink = 2;
			return 0;
		}
		elm = elm->next;
	}

	elm = repos->first;
	while (elm != NULL) {
		r = elm->data;
		if (strcmp(path, r->path[1]) == 0) {
			stbuf->st_mode = S_IFREG | 0444;
			stbuf->st_nlink = 1;
			stbuf->st_size = strlen(repo_string(r));
			return 0;
		}
		elm = elm->next;
	}

	return -ENOENT;
}

static int open_callback(__attribute__((unused)) const char *path, __attribute__((unused)) struct fuse_file_info *fi) {
	return 0;
}

static int read_callback(const char *path, char *buf, size_t size, off_t offset,
	__attribute__((unused)) struct fuse_file_info *fi)
{
	struct el *elm = repos->first;
	repo *r;
	while (elm != NULL) {
		r = elm->data;
		if (strncmp(path, r->path[1], strlen(r->path[1]) + 1) == 0) {
			char *content = repo_string(r);
			size_t len = strlen(content);
			if ((size_t)offset >= len) {
				return 0;
			}

			if (offset + size > len) {
				memcpy(buf, content + offset, len - offset);
				return len - offset;
			}

			memcpy(buf, content + offset, size);
			return size;
		}
		elm = elm->next;
	}
	return -ENOENT;
}

static struct fuse_operations fuse_fetcher_opts = {
	.getattr = getattr_callback,
	.open    = open_callback,
	.read    = read_callback,
	.readdir = readdir_callback,
};

static int _mkdir(const char *dir, mode_t mode)
{
	char tmp[256];
	char *p = NULL;
	size_t len;

	snprintf(tmp, sizeof(tmp), "%s", dir);
	len = strlen(tmp);
	if (tmp[len - 1] == '/')
		tmp[len - 1] = 0;
	for(p = tmp + 1; *p; p++)
		if (*p == '/') {
			*p = 0;
			
			if (mkdir(tmp, mode) == -1 && errno != EEXIST)
				return -1;
			*p = '/';
		}
	return mkdir(tmp, mode);
}

int main(int argc, char *argv[])
{
	// getenv("HOME")
	_mkdir("t/db",     0750);
	_mkdir("t/repos",  0750);

	char *user = getenv("GH_USER");
	char *pass = getenv("GH_TOKEN");

	char *upass = malloc(strlen(user) + strlen(pass) + 3);
	memset(upass, 0, strlen(user) + strlen(pass) + 3);
	snprintf(upass, strlen(pass) + strlen(user) + 3, "%s:%s", user, pass);

	repos = malloc(sizeof(struct list)); // push -> database - 1
	memset(repos, 0, sizeof(struct list));

	// rc = mdb_env_create(&env);
	// rc = mdb_env_open(env, "./testdb", MDB_MAPASYNC|MDB_WRITEMAP|MDB_MAPASYNC, 0664);

	// pthread_create // fetcher
	printf("fetching stars...\n");
	CURL *curl = curl_easy_init();
	if (curl) {
		get(curl, "https://api.github.com/graphql",
			"{\"query\": \"{ viewer { starredRepositories(first: 35) { nodes { name nameWithOwner sshUrl description } pageInfo { endCursor hasNextPage } } } } \"}",
			upass, repos);
		curl_easy_cleanup(curl);
	} else {
		curl_easy_cleanup(curl);
		exit(1);
	}
	// printf("  finished\n");
	printf("deduping orgs\n");
	dup = malloc(sizeof(char **) * list_len(repos));
	dedup_list(repos); // push -> database - 0
	// printf("finished\n");
	printf("mounting filesystem\n");
	//end fetcher

	argc = 3;
	argv[0] = "githubfs";
	argv[1] = "-f";
	argv[2] = "t/repos";

	return fuse_main(argc, argv, &fuse_fetcher_opts, NULL);
}
