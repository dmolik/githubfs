#include "list.h"

#include <stdlib.h>
#include <string.h>

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
