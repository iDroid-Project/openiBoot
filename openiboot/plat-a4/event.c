#include "openiboot.h"
#include "timer.h"
#include "event.h"
#include "clock.h"
#include "interrupt.h"
#include "hardware/timer.h"
#include "openiboot-asmhelpers.h"

Event EventList;

static void init_event_list();
static void eventTimerHandler();

int event_setup() {
    RTCHasInit = TRUE;
	init_event_list();

//  SET_REG(TIMER_REGISTER_TICK, TIMER_STATE_MANUALUPDATE);
    interrupt_install(TIMER_IRQ, eventTimerHandler, 0);
    interrupt_enable(TIMER_IRQ);
	return 0;
}

static void init_event_list() {
	EventList.list.next = &EventList;
	EventList.list.prev = &EventList;
}

static void eventTimerHandler() {
	uint64_t curTime;
	Event* event;

    SET_REG(TIMER_REGISTER_TICK, TIMER_STATE_MANUALUPDATE);

	curTime = timer_get_system_microtime();

	while(EventList.list.next != &EventList) {	// Just keep checking the first event on the list until the list is empty
							// This works because we remove events as we dispatch them

		event = EventList.list.next;

		if(event == NULL) {
			break;
		}

		if(curTime >= event->deadline) {
			// take it off the list and dispatch it
			((Event*)(event->list.next))->list.prev = event->list.prev;
			((Event*)(event->list.prev))->list.next = event->list.next;
			event->list.next = NULL;
			event->list.prev = NULL;
			event->handler(event, event->opaque);
		} else {
			// The event queue is sorted, so we can just stop looping on the first
			// event that hasn't been triggered
			break;
		}
	}
}

int event_add(Event* newEvent, uint64_t timeout, EventHandler handler, void* opaque) {
	EnterCriticalSection();

	// If this item is already on a list, take it off
	if(newEvent->list.prev != NULL || newEvent->list.next != NULL) {
		if(newEvent->list.next != NULL) {
			((Event*)(newEvent->list.next))->list.prev = newEvent->list.prev;
		}
		if(newEvent->list.prev != NULL) {
			((Event*)(newEvent->list.prev))->list.next = newEvent->list.next;
		}
		newEvent->list.next = NULL;
		newEvent->list.prev = NULL;
	}

	newEvent->handler = handler;
	newEvent->interval = timeout;
	newEvent->deadline = timer_get_system_microtime() + timeout;

	Event* insertAt = &EventList;

	while(insertAt != insertAt->list.next) {
		// Find the event that this occurs after
		if(insertAt->deadline > newEvent->deadline) {
			break;
		}
		insertAt = insertAt->list.next;
		if(insertAt == &EventList) {
			// We're after the whole list, so just insert after everyone else (after head)
			break;
		}
	}

	// Insert before insertAt
	newEvent->list.next = insertAt;
	newEvent->list.prev = insertAt->list.prev;
	((Event*)insertAt->list.prev)->list.next = newEvent;
	insertAt->list.prev = newEvent;

	LeaveCriticalSection();

	return 0;
}

int event_readd(Event* event, uint64_t new_interval) {
	EnterCriticalSection();

	// If this item is already on a list, take it off
	if(event->list.prev != NULL || event->list.next != NULL) {
		if(event->list.next != NULL) {
			((Event*)(event->list.next))->list.prev = event->list.prev;
		}
		if(event->list.prev != NULL) {
			((Event*)(event->list.prev))->list.next = event->list.next;
		}
		event->list.next = NULL;
		event->list.prev = NULL;
	}

	uint64_t interval;
	if(new_interval == 0) {
		if(event->interval == 0) {
			LeaveCriticalSection();
			return -1;
		} else {
			interval = event->interval;
		}
	} else {
		interval = new_interval;
	}

	event_add(event, interval, event->handler, event->opaque);
	LeaveCriticalSection();

	return 0;
}

