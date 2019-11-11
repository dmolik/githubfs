#ifndef _GIT_REPO_H
#define _GIT_REPO_H

#include <stdlib.h>

typedef struct repo {
	char name[64];
	char url[64];
	char path[2][64];
	char description[256];
} repo;

repo new_repo(char *name, char *url, char *fullpath, char *description);

char * repo_string(repo *r);

#endif
