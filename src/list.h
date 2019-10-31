#ifndef _GIT_LIST_H
#define _GIT_LIST_H

#include <stdlib.h>

struct el {
	size_t  size;
	void   *data;
	void   *next;
};

struct list {
	size_t length;
	struct el *first;
	struct el *last;
};

struct el * new_el(void *data);

int add_el(struct list *li, struct el *elm);

int list_len(struct list *li);

#endif
