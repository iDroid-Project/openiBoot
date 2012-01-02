/**
 * openiboot.h
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

#ifndef OPENIBOOT_H
#define OPENIBOOT_H

/**
 * @file
 *
 * This is the main include for OpeniBoot.
 *
 * A lot of functionality that is used frequently in this
 * program is defined here.
 *
 * @mainpage
 *
 * This is the documentation for OpeniBoot, which is generated
 * from the OpeniBoot source code.
 *
 * To read the documentation for the various parts of OpeniBoot,
 * please use the buttons above.
 */

#include <stddef.h> // size_t

typedef unsigned long long uint64_t;
typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;
typedef signed long long int64_t;
typedef signed int int32_t;
typedef signed short int16_t;
typedef signed char int8_t;
typedef signed int intptr_t;
typedef uint32_t fourcc_t;

#include "error.h"

#ifdef DEBUG
#define OPENIBOOT_VERSION_DEBUG " (DEBUG)"
#else
#define OPENIBOOT_VERSION_DEBUG ""
#endif

#ifndef OPENIBOOT_VERSION_CONFIG
#define OPENIBOOT_VERSION_CONFIG ""
#endif

#define XSTRINGIFY(s) STRINGIFY(s)
#define STRINGIFY(s) #s
#define OPENIBOOT_VERSION_STR "openiboot " XSTRINGIFY(OPENIBOOT_VERSION) " commit " XSTRINGIFY(OPENIBOOT_VERSION_BUILD) OPENIBOOT_VERSION_DEBUG OPENIBOOT_VERSION_CONFIG

#ifndef MIN
#define MIN(a, b) (((a) < (b))? (a): (b))
#endif

#ifndef MAX
#define MAX(a, b) (((a) > (b))? (a): (b))
#endif

#define FOURCC(a, b, c, d) (d || (c << 8) || (b << 16) || (a << 24))

extern void* _start;
extern void* OpenIBootEnd;
extern int received_file_size;

typedef enum Boolean {
	FALSE = 0,
	TRUE = 1
} Boolean;

typedef enum OnOff {
	OFF = 0,
	ON = 1
} OnOff;

#ifndef NULL
#define NULL 0
#endif
#define uSecPerSec 1000000

#define CONTAINER_OF(type, member, ptr) ((type*)(((char*)(ptr)) - ((char*)(&((type*)NULL)->member))))
#define ARRAY_SIZE(arr) (sizeof(arr)/sizeof((arr)[0]))
#define ROUND_UP(val, amt) (((val + amt - 1) / amt) * amt)
#define CEIL_DIVIDE(val, amt) (((val) + (amt) - 1) / (amt))

typedef struct Event Event;

typedef void (*TaskRoutineFunction)(void* opaque);
typedef void (*EventHandler)(Event* event, void* opaque);

typedef struct LinkedList {
	void* prev;
	void* next;
} __attribute__ ((packed)) LinkedList;

typedef struct TaskRegisterState {
	uint32_t	r4;
	uint32_t	r5;
	uint32_t	r6;
	uint32_t	r7;
	uint32_t	r8;
	uint32_t	r9;
	uint32_t	r10;
	uint32_t	r11;
	uint32_t	sp;
	uint32_t	lr;
} __attribute__ ((packed)) TaskRegisterState;

typedef enum TaskState {
	TASK_READY = 1,
	TASK_RUNNING = 2,
	TASK_SLEEPING = 4,
	TASK_STOPPED = 5
} TaskState;

#define TaskDescriptorIdentifier1 0x7461736b
#define TaskDescriptorIdentifier2 0x74736b32

struct Event {
	LinkedList	list;
	uint64_t	deadline;
	uint64_t	interval;
	EventHandler	handler;
	void*		opaque;
};

typedef struct TaskDescriptor {
	uint32_t		identifier1;
	LinkedList		taskList;
	LinkedList		runqueueList;
	TaskState		state;
	uint32_t		criticalSectionNestCount;
	TaskRegisterState	savedRegisters;
	Event			sleepEvent;
	LinkedList		linked_list_3;
	uint32_t		exitState;
	TaskRoutineFunction	taskRoutine;
	void*			unknown_passed_value;
	void*			storage;
	uint32_t		storageSize;
	char			taskName[16];
	uint32_t		identifier2;

	uint32_t		wasWoken;
} __attribute__ ((packed)) TaskDescriptor;

extern TaskDescriptor* CurrentRunning;

/*
 *	Macros
 */

typedef volatile uint32_t reg32_t;
typedef volatile uint16_t reg16_t;
typedef volatile uint8_t reg8_t;

#define GET_REGX(s, x)		(*((volatile reg##s##_t*)(x)))
#define SET_REGX(s, x, y)	(*((volatile reg##s##_t*)(x)) = (y))

#define GET_REG8(x)			GET_REGX(8, (x))
#define SET_REG8(x, y)		SET_REGX(8, (x), (y))

#define GET_REG16(x)		GET_REGX(16, (x))
#define SET_REG16(x, y)		SET_REGX(16, (x), (y))

#define GET_REG32(x)		GET_REGX(32, (x))
#define SET_REG32(x, y)		SET_REGX(32, (x), (y))

#define GET_REG				GET_REG32
#define SET_REG				SET_REG32

#define GET_BITS(x, start, length) ((((uint32_t)(x)) << (32 - ((start) + (length)))) >> (32 - (length)))

//
// Module support
//

typedef void (*initfn_t)(void);
typedef void (*exitfn_t)(void);
typedef void (*mainfn_t)(void);

#define BOOT_MODULE_INIT_SECTION	".init_modules_boot"
#define MODULE_INIT_SECTION			".init_modules"
#define MODULE_EXIT_SECTION			".exit_modules"

#define MODULE_INIT_BOOT(fn) initfn_t fn##_init __attribute__((section(BOOT_MODULE_INIT_SECTION))) = &fn;
#define MODULE_INIT(fn) initfn_t fn##_init __attribute__((section(MODULE_INIT_SECTION))) = &fn;
#define MODULE_EXIT(fn) exitfn_t fn##_exit __attribute__((section(MODULE_EXIT_SECTION))) = &fn;

void init_setup();
void init_boot_modules();
void init_modules();
void exit_modules();

void OpenIBootShutdown();
void OpenIBootConsole();
extern mainfn_t OpenIBootMain;

#endif
