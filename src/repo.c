#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "repo.h"


repo * new_repo(char *name, char *url, char *fullpath, char *description)
{
	repo *r;
	if ((r = malloc(sizeof(repo))) == NULL)
		return NULL;
	memset(r, 0, sizeof(repo));
	r->name = name;
	r->url  = url;
	r->description = description;

	char *path1  = malloc(strlen(fullpath) + 3);
	memset(path1, 0, strlen(fullpath) + 3);
	path1[0] = '/';
	path1[1] = '\0';
	strcat(path1, fullpath);

	char *path0 = strdup(path1);
	path0[strlen(path0) - strlen(rindex(path0, '/'))] = '\0';

	r->path[0] = strdup(path0);
	r->path[1] = strdup(path1);
	return r;
}

char * repo_string(repo *r)
{
	if (r == NULL)
		return NULL;

	size_t len = strlen(r->url) + strlen(r->description) + strlen("url: \ndescription: \n") + 4;
	char *str = malloc(len);
	memset(str, 0, len);
	snprintf(str, len, "URL: %s\nDescription: %s\n", r->url, r->description);
	return strdup(str);
}

void print_repos(FILE *fh, struct list *li)
{
	struct el *elm = li->first;
	repo *r;
	while (elm != NULL) {
		r = elm->data;
		fprintf(fh, "%s:%s\ndescription: %s\n", r->name, r->url, r->description);
		elm = elm->next;
	}
}
