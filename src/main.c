#define FUSE_USE_VERSION 26
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#include <fuse.h>
#include <lmdb.h>

#include "repo.h"
#include "fetch.h"
#include "fs.h"
#include "conf.h"

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
	for (p = tmp + 1; *p; p++)
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
	char *home = getenv("HOME");
	char *test = getenv("TEST");
	char *repos = strdup(argv[1]);
	char dbpath[256];
	char repopath[256];
	sprintf(dbpath, "%s/.config/gitfs/db", home);
	if (test != NULL) {
		sprintf(dbpath,   "%s/db", repos);
		sprintf(repopath, "%s/repos", repos);
	} else
		sprintf(repopath, "%s", repos);
	_mkdir(dbpath,   0750);
	_mkdir(repopath, 0750);


	char *user = getenv("GH_USER");
	char *pass = getenv("GH_TOKEN");

	char *upass = malloc(strlen(user) + strlen(pass) + 3);
	memset(upass, 0, strlen(user) + strlen(pass) + 3);
	snprintf(upass, strlen(pass) + strlen(user) + 3, "%s:%s", user, pass);

	int rc;
	MDB_env *env;
	if ((rc = mdb_env_create(&env)) != 0) {
		fprintf(stderr, "failed to create db env: (%d) %s\n", rc, mdb_strerror(rc));
		exit(EXIT_FAILURE);
	}
	if ((rc = mdb_env_set_mapsize(env, getpagesize() * 1024 )) != 0) {
		fprintf(stderr, "failed to set mmap size: (%d) %s\n", rc, mdb_strerror(rc));
		exit(EXIT_FAILURE);
	}
	if ((rc = mdb_env_set_maxdbs(env, 2)) != 0) {
		fprintf(stderr, "failed to add multiple named databases: (%d) %s\n", rc, mdb_strerror(rc));
		exit(EXIT_FAILURE);
	}
	if ((rc = mdb_env_open(env, dbpath, MDB_MAPASYNC|MDB_WRITEMAP|MDB_MAPASYNC, 0640)) != 0) {
		fprintf(stderr, "failed to open db env: (%d) %s\n", rc, mdb_strerror(rc));
		exit(EXIT_FAILURE);
	}

	// pthread_create // fetcher
	printf("fetching stars...\n");
	CURL *curl = curl_easy_init();
	if (curl) {
		get(curl, "https://api.github.com/graphql",
			"{\"query\": \"{ viewer { starredRepositories(first: 35) { nodes { name nameWithOwner sshUrl description } pageInfo { endCursor hasNextPage } } } } \"}",
			upass, env);
		curl_easy_cleanup(curl);
	} else {
		curl_easy_cleanup(curl);
		exit(1);
	}
	free(upass);

	gfsconf *gfs;
	gfs = malloc(sizeof(gfsconf));
	memset(gfs, 0, sizeof(gfsconf));
	if ((rc = mdb_txn_begin(env, NULL, 0, &gfs->txn)) != 0) {
		fprintf(stderr, "failed to get txn (%d) %s\n", rc, mdb_strerror(rc));
		return -1;
	}
	if ((rc = mdb_dbi_open(gfs->txn, "repos", MDB_CREATE, &gfs->dbir)) != 0) {
		fprintf(stderr, "failed to open repos dbi (%d) %s\n", rc, mdb_strerror(rc));
		return -1;
	}
	if ((rc = mdb_dbi_open(gfs->txn, "orgs", MDB_CREATE, &gfs->dbio)) != 0) {
		fprintf(stderr, "failed to open orgs dbi (%d) %s\n", rc, mdb_strerror(rc));
		return -1;
	}

	printf("mounting filesystem\n");
	if (test != NULL) {
		argc = 4;
		argv[0] = "githubfs";
		argv[1] = "-d";
		argv[2] = "-f";
		argv[3] = repopath;
	} else {
		argc = 3;
		argv[0] = "githubfs";
		argv[1] = "-f";
		argv[2] = repopath;
	}

	return fuse_main(argc, argv, &fuse_fetcher_opts, gfs);
}
