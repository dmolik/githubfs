
#ifndef _GITFS_FETCH_H
#define _GITFS_FETCH_H

#include <curl/curl.h>
#include <lmdb.h>

#include "repo.h"


int get(CURL *curl, char *url, char *post, char *upass, MDB_env *env);

#endif
