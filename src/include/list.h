#ifndef LIST_H_
#define LIST_H_

#include <stdlib.h>
#include <stdbool.h>

typedef struct {
  int size;
  void **arr;
} list;

void list_alloc(list *l, int size);
void list_free(list *l);
bool list_full(list *l);
void list_add(list *l, void *obj);
void list_remove(list *l, void *obj);

#endif
