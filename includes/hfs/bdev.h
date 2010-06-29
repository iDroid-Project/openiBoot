#ifndef BDEV_H
#define BDEV_H

#include "openiboot.h"
#include "hfs/common.h"

typedef struct MBRPartitionRecord {
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
} MBRPartitionRecord;

typedef struct MBR {
	uint8_t code[0x1B8];
	uint32_t signature;
	uint16_t padding;
	MBRPartitionRecord partitions[4];
	uint16_t magic;
} __attribute__ ((packed)) MBR;

typedef struct GPTPartitionRecord {
	uint64_t type[2];
	uint64_t guid[2];
	uint64_t beginLBA;
	uint64_t endLBA;
	uint64_t attribute;
	uint8_t partitionName[0x48];
} GPTPartitionRecord;

typedef struct GPT {
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

extern int HasBDevInit;

int bdev_setup();
unsigned int bdev_get_start(int partition);
io_func* bdev_open(int partition);

#endif
