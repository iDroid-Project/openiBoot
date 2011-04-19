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

typedef enum _partitioning_mode
{
	partitioning_unknown,
	partitioning_mbr,
	partitioning_gpt,
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

typedef int (*block_device_get_attribute_t)(struct _block_device *);

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

	partitioning_mode_t part_mode;
	MBR mbr;
	GPT gpt;

	MBRPartitionRecord *mbr_records;
	GPTPartitionRecord *gpt_records;
} block_device_t;

typedef struct _block_device_handle_struct
{
	block_device_t *device;

	union
	{
		MBRPartitionRecord *mbr_record;
		GPTPartitionRecord *gpt_record;
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
