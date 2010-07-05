#include "openiboot.h"
#include "hfs/common.h"
#include "hfs/bdev.h"
#include "ftl.h"
#include "util.h"
#include "nand.h"

int HasBDevInit = FALSE;
int useGPT = FALSE;

static MBR MBRData;
static GPT GPTData;
static GPTPartitionRecord GPTRecord[128];

unsigned int BLOCK_SIZE = 0;

int bdev_setup() {
	if(HasBDevInit)
		return 0;

	ftl_setup();

	NANDData* Data = nand_get_geometry();
	BLOCK_SIZE = Data->bytesPerPage;

	ftl_read(&MBRData, 0, sizeof(MBRData));
	MBRPartitionRecord* record = MBRData.partitions;

	int id = 0;
	while(record->type != 0) {
		if (record->type == 0xee)
			useGPT = TRUE;
		bufferPrintf("bdev: partition id: %d, type: %x, sectors: %d - %d\r\n", id, record->type, record->beginLBA, record->beginLBA + record->numSectors);
		record++;
		id++;
	}

	if (useGPT) {
		ftl_read(&GPTData, BLOCK_SIZE, sizeof(GPTData));

		uint32_t headerChecksum = GPTData.headerChecksum;
		uint32_t checksum = 0;
		GPTData.headerChecksum = 0;
		crc32(&checksum, &GPTData, sizeof(GPTData));

		if (checksum != headerChecksum) {
			bufferPrintf("bdev: gpt checksum wrong. switching back to mbr only\r\n");
			useGPT = FALSE;
		} else {
			bufferPrintf("bdev: guid partition table detected\r\n");
			ftl_read(&GPTRecord, GPTData.partitionEntriesFirstLBA * BLOCK_SIZE, sizeof(GPTPartitionRecord) * (GPTData.numPartitions > 128 ? 128 : GPTData.numPartitions));

			id = 0;
			while(GPTRecord[id].type[1] != 0) {
				bufferPrintf("bdev: partition id: %d, sectors: %d - %d\r\n", id, (uint32_t)GPTRecord[id].beginLBA, (uint32_t)GPTRecord[id].endLBA);
				id++;
			}
		}
		GPTData.headerChecksum = headerChecksum;
	}

	HasBDevInit = TRUE;

	return 0;
}

int bdevRead(io_func* io, off_t location, size_t size, void *buffer) {
	if (useGPT) {
		GPTPartitionRecord* record = (GPTPartitionRecord*) io->data;
		return ftl_read(buffer, location + record->beginLBA * BLOCK_SIZE, size);
	} else {
		MBRPartitionRecord* record = (MBRPartitionRecord*) io->data;
		//bufferPrintf("bdev: attempt to read %d sectors from partition %d, sector %Ld to 0x%x\r\n", size, ((uint32_t)record - (uint32_t)MBRData.partitions)/sizeof(MBRPartitionRecord), location, buffer);
		return ftl_read(buffer, location + record->beginLBA * BLOCK_SIZE, size);
	}
}

static int bdevWrite(io_func* io, off_t location, size_t size, void *buffer) {
	if (useGPT) {
		GPTPartitionRecord* record = (GPTPartitionRecord*) io->data;
		return ftl_write(buffer, location + record->beginLBA * BLOCK_SIZE, size);
	} else {
		MBRPartitionRecord* record = (MBRPartitionRecord*) io->data;
		//bufferPrintf("bdev: attempt to write %d sectors to partition %d, sector %d!\r\n", size, ((uint32_t)record - (uint32_t)MBRData.partitions)/sizeof(MBRPartitionRecord), location);
		return ftl_write(buffer, location + record->beginLBA * BLOCK_SIZE, size);
	}
}

static void bdevClose(io_func* io) {
	free(io);
}

unsigned int bdev_get_start(int partition)
{
	if (useGPT)
		return GPTRecord[partition].beginLBA;
	else
		return MBRData.partitions[partition].beginLBA;
}

io_func* bdev_open(int partition) {
	io_func* io;

	if(MBRData.partitions[partition].type != 0xAF && (!useGPT || partition >= 128))
		return NULL;

	io = (io_func*) malloc(sizeof(io_func));

	if (useGPT)
		io->data = &GPTRecord[partition];
	else
		io->data = &MBRData.partitions[partition];

	io->read = &bdevRead;
	io->write = &bdevWrite;
	io->close = &bdevClose;

	return io;
}

