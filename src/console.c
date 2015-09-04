/*
 * console.c
 *
 *  Created on: Jun 29, 2015
 *      Author: Aidan
 */

#include "common.h"
#include "console.h"

//initialize the console for printing out via UART to putty or hyperterminal
void con_initialize() {
	static int initialized = 0;

	//only initialize the console once
	if(initialized == 0) {

		//enable the UART port
		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_UART0);
		ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);

		//set GPIO A0 and A1 as UART pins
		GPIOPinConfigure(GPIO_PA0_U0RX);
		GPIOPinConfigure(GPIO_PA1_U0TX);
		ROM_GPIOPinTypeUART(GPIO_PORTA_BASE, GPIO_PIN_0 | GPIO_PIN_1);

		//configure the UART for 115,200, 8-N-1 operation.
		ROM_UARTConfigSetExpClk(UART0_BASE, ROM_SysCtlClockGet(), 115200,
								(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
								 UART_CONFIG_PAR_NONE));
		initialized = 1;
	}
}

//clear the console contents and place the cursor in the upper left corner
void con_clear() {
	ROM_UARTCharPut(UART0_BASE, 0x0C);
}

//print a string out to the console starting from the current cursor location
void con_printf(char *str) {
	int i;
	for(i = 0; i < strlen(str); i++) {
		ROM_UARTCharPut(UART0_BASE, str[i]);
	}
}

//print a string out to the console starting from the current cursor location
//only print the specified amount of characters
void con_nprintf(char *str, int size) {
	int i;
	for(i = 0; i < size; i++) {
		ROM_UARTCharPut(UART0_BASE, str[i]);
	}
}

//print out a string and a newline
//only print the specified amount of characters
void con_nprintln(char *str, int size) {
	con_nprintf(str, size);
	con_printf("\r\n");
}

//print out a string and a newline
void con_println(char *str) {
	con_printf(str);
	con_printf("\r\n");
}

void con_putchar(char c) {
	ROM_UARTCharPut(UART0_BASE, c);
}
