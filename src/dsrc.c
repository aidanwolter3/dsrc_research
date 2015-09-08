/*
 * main.c
 *
 *  Created on: Jun 29, 2015
 *      Author: Aidan
 */

#include "common.h"
#include "console.h"
#include "gps_l80_mikro.h"
#include "wifi_mcw1001a_mikro.h"
#include "task_manager.h"

//*****************************************************************************
//
// The error routine that is called if the driver library encounters an error.
//
//*****************************************************************************
#ifdef DEBUG
void __error__(char *pcFilename, uint32_t ui32Line) {
}
#endif

void wifi_configure();
void wifi_start();
void switch_configure();

//determines what device it is
uint32_t device_color = RED_LED;

//wifi configurations
WIFI_NETWORK_MODE network_mode = WIFI_NETWORK_MODE_ADHOC;
uint8_t channels[] = {1,2,3,4,5,6,7,8,9,10,11};
uint8_t ip[16] = {0xA9, 0xFE, 0x01, 0x01};
uint8_t netmask[16] = {0xFF, 0xFF, 0xFF, 0x00};
uint8_t gateway[16] = {0xA9, 0xFE, 0x01, 0x01};
uint8_t mac[6] = {0x22, 0x33, 0x44, 0x55, 0x66, 0x00};
uint16_t arp_time = 1;
uint16_t local_port = 10002;
uint8_t socket_handle = 0xFF;

void port_f_handler() {
  if(device_color == RED_LED) {
    uint32_t sw1_status = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_4);
    uint32_t sw2_status = GPIOPinRead(GPIO_PORTF_BASE, GPIO_PIN_0);
    if(sw1_status > 0) {
      con_println("Configure device: BLUE"); //TRANSMITTER
      device_color = BLUE_LED;
      ip[3] = 0x02;
      mac[5] = 0x22;
      task_set_event(TASK_EVENT_WIFI_START);
    }
    if(sw2_status > 0) {
      con_println("Configure device: GREEN"); //RECEIVER
      device_color = GREEN_LED;
      ip[3] = 0x03;
      mac[5] = 0x33;
      task_set_event(TASK_EVENT_WIFI_START);
    }
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
  con_println("INITIALIZING DEVICE");

  //initialize the led
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED);

#if WIFI_PRESENT
  con_println("INIT WIFI");
  wifi_init();

  //disconnect if connected
  WIFI_PACKET_NETWORK_STATUS network_status = wifi_get_network_status();
  if(network_status.status == WIFI_NETWORK_STATUS_CONNECTED_STATIC_IP ||
     network_status.status == WIFI_NETWORK_STATUS_CONNECTED_DHCP) {
    wifi_disconnect();
  }
#endif

#if GPS_PRESENT
  gps_l80_mikro_init();
#endif

  //save the led state
  bool led_on = false;

  //start the periodic timer
  task_start_timer0(TASK_EVENT_TIMER0, 1000000);

  while(1) {
    task_wait_for_event_wto(TASK_EVENT_ANY, 100);
    uint8_t events = task_get_events();

    if(isbitset(events, TASK_EVENT_GPS) == true) {
      gps_parse_buffer();
      task_clear_event(TASK_EVENT_GPS);
    }
    if(isbitset(events, TASK_EVENT_WIFI) == true) {
      wifi_parse_buffer();
      task_clear_event(TASK_EVENT_WIFI);
    }
    if(isbitset(events, TASK_EVENT_TIMER0) == true) {

      #if WIFI_PRESENT
      if(device_color == BLUE_LED) { //TRANSMITTER
        if(socket_handle != 0xFF) {
          uint16_t remote_port = 10002;
          uint8_t remote_ip[16] = {0xA9, 0xFE, 0x01, 0x03};
          uint16_t len = 4;
          uint8_t data[] = {'t', 'e', 's', 't'};
          //WIFI_PACKET_SOCKET_SEND_TO_RESPONSE send_response;
          wifi_socket_send_to(socket_handle, remote_port, remote_ip, len, data);
        }
      }
      else if(device_color == GREEN_LED) { //RECEIVER
        if(socket_handle != 0xFF) {
          //WIFI_PACKET_SOCKET_RECV_FROM_RESPONSE recv_response;
          wifi_socket_recv_from(socket_handle, 100);
        }
      }
      #endif

      #if GPS_PRESENT
      uint32_t lat;
      uint32_t lon;
      NMEA_STATUS ret = NMEA_VAL_OK;
      ret |= gps_l80_get_latitude(&lat);
      ret |= gps_l80_get_longitude(&lon);
      if(ret == NMEA_VAL_INVALID) {
        con_println("Error retrieving GPS location");
      }
      else {
        char str[256];
        sprintf(str, "lat: %lu, lon: %lu", lat, lon);
        con_println(str);
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
    if(isbitset(events, TASK_EVENT_WIFI_START) == true) {
      wifi_configure();
      wifi_start();

      task_clear_event(TASK_EVENT_WIFI_START);
    }
  }
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
  wifi_set_list_retry_count(5, 1);

  wifi_get_network_status();

#endif
}

//connect to the connection profile and bind to the socket
void wifi_start() {
#if WIFI_PRESENT
  con_println("CONNECTING WIFI");

  wifi_connect(WIFI_CP1);

  //start udp socket connection
  socket_handle = wifi_socket_create(WIFI_SOCKET_TYPE_UDP);
  wifi_socket_bind(local_port, socket_handle);

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
