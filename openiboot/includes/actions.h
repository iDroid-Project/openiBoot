/**
 * actions.h
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
 *	@file Header file for the 'actions' OiB can perform, this includes image loading, setting the boot entry, kernel loading, ramdisk loading and the chainloading of iBoot.
 *
 */
#ifndef ACTIONS_H
#define ACTIONS_H

#include "openiboot.h"

typedef enum _BootType
{
	kBootAuto,
	kBootLinux,
	kBootImage,
	kBootChainload,
} BootType;

typedef struct _BootEntry
{
	BootType type;
	char *title;
	char *path; // Use for linux kernel args and image name
	char *kernel;
	char *ramdisk;
	uint32_t machine; // Used for linux kernels.

	LinkedList list_ptr;
} BootEntry;

void chainload(uint32_t address);
int chainload_image(char *name);
void set_kernel(void* location, int size);
void set_ramdisk(void* location, int size);
void boot_linux(const char* args, uint32_t mach_id);

BootEntry *setup_root();
BootEntry *setup_current();
BootEntry *setup_default();

void setup_title(const char *title);
void setup_entry(BootEntry *entry);
void setup_auto();
void setup_kernel(const char *kernel, const char *args);
void setup_initrd(const char *initrd);
void setup_image(const char *image);
int setup_boot();

#endif
