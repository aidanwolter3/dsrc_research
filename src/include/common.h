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
#define SHOW_WIFI_TX false
#define SHOW_WIFI_RX false

//delays to accommodate for receiving UDP hardware limitations
#define TX_REPEAT_CNT 3
#define TX_REPEAT_DELAY 100

//selecting the device
#define BLUE_DEVICE 0x1
#define GREEN_DEVICE 0x2
#define RED_DEVICE 0x3
#define WHITE_DEVICE 0x4
#ifdef SELECT_GREEN_DEVICE
  #define DEVICE GREEN_DEVICE
#endif
#ifdef SELECT_BLUE_DEVICE
  #define DEVICE BLUE_DEVICE
#endif
#ifdef SELECT_RED_DEVICE
  #define DEVICE RED_DEVICE
#endif
#ifdef SELECT_WHITE_DEVICE
  #define DEVICE WHITE_DEVICE
#endif


//led colors
#define RED_LED   GPIO_PIN_1
#define BLUE_LED  GPIO_PIN_2
#define GREEN_LED GPIO_PIN_3
#define WHITE_LED GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_3

//the tolerance allowed for the stated angle and the calculated angle out of 360 degrees
#define GPS_TOLERANCE_360 4

#define M_PI        3.14159265358979323846264338327950288   /* pi             */

//macros
#define bitset(b) (1 << (b))
#define bitclear(b) (~(1 << (b)))
#define isbitset(v,b) (((v & bitset(b)) != 0) ? true : false)
#define isbitclear(v,b) (((v | ~bitset(b)) == 0) ? true : false)
#define min(a,b) ((a) < (b) ? (a) : (b))

//procedures

void delay_us(uint32_t us);
void delay_ms(uint32_t ms);
void delay_s(uint32_t s);

//modified strtok that takes into account empty strings
//courtesy of http://www.tek-tips.com/viewthread.cfm?qid=294161
char* xstrtok(char *line, char *delims);

#endif //COMMON_H_
