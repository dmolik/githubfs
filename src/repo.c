#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "repo.h"


struct el * new_el(void *data)
{
	struct el *elm;
	if ((elm = malloc(sizeof(struct el))) == NULL)
		return NULL;
	memset(elm, 0, sizeof(struct el));
	elm->data = data;
	elm->next = NULL;
	return elm;
}

int add_el(struct list *li, struct el *elm)
{
	if (li->first == NULL)
		li->first = elm;
	else
		li->last->next = elm;
	li->last = elm;
	li->length++;
	return 0;
}

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

	char *str = malloc(strlen(r->url) + strlen(r->description) + strlen("url: \ndescription: \n") + 4);
	memset(str, 0, strlen(r->url) + strlen(r->description) + strlen("url: \ndescription: \n") + 4);
	snprintf(str, strlen(r->url) + strlen(r->description) + strlen("url: \ndescription: \n") + 4,
		"URL: %s\nDescription: %s\n", r->url, r->description);
	return strdup(str);
}

int list_len(struct list *li)
{
	struct el *elm = li->first;
	int size = 1;
	while (elm != NULL) {
		size++;
		elm = elm->next;
	}
	return size;
}
void print_repos(struct list *li)
{
	struct el *elm = li->first;
	repo *r;
	while (elm != NULL) {
		r = elm->data;
		printf("%s:%s\n", r->name, r->url);
		elm = elm->next;
	}
}
