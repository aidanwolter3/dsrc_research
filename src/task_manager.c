/*
 * task_manager.c
 *
 *  Created on: Jul 26, 2015
 *      Author: Aidan
 */

#include "task_manager.h"
#include "console.h"

//indicates the dirty tasks to be triggered
//size should be updated depending on the number of events available
volatile static uint8_t task_events = 0;

//indicates which event to set on a timer interrupt
TASK_EVENT timer_events[TIMER_CNT] = {TASK_EVENT_END};

//timer 0
void timer0_handler() {
	ROM_TimerIntClear(TIMER0_BASE, TIMER_A);
	task_set_event(timer_events[0]);
}
void task_start_timer0(TASK_EVENT e, uint32_t us) {
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER0);
	ROM_TimerConfigure(TIMER0_BASE, TIMER_CFG_PERIODIC);
	ROM_TimerLoadSet(TIMER0_BASE, TIMER_A, 16*us);
	TimerIntRegister(TIMER0_BASE, TIMER_A, timer0_handler);
	ROM_IntEnable(INT_TIMER0A);
	ROM_TimerIntEnable(TIMER0_BASE, TIMER_TIMA_TIMEOUT);
	ROM_TimerEnable(TIMER0_BASE, TIMER_A);

	timer_events[0] = TASK_EVENT_TIMER0;
}

//timer 1
void timer1_handler() {
	ROM_TimerIntClear(TIMER1_BASE, TIMER_A);
	task_set_event(timer_events[1]);
}
void task_start_timer1(TASK_EVENT e, uint32_t us) {
	ROM_SysCtlPeripheralEnable(SYSCTL_PERIPH_TIMER1);
	ROM_TimerConfigure(TIMER1_BASE, TIMER_CFG_PERIODIC);
	ROM_TimerLoadSet(TIMER1_BASE, TIMER_A, 16*us);
	TimerIntRegister(TIMER1_BASE, TIMER_A, timer1_handler);
	ROM_IntEnable(INT_TIMER1A);
	ROM_TimerIntEnable(TIMER1_BASE, TIMER_TIMA_TIMEOUT);
	ROM_TimerEnable(TIMER1_BASE, TIMER_A);

	timer_events[1] = TASK_EVENT_TIMER1;
}

//set an event to be triggered
void task_set_event(TASK_EVENT e) {
	task_events |= bitset(e);
	task_events |= bitset(TASK_EVENT_ANY);
}

//clear an event from being triggered
void task_clear_event(TASK_EVENT e) {
	task_events &= ~bitset(e);
	if(task_events <= 1) {
		task_events = 0;
	}
}

void task_wait_for_event(TASK_EVENT e) {
	while(isbitset(task_events, e) == true);
}

bool task_wait_for_event_wto(TASK_EVENT e, uint32_t ms) {
	while((isbitset(task_events, e) == false) && (ms > 0)) {
		ms--;
		SysCtlDelay(16000/3);
	}
	if(ms == 0) {
		return false;
	}
	return true;
}

//get the events to be triggered
uint8_t task_get_events() {
	return task_events;
}
