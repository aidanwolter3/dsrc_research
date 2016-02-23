#ifndef DSRC_H_
#define DSRC_H_

#include "common.h"
#include "console.h"
#include "gps_l80_mikro.h"
#include "wifi_mcw1001a_mikro.h"
#include "task_manager.h"
#include "math.h"
#include "device_table.h"

#define MAX_TRACKED_DEVICES 20

typedef struct {
  uint32_t  id;
  uint32_t  hb_packet_id;
  uint8_t   ip[4];
  int8_t    trust;
} DSRC_DEVICE_TRUST;

typedef struct {
  uint32_t  id;
  uint8_t   ip[4];
  int8_t    computed_trust;
} DSRC_DEVICE_TRUST_REPORT;

typedef struct {
  uint32_t   id;
  uint32_t   packet_id;
  char       name[16];
  uint32_t   lat;
  uint32_t   lon;
} DSRC_HEARTBEAT;

void send_device_trust(DSRC_DEVICE *dev, uint32_t hb_packet_id);
void send_heartbeat();
void wifi_configure();
void wifi_start();
void switch_configure();

uint32_t device_color = RED_LED;

#endif
