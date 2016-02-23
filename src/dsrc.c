/*
 * main.c
 *
 *  Created on: Jun 29, 2015
 *      Author: Aidan
 */

#include "dsrc.h"

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) {
}
#endif

//control system parameters
#define SELF_WEIGHTED_TRUST 14
#define OTHERS_WEIGHTED_TRUST 10
#define OTHERS_MIN_TRUST 2
#define MINIMUM_ACC_TRUST 30

//wifi configurations
WIFI_NETWORK_MODE network_mode = WIFI_NETWORK_MODE_ADHOC;
uint8_t channels[]    = {1,2,3,4,5,6,7,8,9,10,11};
uint8_t ip[16]        = {0xA9, 0xFE, 0x01, DEVICE};
uint8_t remote_ip[16] = {0xFF, 0xFF, 0xFF, 0xFF}; // broadcast to all
uint8_t netmask[16]   = {0xFF, 0xFF, 0x00, 0x00};
uint8_t gateway[16]   = {0xA9, 0xFE, 0x00, 0x01};
uint8_t mac[6]        = {0x22, 0x33, 0x44, 0x55, 0x66, DEVICE};
uint16_t arp_time     = 5;

uint16_t rx_local_port    = 10002;
uint16_t tx_local_port    = 10003;
uint16_t rx_remote_port   = 10003;
uint16_t tx_remote_port   = 10002;
uint8_t  rx_socket_handle = 0xFF;
uint8_t  tx_socket_handle = 0xFF;

//keep track of the last known self location
uint32_t latitude = 0;
uint32_t longitude = 0;

//interrupt handler for switches
void port_f_handler() {
  uint32_t sw1_status = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4);
  uint32_t sw2_status = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0);
  if(sw1_status > 0) {
  }
  if(sw2_status > 0) {
  }
  GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);
}

//keep track of how many seconds have passed for the heartbeat packet id
uint8_t  hz_count = 0;
uint32_t pck_count = 0;

int main(void) {

  //enable lazy stacking so floating-point instruction can be used in interrupt handlers
  ROM_FPULazyStackingEnable();

  //set the system clock to 16MHZ
  ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

  switch_configure();

  con_initialize();
  con_clear();
  con_println("\n---------------------------------------------------");
  con_printf("INITIALIZING DEVICE: ");
  if(DEVICE == GREEN_DEVICE) {
    con_println("Green");
  }
  else if(DEVICE == BLUE_DEVICE) {
    con_println("Blue");
  }
  else if(DEVICE == RED_DEVICE) {
    con_println("Red");
  }
  else if(DEVICE == WHITE_DEVICE) {
    con_println("White");
  }

  //initialize the device table
  device_table_init(MAX_TRACKED_DEVICES);

  //first determine who we are (GREEN or BLUE)
  if(DEVICE == GREEN_DEVICE) {
    device_color = GREEN_LED;
  }
  else if(DEVICE == BLUE_DEVICE) {
    device_color = BLUE_LED;
  }
  else if(DEVICE == RED_DEVICE) {
    device_color = RED_LED;
  }
  else if(DEVICE == WHITE_DEVICE) {
    device_color = WHITE_LED;
  }

  //initialize the led
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED);

#if WIFI_PRESENT
  con_println("INITIALIZING WIFI");
  wifi_init();

  //disconnect if connected
  WIFI_PACKET_NETWORK_STATUS network_status = wifi_get_network_status();
  if(network_status.status == WIFI_NETWORK_STATUS_CONNECTED_STATIC_IP ||
     network_status.status == WIFI_NETWORK_STATUS_CONNECTED_DHCP) {
    wifi_disconnect();
  }

  wifi_configure();
  wifi_start();
#endif

#if GPS_PRESENT
  gps_l80_mikro_init();
#endif

  //save the led state
  bool led_on = false;

  //start the periodic timers
  task_start_timer0(TASK_EVENT_TIMER0, 1000000);
  task_start_timer1(TASK_EVENT_TIMER1, 1000);

  while(1) {
    task_wait_for_event_wto(TASK_EVENT_ANY, 100);
    uint8_t events = task_get_events();

    //parse the incoming gps buffer
    if(isbitset(events, TASK_EVENT_GPS) == true) {
      gps_parse_buffer();
      task_clear_event(TASK_EVENT_GPS);
    }

    //parse the incoming wifi buffer
    if(isbitset(events, TASK_EVENT_WIFI) == true) {
      wifi_parse_buffer();
      task_clear_event(TASK_EVENT_WIFI);
    }

    //parse the incoming RECV_FROM packets
    if(isbitset(events, TASK_EVENT_WIFI_RECV_FROM) == true) {

      //iterate over every packet
      WIFI_PACKET_SOCKET_RECV_FROM_RESPONSE *recv_from = malloc(sizeof(WIFI_PACKET_SOCKET_RECV_FROM_RESPONSE));
      while(wifi_get_recv_from_packet(recv_from) == true) {

        //only modify the device table if we know our current location
        if(latitude != 0 && longitude != 0) {

          if(recv_from->data[0] == 0xAA) {

            //determine if the device already exists in the table and allocate space if it doesn't
            DSRC_DEVICE *dev = device_table_get(recv_from->remote_ip);

            if(dev == NULL) {
              dev = (DSRC_DEVICE*)malloc(sizeof(DSRC_DEVICE));
              device_table_put(dev);
            }

            //check if the packet was already received
            DSRC_HEARTBEAT* hb = (DSRC_HEARTBEAT*)(recv_from->data);
            if(dev->last_hb_id != hb->packet_id) {

              //copy the device into the table
              memcpy(dev->ip, recv_from->remote_ip, 4*sizeof(uint8_t));
              memcpy(dev->name, hb->name, sizeof(dev->name));
              dev->last_hb_id = hb->packet_id;
              dev->lat = hb->lat;
              dev->lon = hb->lon;
              dev->timeout = DEVICE_TIMEOUT;

              //TESTING
              char str[256];
              sprintf(str, "got hb: %s", dev->name);
              con_println(str);

              //clear the used neighbors
              dev->used_neighbors_cnt = 0;
              memset(dev->used_neighbors, 0, sizeof(dev->used_neighbors));

              int16_t their_lat = ((uint32_t)dev->lat) % 10000;
              int16_t their_lon = ((uint32_t)dev->lon) % 10000;
              int16_t my_lat = latitude % 10000;
              int16_t my_lon = longitude % 10000;

              //compute angle and put in packet
              int16_t angle = atan((their_lat-my_lat)/(their_lon-my_lon))*180/M_PI;
              if(their_lon < my_lon) {
                angle += 180;
              }
              else if(their_lat < my_lat) {
                angle += 360;
              }

              //determine if the angle is close to the actual
              if(abs(angle - recv_from->dir) < GPS_TOLERANCE_360) {
                dev->self_trust = SELF_WEIGHTED_TRUST;
                dev->computed_trust = SELF_WEIGHTED_TRUST;
              }
              else {
                dev->self_trust = 0;
                dev->computed_trust = 0;
              }
              if(DEVICE != BLUE_DEVICE) {
                delay_ms(100*DEVICE);
                send_device_trust(dev, hb->packet_id);
              }
            }
          }

          //received a device trust packet
          else if(DEVICE == BLUE_DEVICE && recv_from->data[0] == 0xBB) {
            DSRC_DEVICE_TRUST *trust = (DSRC_DEVICE_TRUST*)(recv_from->data);
            
            //ignore the recommendation if they don't trust the device
            if(trust->trust != 0) {
              //DSRC_DEVICE *recommender = device_table_get(recv_from->remote_ip);
              DSRC_DEVICE *dev = device_table_get(trust->ip);

              //only continue if the packet id matches
              if(trust->hb_packet_id == dev->last_hb_id) {

                //only use the opinions of others if we know about the device they are recommending
                //and the device that is recommending
                //if(recommender != NULL) {
                  if(dev != NULL) {

                    //see if we have already used the opinion of this neighbor
                    //bool neighbor_already_used = false;
                    //for(int i = 0; i < dev->used_neighbors_cnt; i++) {
                    //  if(dev->used_neighbors[i] == 0) {
                    //    break;
                    //  }
                    //  if(memcmp(&(dev->used_neighbors[i]), recommender->ip, sizeof(trust->ip) == 0)) {
                    //    neighbor_already_used = true;
                    //    break;
                    //  }
                    //}

                    //if(!neighbor_already_used) {
                      char str[256];
                      sprintf(str, "got trust %s->%s", /*recommender->name*/"device", dev->name);
                      con_println(str);

                      uint8_t weighted_opinion = min(/*recommender->computed_trust*/MINIMUM_ACC_TRUST / MINIMUM_ACC_TRUST + OTHERS_MIN_TRUST, 1) * OTHERS_WEIGHTED_TRUST;
                      dev->computed_trust += weighted_opinion;
                      //memcpy(&(dev->used_neighbors[dev->used_neighbors_cnt]), recommender->ip, sizeof(trust->ip));
                      //dev->used_neighbors_cnt++;

                      //report the new computed trust
                      //DSRC_DEVICE_TRUST_REPORT tr;
                      //tr.id = 0xCC;
                      //memcpy(tr.ip, dev->ip, sizeof(tr.ip));
                      //tr.computed_trust = dev->computed_trust;
                      //wifi_socket_send_to(tx_socket_handle, tx_remote_port, remote_ip, sizeof(DSRC_DEVICE_TRUST_REPORT), (uint8_t*)&tr);
                    //}
                  }
                //}
              }
            }
          }
        }

        //free the packet
        free(recv_from->data);
      }
      free(recv_from);

      #if WIFI_PRESENT

      //receive socket is ready
      if(rx_socket_handle != 0xFF) {
        wifi_socket_recv_from(rx_socket_handle, 2);
      }
      #endif

      task_clear_event(TASK_EVENT_WIFI_RECV_FROM);
    }

    //Timer for blinking the status LED and sending heartbeats
    if(isbitset(events, TASK_EVENT_TIMER0) == true) {
      char str[3];
      sprintf(str, "%d", hz_count);
      con_println(str);
      if(hz_count == 0) {

        //only send a heartbeat every 10 interrupts
        pck_count = (pck_count + 1) % 60; // for some reason if the count goes up too high, we don't receive any more heartbeats

        //decrement the timeouts in the device table
        device_table_update();

        #if WIFI_PRESENT

        //transmit socket is ready
        if(tx_socket_handle != 0xFF) {

          //if we know our current location
          if(latitude != 0 && longitude != 0) {
            send_heartbeat();
          }
        }
        #endif

        #if GPS_PRESENT
        NMEA_STATUS ret = NMEA_VAL_OK;
        ret |= gps_l80_get_latitude(&latitude);
        ret |= gps_l80_get_longitude(&longitude);
        if(ret == NMEA_VAL_INVALID) {
          //con_println("Error retrieving GPS location");
        }
        else {
          //char str[256];
          //sprintf(str, "lat: %lu, lon: %lu", latitude, longitude);
          //con_println(str);
        }
        #endif
      }
      hz_count = (hz_count + 1) % 5;

      //toggle the led to indicate the board hasn't frozen
      if(led_on == false) {
        GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, device_color);
        led_on = true;
      }
      else {
        GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, 0);
        led_on = false;
      }

      task_clear_event(TASK_EVENT_TIMER0);
    }

    //Timer for sending recv_from packets
    if(isbitset(events, TASK_EVENT_TIMER1) == true) {

      #if WIFI_PRESENT

      //receive socket is ready
      if(rx_socket_handle != 0xFF) {
        wifi_socket_recv_from(rx_socket_handle, 2);
      }
      #endif

      task_clear_event(TASK_EVENT_TIMER1);
    }
  }
}

void send_device_trust(DSRC_DEVICE *dev, uint32_t hb_packet_id) {
  char str[256];
  sprintf(str, "send trust: %s", dev->name);
  con_println(str);

  DSRC_DEVICE_TRUST *dt = (DSRC_DEVICE_TRUST*)malloc(sizeof(DSRC_DEVICE_TRUST));
  memset(dt, 0, sizeof(DSRC_DEVICE_TRUST));
  dt->hb_packet_id = hb_packet_id;
  dt->id = 0xBB;
  memcpy(dt->ip, dev->ip, sizeof(dt->ip));
  dt->trust = dev->self_trust;

  wifi_socket_send_to(tx_socket_handle, tx_remote_port, remote_ip, sizeof(DSRC_DEVICE_TRUST), (uint8_t*)dt);

  free(dt);
}

void send_heartbeat() {
  if(DEVICE == WHITE_DEVICE) {
  con_println("");

  //send different data depending on the device selected
  DSRC_HEARTBEAT *hb = malloc(sizeof(DSRC_HEARTBEAT));
  memset(hb, 0, sizeof(DSRC_HEARTBEAT));
  hb->packet_id = pck_count;
  hb->id = 0xAA;
  hb->lat = latitude;
  hb->lon = longitude;
  if(DEVICE == GREEN_DEVICE) {
    strcpy(hb->name, "green");
    con_println("send hb: green");
  }
  else if(DEVICE == BLUE_DEVICE) {
    strcpy(hb->name, "blue");
    con_println("send hb: blue");
  }
  else if(DEVICE == RED_DEVICE) {
    strcpy(hb->name, "red");
    con_println("send hb: red");
  }
  else if(DEVICE == WHITE_DEVICE) {
    strcpy(hb->name, "white");
    con_println("send hb: white");
  }

  wifi_socket_send_to(tx_socket_handle, tx_remote_port, remote_ip, sizeof(DSRC_HEARTBEAT), (uint8_t*)hb);

  //clean up
  free(hb);
  }

  device_table_print();
}

//configure adhoc, channels, security, ssid, ip, netmask, gateway, mac, arp, and retries
void wifi_configure() {
#if WIFI_PRESENT
  con_println("CONFIGURING WIFI");

  wifi_set_cp_network_mode(WIFI_CP1, network_mode);
  wifi_set_channel_list(channels, 11);
  wifi_set_cp_security_open(WIFI_CP1);
  wifi_set_cp_ssid(WIFI_CP1, "vanet_server");
  wifi_set_ip_address(WIFI_IP_CONFIG_STATIC, ip);
  wifi_set_netmask(netmask);
  wifi_set_gateway(gateway);
  wifi_set_mac(mac);
  wifi_set_arp_time(arp_time);
  wifi_set_list_retry_count(255, 5);
#endif
}

//connect to the connection profile and bind to the socket
void wifi_start() {
#if WIFI_PRESENT
  con_println("CONNECTING WIFI");

  wifi_connect(WIFI_CP1);

  //start udp socket connection
  rx_socket_handle = wifi_socket_create(WIFI_SOCKET_TYPE_UDP);
  tx_socket_handle = wifi_socket_create(WIFI_SOCKET_TYPE_UDP);
  wifi_socket_bind(rx_local_port, rx_socket_handle);
  wifi_socket_bind(tx_local_port, tx_socket_handle);

  wifi_print_network_status();
#endif
}

//configure the switch 2 for interrupts
void switch_configure() {
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);

  //unlock PF0
  HWREG(GPIO_PORTF_BASE+GPIO_O_LOCK) = GPIO_LOCK_KEY;
  HWREG(GPIO_PORTF_BASE+GPIO_O_CR) |= 1;

  //set up the button SW2
  GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);
  GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4, GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);

  // Interrupt setup
  GPIOIntDisable(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);
  GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);
  GPIOIntRegister(GPIO_PORTF_BASE, port_f_handler);
  GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4, GPIO_FALLING_EDGE);

  GPIOIntEnable(GPIO_PORTF_BASE, GPIO_PIN_0|GPIO_PIN_4);
}
