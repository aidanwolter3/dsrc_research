#ifndef QUEUE_H_
#define QUEUE_H_

#include <stdlib.h>

typedef struct {
  int size;
  void **arr;

  int st;
  int en;
  int cnt;

} queue;

void queue_alloc(queue *q, int size) {
  q->size = size;
  q->arr = (void*)malloc(size*sizeof(void*));
  q->st = 0;
  q->en = 0;
  q->cnt = 0;
}
void queue_free(queue *q) {
  free(q->arr);
}
bool queue_empty(queue *q) {
  return q->cnt == 0;
}
bool queue_full(queue *q) {
  return q->cnt == q->size;
}
void queue_push(queue *q, void *obj) {
  if(queue_full(q)) {
    return;
  }
  if(!queue_empty(q)) {
    q->en = (q->en + 1) % q->size;
  }
  q->arr[q->en] = obj;
  q->cnt++;
}
void* queue_pop(queue *q) {
  if(queue_empty(q)) {
    return NULL;
  }
  void *obj = q->arr[q->st];
  q->cnt--;
  if(!queue_empty(q)) {
    q->st = (q->st + 1) % q->size;
  }
  return obj;
}

#endif
