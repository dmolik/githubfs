#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "repo.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

repo new_repo(char *name, char *url, char *fullpath, char *description)
{
	repo r = { 0 };
	size_t len = MIN(strlen(name) + 1, sizeof(r.name) - 1);
	memcpy(r.name, name, len);
	r.name[len] = '\0';

	len = MIN(strlen(url) + 1, sizeof(r.url) - 1);
	memcpy(r.url, url, len);
	r.url[len] = '\0';

	len = MIN(strlen(description) + 1, sizeof(r.description) - 1);
	memcpy(r.description, description, len);
	r.description[len] = '\0';

	char *path1  = malloc(strlen(fullpath) + 2);
	memset(path1, 0, strlen(fullpath) + 2);
	path1[0] = '/';
	path1[1] = '\0';
	strcat(path1, fullpath);

	char *path0 = strdup(path1);
	path0[strlen(path0) - strlen(rindex(path0, '/'))] = '\0';

	len = MIN(strlen(path0) + 1, sizeof(r.path[0]));
	memcpy(r.path[0], path0, len);
	r.path[0][len - 1] = '\0';
	len = MIN(strlen(path1) + 1, sizeof(r.path[1]));
	memcpy(r.path[1], path1, len);
	r.path[1][len - 1] = '\0';
	free(path0);
	free(path1);
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
