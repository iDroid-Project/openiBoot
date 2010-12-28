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
