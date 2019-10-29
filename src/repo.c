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

repo * new_repo(char *name, char *url)
{
	repo *r;
	if ((r = malloc(sizeof(repo))) == NULL)
		return NULL;
	memset(r, 0, sizeof(repo));
	r->name = name;
	r->url  = url;
	char *path = malloc(strlen(name) + 3);
	memset(path, 0, strlen(name) + 3);
	path[0] = '/';
	path[1] = '\0';
	strcat(path, name);
	r->path = strdup(path);
	return r;
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
