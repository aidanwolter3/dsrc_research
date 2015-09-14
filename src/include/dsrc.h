#include "common.h"
#include "console.h"
#include "gps_l80_mikro.h"
#include "wifi_mcw1001a_mikro.h"
#include "task_manager.h"

typedef struct {
  char     name[16];
  uint32_t lat;
  uint32_t lon;
} DSRC_HEARTBEAT;

typedef struct {
  uint8_t         ip[4];
  DSRC_HEARTBEAT *hb;
  uint8_t         trust;
  uint8_t         timeout;
} DSRC_DEVICE;

void print_device_table();
void send_heartbeat();
void wifi_configure();
void wifi_start();
void switch_configure();

uint32_t device_color = RED_LED;
