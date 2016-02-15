/*
 * main.c
 *
 *  Created on: Jun 29, 2015
 *      Author: Aidan
 */

#include "dsrc.h"
#include "math.h"

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) {
}
#endif

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

//table to keep track of every device and their location
DSRC_DEVICE device_table[MAX_TRACKED_DEVICES];
uint8_t device_table_size = 0;

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

  //first determine who we are (GREEN or BLUE)
  if(DEVICE == GREEN_DEVICE) {
    device_color = GREEN_LED;
  }
  else if(DEVICE == BLUE_DEVICE) {
    device_color = BLUE_LED;
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
  task_start_timer1(TASK_EVENT_TIMER1, 10000);

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

            //determine the index to place the heartbeat into the device table
            uint8_t index;
            for(index = 0; index < device_table_size; index++) {
              if(memcmp(recv_from->remote_ip, device_table[index].ip, 4*sizeof(uint8_t)) == 0) {
                break;
              }
            }

            //copy the heartbeat into the table
            if(index >= device_table_size) {
              device_table[index].hb = malloc(sizeof(DSRC_HEARTBEAT));
            }
            memcpy(device_table[index].ip, recv_from->remote_ip, 4*sizeof(uint8_t));
            memcpy(device_table[index].hb, recv_from->data, sizeof(DSRC_HEARTBEAT));
            device_table[index].timeout = 0;

            //update the table size
            if(index+1 > device_table_size) {
              device_table_size = index+1;
            }

            int16_t their_lat = ((uint32_t)device_table[index].hb->lat) % 100;
            int16_t their_lon = ((uint32_t)device_table[index].hb->lon) % 100;
            int16_t my_lat = latitude % 100;
            int16_t my_lon = longitude % 100;

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
              if(device_table[index].trust < 5) {
                device_table[index].trust++;
              }
            }
            else {
              if(device_table[index].trust > -5) {
                device_table[index].trust--;
              }
            }
            send_device_trust(index);
          }

          //received a device trust packet
          else if(recv_from->data[0] == 0xBB) {
          }


          print_device_table();
        }

        //free the packet
        free(recv_from->data);
      }
      free(recv_from);

      task_clear_event(TASK_EVENT_WIFI_RECV_FROM);
    }

    //1Hz timer
    if(isbitset(events, TASK_EVENT_TIMER0) == true) {

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

    //100Hz timer
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

void print_device_table() {
  con_println("DEVICE TABLE:");
  char str[256];
  for(int i = 0; i < device_table_size; i++) {
    sprintf(str, "%s: %lu, %lu - trust: %d", device_table[i].hb->name,
        device_table[i].hb->lat,
        device_table[i].hb->lon,
        device_table[i].trust);
    con_println(str);
  }
  con_printf("\n");
}

void send_device_trust(uint8_t device_index) {
  DSRC_DEVICE_TRUST dt;
  memset(&dt, 0, sizeof(DSRC_DEVICE_TRUST));
  dt.id = 0xBB;
  memcpy(dt.ip, device_table[device_index].ip, sizeof(dt.ip));
  dt.trust = device_table[device_index].trust;

  wifi_socket_send_to(tx_socket_handle, tx_remote_port, remote_ip, sizeof(DSRC_DEVICE_TRUST), (uint8_t*)&dt);
}

void send_heartbeat() {

  //send different data depending on the device selected
  DSRC_HEARTBEAT *hb = malloc(sizeof(DSRC_HEARTBEAT));
  memset(hb, 0, sizeof(DSRC_HEARTBEAT));
  hb->id = 0xAA;
  hb->lat = latitude;
  hb->lon = longitude;
  if(DEVICE == GREEN_DEVICE) {
    strcpy(hb->name, "green");
  }
  else {
    strcpy(hb->name, "blue");
  }

  wifi_socket_send_to(tx_socket_handle, tx_remote_port, remote_ip, sizeof(DSRC_HEARTBEAT), (uint8_t*)hb);

  //clean up
  free(hb);
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
