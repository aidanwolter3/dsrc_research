#ifndef COMMON_H_
#define COMMON_H_

//standard C includes
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//Tivaware includes
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_ints.h"
#include "inc/hw_gpio.h"
#include "driverlib/debug.h"
#include "driverlib/fpu.h"
#include "driverlib/gpio.h"
#include "driverlib/pin_map.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/uart.h"
#include "driverlib/timer.h"
#include "utils/uartstdio.h"

//constants
#define WIFI_PRESENT	true
#define GPS_PRESENT		true
#define SHOW_WIFI_PACKETS false
#define SHOW_WIFI_TX true
#define SHOW_WIFI_RX true

#define GREEN_DEVICE 0x1
#define BLUE_DEVICE 0x2
#ifdef SELECT_GREEN_DEVICE
  #define DEVICE GREEN_DEVICE
#endif
#ifdef SELECT_BLUE_DEVICE
  #define DEVICE BLUE_DEVICE
#endif

#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3

//the tolerance allowed for the stated angle and the calculated angle out of 360 degrees
#define GPS_TOLERANCE_360 4

#define M_PI        3.14159265358979323846264338327950288   /* pi             */

//macros
#define bitset(b) (1 << (b))
#define bitclear(b) (~(1 << (b)))
#define isbitset(v,b) (((v & bitset(b)) != 0) ? true : false)
#define isbitclear(v,b) (((v | ~bitset(b)) == 0) ? true : false)

//procedures


//modified strtok that takes into account empty strings
//courtesy of http://www.tek-tips.com/viewthread.cfm?qid=294161
char* xstrtok(char *line, char *delims);

#endif //COMMON_H_
