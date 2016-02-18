#include "list.h"

void list_alloc(list *l, int size) {
  l->size = size;
  l->arr = (void*)malloc(size*sizeof(void*));
  for(int i = 0; i < size; i++) {
    l->arr[i] = NULL;
  }
}
void list_free(list *l) {
  free(l->arr);
}
bool list_full(list *l) {
  int index = 0;
  for(index = 0; index < l->size; index++) {
    if(l->arr[index] == NULL) {
      return false;
    }
  }
  return true;
}
void list_add(list *l, void *obj) {
  if(list_full(l)) {
    return;
  }
  int index = 0;
  for(index = 0; index < l->size; index++) {
    if(l->arr[index] == NULL) {
      break;
    }
  }
  l->arr[index] = obj;
}
void list_remove(list *l, void *obj) {
  int index = 0;
  for(index = 0; index < l->size; index++) {
    if(l->arr[index] == obj) {
      free(obj);
      l->arr[index] = NULL;
      break;
    }
  }
}


