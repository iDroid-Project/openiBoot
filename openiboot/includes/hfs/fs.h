/**
 * hfs/fs.h
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

#ifndef FS_H
#define FS_H

#include "openiboot.h"
#include "hfs/hfsplus.h"

typedef struct ExtentListItem {
	uint32_t startBlock;
	uint32_t blockCount;
} ExtentListItem;

typedef struct ExtentList {
	uint32_t numExtents;
	ExtentListItem extents[16];
} ExtentList;

extern int HasFSInit;

uint32_t readHFSFile(HFSPlusCatalogFile* file, uint8_t** buffer, Volume* volume);

int fs_setup();
int add_hfs(Volume* volume, uint8_t* buffer, size_t size, const char* outFileName);
ExtentList* fs_get_extents(int device, int partition, const char* fileName);
int fs_extract(int device, int partition, const char* file, void* location);

#endif
