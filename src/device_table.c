#include "device_table.h"

list device_table;

void device_table_init(uint8_t size) {
  list_alloc(&device_table, size);
}

void* device_table_get(uint8_t *ip) {
  int index = 0;
  for(index = 0; index < device_table.size; index++) {
    if(memcmp((DSRC_DEVICE*)(device_table.arr[index]), ip, 4) == 0) {
      break;
    }
  }
  if(index >= device_table.size) {
    return NULL;
  }
  return device_table.arr[index];
}

void device_table_put(DSRC_DEVICE *dev) {
  list_add(&device_table, dev);
}

void device_table_update() {
  //check all timeouts
}

void device_table_print() {
  con_println("DEVICE TABLE:");
  char str[256];
  for(int i = 0; i < device_table.size; i++) {
    if(device_table.arr[i] != NULL) {
      sprintf(str, "%s: %lu, %lu - self_trust: %d", ((DSRC_DEVICE*)device_table.arr[i])->name,
          ((DSRC_DEVICE*)device_table.arr[i])->lat,
          ((DSRC_DEVICE*)device_table.arr[i])->lon,
          ((DSRC_DEVICE*)device_table.arr[i])->self_trust);
      con_println(str);
    }
  }
  con_printf("\n");
}
