/**
 *	@file 
 *
 *  Header file for OpeniBoot's NVRAM interface.
 *
 *  The NVRAM (Non-volatile Random Access Memory) interface is designed to 
 *  represent the reading, writing and setup of the NVRAM.
 *
 *  This provides a way of retrieving the values of environment variables 
 *  stored in the NVRAM on the device.
 *
 *  @defgroup NVRAM
 */

/**
 * nvram.h
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

#ifndef NVRAM_H
#define NVRAM_H

#include "openiboot.h"

/**
 *	Start value of NVRAM
 *
 *  @ingroup NVRAM
 */
#define NVRAM_START 0xFC000

/**
 *	Size of NVRAM
 *
 *  @ingroup NVRAM
 */
#define NVRAM_SIZE 0x2000

typedef struct NVRamInfo {
	uint8_t ckByteSeed;
	uint8_t ckByte;
	uint16_t size;
	char type[12];
} __attribute__ ((packed)) NVRamInfo;

typedef struct NVRamData {
	uint32_t adler;
	uint32_t epoch;
} NVRamData;

typedef struct NVRamAtom {
	NVRamInfo* info;
	uint32_t size;
	struct NVRamAtom* next;
	uint8_t* data;
} NVRamAtom;

/**
 *	Environment variable structure
 *
 *  The information returned for each variable retrieved from NVRAM
 *
 *	@param name Name of Variable
 *
 *	@param value Value held by variable
 *
 *  @ingroup NVRAM
 */
typedef struct EnvironmentVar {
	char* name;
	char* value;
	struct EnvironmentVar* next;
} EnvironmentVar;

/**
 * Setup the NVRAM to be read or written to, called by nor_init in nor.c
 *
 *  Returns 0 on successful loading of both NVRAM banks and -1 if the 
 *  device cannot be found or one of the banks cannot be loaded.
 *
 *  @ingroup NVRAM
 */
error_t nvram_setup();

/**
 *	List the variables stored in NVRAM
 *
 *  @ingroup NVRAM
 */
void nvram_listvars();

/**
 *	Get a variable stored in NVRAM
 *
 *  Returns the value of a variable retrieved from NVRAM to the buffer.
 *
 *  Returns variable on success, returns NULL on failure.
 *
 *	@param name Title of variable stored in NVRAM
 *
 *  @ingroup NVRAM
 */
const char* nvram_getvar(const char* name);

/**
 *	Set a variable in NVRAM
 *
 *	@param name Name of title in NVRAM
 *
 *	@param value Value to be written into NVRAM
 *
 *  @ingroup NVRAM
 */
void nvram_setvar(const char* name, const char* value);

/**
 *	Save any temprorary variables to NVRAM
 *
 *  Write the edited values of NVRAM from the newest bank to the oldest
 *  bank, and setting up the oldest bank to become newest bank.
 *
 *  @ingroup NVRAM
 */
void nvram_save();

#endif
