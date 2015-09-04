/*
 * mikro.h
 *
 *  Created on: Jun 29, 2015
 *      Author: Aidan
 */

#ifndef MIKRO_H_
#define MIKRO_H_

//initialization and startup functions
void mikro_initialize();
void mikro_enable_reset();
void mikro_enable_uart(uint32_t speed);
void mikro_enable_interrupt(void (*handler)());
void mikro_reset();

#endif /* MIKRO_H_ */
