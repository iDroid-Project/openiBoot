/**
 * hfs/hfscompress.h
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

#ifndef HFSCOMPRESS_H
#define HFSCOMPRESS_H

#include "common.h"

#define CMPFS_MAGIC 0x636D7066

typedef struct HFSPlusDecmpfs {
	uint32_t magic;
	uint32_t flags;
	uint64_t size;
	uint8_t data[0];
} __attribute__ ((packed)) HFSPlusDecmpfs;

typedef struct HFSPlusCmpfRsrcHead {
	uint32_t headerSize;
	uint32_t totalSize;
	uint32_t dataSize;
	uint32_t flags;
} __attribute__ ((packed)) HFSPlusCmpfRsrcHead;

typedef struct HFSPlusCmpfRsrcBlock {
	uint32_t offset;
	uint32_t size;
} __attribute__ ((packed)) HFSPlusCmpfRsrcBlock;

typedef struct HFSPlusCmpfRsrcBlockHead {
	uint32_t dataSize;
	uint32_t numBlocks;
	HFSPlusCmpfRsrcBlock blocks[0];
} __attribute__ ((packed)) HFSPlusCmpfRsrcBlockHead;

typedef struct HFSPlusCmpfEnd {
	uint32_t pad[6];
	uint16_t unk1;
	uint16_t unk2;
	uint16_t unk3;
	uint32_t magic;
	uint32_t flags;
	uint64_t size;
	uint32_t unk4;
} __attribute__ ((packed)) HFSPlusCmpfEnd;

typedef struct HFSPlusCompressed {
	Volume* volume;
	HFSPlusCatalogFile* file;
	io_func* io;
	size_t decmpfsSize;
	HFSPlusDecmpfs* decmpfs;

	HFSPlusCmpfRsrcHead rsrcHead;
	HFSPlusCmpfRsrcBlockHead* blocks;

	int dirty;

	uint8_t* cached;
	uint32_t cachedStart;
	uint32_t cachedEnd;
} HFSPlusCompressed;

#ifdef __cplusplus
extern "C" {
#endif
	void flipHFSPlusDecmpfs(HFSPlusDecmpfs* compressData);
	io_func* openHFSPlusCompressed(Volume* volume, HFSPlusCatalogFile* file);
#ifdef __cplusplus
}
#endif

#endif
