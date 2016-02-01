/*
 * mikro.cpp
 *
 *  Created on: Jun 29, 2015
 *      Author: Aidan
 */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "mikro.h"
#include "console.h"

#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_gpio.h"
#include "inc/hw_types.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

void mikro_enable_reset(DEV dev) {
  if(dev == DEV1) {
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_5);
  }
  else if(dev == DEV2) {
    ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_6);
  }
}

void mikro_enable_uart(DEV dev, uint32_t speed) {

  if(dev == DEV1) {

    //gpio stuff
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOD);

    HWREG(GPIO_PORTD_BASE + GPIO_O_LOCK) = GPIO_LOCK_KEY;
    HWREG(GPIO_PORTD_BASE + GPIO_O_CR) = 0x80;

    SysCtlDelay(3);
    ROM_GPIOPinConfigure(GPIO_PD6_U2RX);
    ROM_GPIOPinConfigure(GPIO_PD7_U2TX);

    //uart stuff
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART2);
    SysCtlDelay(3);
    ROM_GPIOPinTypeUART(GPIO_PORTD_BASE, GPIO_PIN_6 | GPIO_PIN_7);
    ROM_UARTConfigSetExpClk(UART2_BASE, SysCtlClockGet(), speed,
                    (UART_CONFIG_WLEN_8   |
                     UART_CONFIG_STOP_ONE |
                     UART_CONFIG_PAR_NONE));
  }

  else if(dev == DEV2) {

    //gpio stuff
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOB);

    SysCtlDelay(3);
    ROM_GPIOPinConfigure(GPIO_PB0_U1RX);
    ROM_GPIOPinConfigure(GPIO_PB1_U1TX);

    //uart stuff
    ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART1);
    SysCtlDelay(3);
    ROM_GPIOPinTypeUART(GPIO_PORTB_BASE, GPIO_PIN_0 | GPIO_PIN_1);
    ROM_UARTConfigSetExpClk(UART1_BASE, SysCtlClockGet(), speed,
                    (UART_CONFIG_WLEN_8   |
                     UART_CONFIG_STOP_ONE |
                     UART_CONFIG_PAR_NONE));
  }
}

void mikro_enable_interrupt(DEV dev, void (*handler)()) {
  ROM_IntMasterEnable();

  if(dev == DEV1) {
    ROM_IntEnable(INT_UART2);
    ROM_UARTIntEnable(UART2_BASE, UART_INT_RX | UART_INT_RT);
    UARTIntRegister(UART2_BASE, handler);
  }

  else if(dev == DEV2) {
    ROM_IntEnable(INT_UART1);
    ROM_UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);
    UARTIntRegister(UART1_BASE, handler);
  }
}

void mikro_reset(DEV dev) {
  if(dev == DEV1) {
    GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, 0);
    SysCtlDelay(16000000/3/10); //100ms
    GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_PIN_5);
  }
  else if(dev == DEV2) {
    GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_6, 0);
    SysCtlDelay(16000000/3/10); //100ms
    GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_6, GPIO_PIN_6);
  }
}
