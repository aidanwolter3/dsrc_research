#ifndef DEVICE_TABLE_H_
#define DEVICE_TABLE_H_

#include "common.h"
#include "console.h"
#include "list.h"

#define DEVICE_TIMEOUT  200
#define MAX_NEIGHBORS   20

typedef struct {
  uint8_t         ip[4];
  uint32_t        last_hb_id;
  char            name[16];
  uint32_t        lat;
  uint32_t        lon;
  int8_t          self_trust;
  uint8_t         computed_trust;
  uint8_t         used_neighbors_cnt;
  uint32_t        used_neighbors[MAX_NEIGHBORS];
  uint8_t         timeout;
} DSRC_DEVICE;

void device_table_init(uint8_t size);
void* device_table_get(uint8_t *ip);
void device_table_put(DSRC_DEVICE *dev);
void device_table_update();
void device_table_print();

#endif
