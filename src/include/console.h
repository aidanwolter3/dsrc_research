/*
 * console.h
 *
 *  Created on: Jun 29, 2015
 *      Author: Aidan
 */

#ifndef CONSOLE_H_
#define CONSOLE_H_

void con_initialize();
void con_clear();
void con_printf(char *str);
void con_nprintf(char *str, int size);
void con_println(char *str);
void con_nprintln(char *str, int size);
void con_putchar(char c);

#endif /* CONSOLE_H_ */
