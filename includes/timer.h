/*
 * timer.h
 *
 * Copyright 2011 iDroid Project
 *
 * This file is part of iDroid. An android distribution for Apple products.
 * For more information, please visit http://www.idroidproject.org/.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef TIMER_H
#define TIMER_H

#include "openiboot.h"
#include "hardware/timer.h"

typedef void (*TimerHandler)(void);

typedef struct TimerRegisters {
	uint32_t	config;
	uint32_t	state;
	uint32_t	count_buffer;
	uint32_t	count_buffer2;
	uint32_t	prescaler;
	uint32_t	cur_count;
} TimerRegisters;

typedef struct TimerInfo {
	Boolean		option6;
	uint32_t	divider;
	uint32_t	unknown1;
	TimerHandler	handler1;
	TimerHandler	handler2;
	TimerHandler	handler3;
} TimerInfo;

extern const TimerRegisters HWTimers[];
extern TimerInfo Timers[NUM_TIMERS];

int timer_setup();
int timer_stop_all();
int timer_on_off(int timer_id, OnOff on_off);
int timer_setup_clk(int timer_id, int type, int divider, uint32_t unknown1);
int timer_init(int timer_id, uint32_t interval, uint32_t interval2, uint32_t prescaler, uint32_t z, Boolean option24, Boolean option28, Boolean option11, Boolean option5, Boolean interrupts);

uint64_t timer_get_system_microtime();
void timer_get_rtc_ticks(uint64_t* ticks, uint64_t* sec_divisor);

void udelay(uint64_t delay);
int has_elapsed(uint64_t startTime, uint64_t elapsedTime);

extern int RTCHasInit;

#endif

