#ifndef _GIT_REPO_H
#define _GIT_REPO_H

#include <stdlib.h>

#include "list.h"

typedef struct repo {
	char *name;
	char *url;
	char *path[2];
	char *description;
} repo;

repo * new_repo(char *name, char *url, char *fullpath, char *description);

char * repo_string(repo *r);

void print_repos(FILE *fh, struct list *li);

#endif
