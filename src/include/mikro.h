/*
 * mikro.h
 *
 *  Created on: Jun 29, 2015
 *      Author: Aidan
 */

#ifndef MIKRO_H_
#define MIKRO_H_

typedef enum {
  DEV1, // UART2 and PA5 reset
  DEV2  // UART1 and PA6 reset
} DEV;

//initialization and startup functions
void mikro_enable_reset(DEV dev);
void mikro_enable_uart(DEV dev, uint32_t speed);
void mikro_enable_interrupt(DEV dev, void (*handler)());
void mikro_reset(DEV dev);


#endif /* MIKRO_H_ */
