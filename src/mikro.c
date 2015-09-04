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
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

void mikro_enable_reset() {
	ROM_GPIOPinTypeGPIOOutput(GPIO_PORTA_BASE, GPIO_PIN_5);
}

void mikro_enable_uart(uint32_t speed) {

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

void mikro_enable_interrupt(void (*handler)()) {
	ROM_IntMasterEnable();
	ROM_IntEnable(INT_UART1);
	ROM_UARTIntEnable(UART1_BASE, UART_INT_RX | UART_INT_RT);
	UARTIntRegister(UART1_BASE, handler);
	//ROM_UARTFIFOLevelSet(UART1_BASE, 0, UART_FIFO_RX1_8);
	//ROM_UARTFIFOEnable(UART1_BASE);
}

void mikro_reset() {
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, 0);
	SysCtlDelay(16000000/3/10); //100ms
	GPIOPinWrite(GPIO_PORTA_BASE, GPIO_PIN_5, GPIO_PIN_5);
}
