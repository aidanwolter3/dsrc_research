/*
 * task_manager.h
 *
 *  Created on: Jul 26, 2015
 *      Author: Aidan
 */

#ifndef TASK_MANAGER_H_
#define TASK_MANAGER_H_

#include "common.h"

//bits that indicate what tasks need to be triggered
typedef enum {
	TASK_EVENT_ANY = 0,
	TASK_EVENT_GPS,
	TASK_EVENT_WIFI,
  TASK_EVENT_WIFI_RECV_FROM,
	TASK_EVENT_TIMER0,
	TASK_EVENT_TIMER1,
	TASK_EVENT_END
} TASK_EVENT;

//set and get whether an event should be run
void task_set_event(TASK_EVENT e);
void task_clear_event(TASK_EVENT e);
void task_wait_for_event(TASK_EVENT e);
bool task_wait_for_event_wto(TASK_EVENT e, uint32_t ms);
uint8_t task_get_events();

//timers
#define TIMER_CNT 2
void task_start_timer0(TASK_EVENT e, uint32_t us);
void task_start_timer1(TASK_EVENT e, uint32_t us);

#endif //TASK_MANAGER_H_
