#include "common.h"
#include "console.h"
#include "gps_l80_mikro.h"
#include "wifi_mcw1001a_mikro.h"
#include "task_manager.h"

typedef struct {
  char     name[16];
  uint32_t latitude;
  uint32_t longitude;
} DSRC_HEARTBEAT;

void send_heartbeat();
void wifi_configure();
void wifi_start();
void switch_configure();

uint32_t device_color = RED_LED;
