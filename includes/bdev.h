/**
 *	@file 
 *
 *  Header file for OiB's Block Device system.
 *
 *  This allows easy access to devices attached to OiB and contains structures 
 *  for MBR, GPT and LwVM (Light Weight Volume Manager (Apple TV and iOS 5+))
 */

/*
 * bdev.h
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


#ifndef  BDEV_H
#define  BDEV_H

#include "openiboot.h"

typedef struct _MBRPartitionRecord {
	uint8_t status;
	uint8_t beginHead;
	uint8_t beginSectorCyl;
	uint8_t beginCyl;
	uint8_t type;
	uint8_t endHead;
	uint8_t endSectorCyl;
	uint8_t endCyl;
	uint32_t beginLBA;
	uint32_t numSectors;
} __attribute__ ((packed)) MBRPartitionRecord;

typedef struct _MBR {
	uint8_t code[0x1B8];
	uint32_t signature;
	uint16_t padding;
	MBRPartitionRecord partitions[4];
	uint16_t magic;
} __attribute__ ((packed)) MBR;

typedef struct _GPTPartitionRecord {
	uint64_t type[2];
	uint64_t guid[2];
	uint64_t beginLBA;
	uint64_t endLBA;
	uint64_t attribute;
	uint8_t partitionName[0x48];
} __attribute__ ((packed)) GPTPartitionRecord;

typedef struct _GPT {
	uint64_t signature;
	uint32_t revision;
	uint32_t headerSize;
	uint32_t headerChecksum;
	uint32_t reserved;
	uint64_t currentLBA;
	uint64_t backupLBA;
	uint64_t firstLBA;
	uint64_t lastLBA;
	uint64_t guid[2];
	uint64_t partitionEntriesFirstLBA;
	uint32_t numPartitions;
	uint32_t partitionEntrySize;
	uint32_t partitionArrayChecksum;
} __attribute__ ((packed)) GPT;

typedef struct _LwVMPartitionRecord {
	uint64_t type[2];
	uint64_t guid[2];
	uint64_t begin;
	uint64_t end;
	uint64_t attribute; // 0 == unencrypted; 0x1000000000000 == encrypted
	char	partitionName[0x48];
} __attribute__ ((packed)) LwVMPartitionRecord;

typedef struct _LwVM {
	uint64_t type[2];
	uint64_t guid[2];
	uint64_t mediaSize;
	uint32_t numPartitions;
	uint32_t crc32;
	uint8_t unkn[464];
	LwVMPartitionRecord partitions[12];
	uint16_t chunks[1024]; // chunks[0] should be 0xF000
} __attribute__ ((packed)) LwVM;

static const char LwVMType[] = { 0x6A, 0x90, 0x88, 0xCF, 0x8A, 0xFD, 0x63, 0x0A, 0xE3, 0x51, 0xE2, 0x48, 0x87, 0xE0, 0xB9, 0x8B };
static const char LwVMType_noCRC[] = { 0xB1, 0x89, 0xA5, 0x19, 0x4F, 0x59, 0x4B, 0x1D, 0xAD, 0x44, 0x1E, 0x12, 0x7A, 0xAF, 0x45, 0x39 };
uint16_t** LwVM_chunks;
uint32_t LwVM_numValidChunks;
uint32_t LwVM_rangeShiftValue;
uint32_t LwVM_rangeByteCount;
uint64_t LwVM_seek;

typedef enum _partitioning_mode
{
	partitioning_unknown,
	partitioning_mbr,
	partitioning_gpt,
	partitioning_lwvm,
	partitioning_none,
} partitioning_mode_t;

typedef enum _seek_mode
{
	seek_begin,
	seek_end,
	seek_offset,
} seek_mode_t;

struct _block_device;
typedef error_t (*block_device_prepare_t)(struct _block_device *);
typedef void (*block_device_finish_t)(struct _block_device *);

typedef error_t (*block_device_read_t)(struct _block_device *, void *_dest, int _sz);
typedef error_t (*block_device_write_t)(struct _block_device *, void *_src, int _sz);
typedef error_t (*block_device_seek_t)(struct _block_device *, seek_mode_t _mode, int64_t _amt);
typedef error_t (*block_device_sync_t)(struct _block_device *);

typedef int64_t (*block_device_get_attribute_t)(struct _block_device *);

typedef struct _block_device
{
	block_device_prepare_t prepare;
	block_device_finish_t finish;

	block_device_read_t read;
	block_device_write_t write;
	block_device_seek_t seek;
	block_device_sync_t sync;

	block_device_get_attribute_t size;
	block_device_get_attribute_t block_size;

	int setup_done;
	LinkedList list_ptr;

	struct _block_device_handle_struct *handle;

	partitioning_mode_t part_mode;
	MBR mbr;
	GPT gpt;
	LwVM lwvm;

	MBRPartitionRecord *mbr_records;
	GPTPartitionRecord *gpt_records;
	LwVMPartitionRecord *lwvm_records;
} block_device_t;

typedef struct _block_device_handle_struct
{
	block_device_t *device;
	int pIdx;

	union
	{
		MBRPartitionRecord *mbr_record;
		GPTPartitionRecord *gpt_record;
		LwVMPartitionRecord *lwvm_record;
	};
	
} block_device_handle_struct_t, *block_device_handle_t;
typedef block_device_handle_t bdev_handle_t;

// For drivers
error_t block_device_init(block_device_t *_bdev);
error_t block_device_register(block_device_t *_bdev);
void block_device_unregister(block_device_t *_bdev);

// For clients
block_device_t *block_device_find(block_device_t *_last);

int block_device_size(block_device_t *_bdev);
int block_device_block_size(block_device_t *_bdev);

int block_device_partition_count(block_device_t *_bdev);
block_device_handle_t block_device_open(block_device_t *_bdev, int _idx);
void block_device_close(block_device_handle_t);

int block_device_get_start(block_device_handle_t _handle);

error_t block_device_read(block_device_handle_t, void *_dest, int _sz);
error_t block_device_write(block_device_handle_t, void *_src, int _sz);
error_t block_device_seek(block_device_handle_t, seek_mode_t _mode, int64_t _amt);
error_t block_device_sync(block_device_handle_t);

#endif //BDEV_H
