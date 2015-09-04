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

int main(void)
{

	//enable lazy stacking so floating-point instruction can be used in interrupt handlers
	ROM_FPULazyStackingEnable();

	//set the system clock to 16MHZ
	ROM_SysCtlClockSet(SYSCTL_SYSDIV_1 | SYSCTL_USE_OSC | SYSCTL_XTAL_16MHZ | SYSCTL_OSC_MAIN);

//	//set up the button SW1
//	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);        // Enable port F
//	GPIOPinTypeGPIOInput(GPIO_PORTF_BASE, GPIO_PIN_4);  // Init PF4 as input
//	GPIOPadConfigSet(GPIO_PORTF_BASE, GPIO_PIN_4,
//		GPIO_STRENGTH_2MA, GPIO_PIN_TYPE_STD_WPU);  // Enable weak pullup resistor for PF4
//
//	// Interrupt setup
//	GPIOIntDisable(GPIO_PORTF_BASE, GPIO_PIN_4);        // Disable interrupt for PF4 (in case it was enabled)
//	GPIOIntClear(GPIO_PORTF_BASE, GPIO_PIN_4);      // Clear pending interrupts for PF4
//	GPIOIntRegister(GPIO_PORTF_BASE, onButtonDown);     // Register our handler function for port F
//	GPIOIntTypeSet(GPIO_PORTF_BASE, GPIO_PIN_4,
//		GPIO_FALLING_EDGE);             // Configure PF4 for falling edge trigger
//	GPIOIntEnable(GPIO_PORTF_BASE, GPIO_PIN_4);     // Enable interrupt for PF4

  con_initialize();
  con_clear();

  //initialize the led
  SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOF);
  GPIOPinTypeGPIOOutput(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED);

#if WIFI_PRESENT
	wifi_init();
	con_println("WIFI started");

	//disconnect if connected
	WIFI_PACKET_NETWORK_STATUS network_status = wifi_get_network_status();
	if(network_status.status == WIFI_NETWORK_STATUS_CONNECTED_STATIC_IP ||
	   network_status.status == WIFI_NETWORK_STATUS_CONNECTED_DHCP) {
		wifi_disconnect();
	}

	//initialize adhoc, channels, security, ssid, ip, netmask, gateway, mac, arp, and retries
	wifi_configure();
	con_println("after configure");
	wifi_get_network_status();

	//start udp socket connection
	uint16_t local_port = 1110;
	uint8_t socket_handle;

	wifi_connect(WIFI_CP1);
	con_println("after connect");
	socket_handle = wifi_socket_create(WIFI_SOCKET_TYPE_UDP);
	wifi_socket_bind(local_port, socket_handle);

	//check current status
	wifi_get_network_status();
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
        GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, BLUE_LED);
        led_on = true;
      }
      else {
        GPIOPinWrite(GPIO_PORTF_BASE, RED_LED|BLUE_LED|GREEN_LED, 0);
        led_on = false;
      }

			task_clear_event(TASK_EVENT_TIMER0);
		}
	}

	
}

//configure adhoc, channels, security, ssid, ip, netmask, gateway, mac, arp, and retries
void wifi_configure() {

	//initialize the configurations
	WIFI_NETWORK_MODE network_mode = WIFI_NETWORK_MODE_ADHOC;
	uint8_t channels[] = {1,2,3,4,5,6,7,8,9,10,11};
	uint8_t ip[16] = {0xA9, 0xFE, 0x01, 0x03};
	uint8_t netmask[16] = {0xFF, 0xFF, 0xFF, 0x00};
	uint8_t gateway[16] = {0xA9, 0xFE, 0x01, 0x01};
	uint8_t mac[6] = {0x22, 0x33, 0x44, 0x55, 0x66, 0x88};
	uint16_t arp_time = 1;

	//send the configurations
	wifi_set_cp_network_mode(WIFI_CP1, network_mode);
	wifi_set_channel_list(channels, 11);
	wifi_set_cp_security_open(WIFI_CP1);
	wifi_set_cp_ssid(WIFI_CP1, "vanet_server");
	wifi_set_ip_address(WIFI_IP_CONFIG_STATIC, ip);
	wifi_set_netmask(netmask);
	wifi_set_gateway(gateway);
	wifi_set_mac(mac);
	wifi_set_arp_time(arp_time);
}

