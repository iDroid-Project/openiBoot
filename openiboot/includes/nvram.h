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

/**
 *	@file nvram.h
 *
 *	@brief Header file for NVRAM, including reading, writing and setup.
 *
 */

#ifndef NVRAM_H
#define NVRAM_H

#include "openiboot.h"
/**
 *	Start value of NVRAM
 */
#define NVRAM_START 0xFC000
/**
 *	Size of NVRAM
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
 *	@param name Name of Variable
 *
 *	@param value Value held by variable
 *
 */
typedef struct EnvironmentVar {
	char* name;
	char* value;
	struct EnvironmentVar* next;
} EnvironmentVar;
/**
 * Setup the NVRAM to be read or written to, called by nor.c
 *
 */
int nvram_setup();

/**
 *	List the variables stored in NVRAM
 *
 */
void nvram_listvars();

/**
 *	Get a variable stored in NVRAM
 *
 *	@param name Title of variable stored in NVRAM
 *
 */
const char* nvram_getvar(const char* name);

/**
 *	Set a variable in NVRAM
 *
 *	@param name Name of title in NVRAM
 *
 *	@param value Value to be written into NVRAM
 *
 */
void nvram_setvar(const char* name, const char* value);

/**
 *	Save any temprorary variables to NVRAM
 *
 */
void nvram_save();

#endif
