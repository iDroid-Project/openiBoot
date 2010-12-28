#include "openiboot.h"
#include "commands.h"
#include "hfs/common.h"
#include "hfs/bdev.h"
#include "hfs/fs.h"
#include "hfs/hfsplus.h"
#include "util.h"

int HasFSInit = FALSE;

void writeToHFSFile(HFSPlusCatalogFile* file, uint8_t* buffer, size_t bytesLeft, Volume* volume) {
	io_func* io;

	io = openRawFile(file->fileID, &file->dataFork, (HFSPlusCatalogRecord*)file, volume);
	if(io == NULL) {
		hfs_panic("error opening file");
		return;
	}
	allocate((RawFile*)io->data, bytesLeft);
	
	if(!WRITE(io, 0, (size_t)bytesLeft, buffer)) {
		hfs_panic("error writing");
	}

	CLOSE(io);
}

int add_hfs(Volume* volume, uint8_t* buffer, size_t size, const char* outFileName) {
	HFSPlusCatalogRecord* record;
	int ret;
	
	record = getRecordFromPath(outFileName, volume, NULL, NULL);
	
	if(record != NULL) {
		if(record->recordType == kHFSPlusFileRecord) {
			writeToHFSFile((HFSPlusCatalogFile*)record, buffer, size, volume);
			ret = TRUE;
		} else {
			ret = FALSE;
		}
	} else {
		if(newFile(outFileName, volume)) {
			record = getRecordFromPath(outFileName, volume, NULL, NULL);
			writeToHFSFile((HFSPlusCatalogFile*)record, buffer, size, volume);
			ret = TRUE;
		} else {
			ret = FALSE;
		}
	}
	
	if(record != NULL) {
		free(record);
	}
	
	return ret;
}

void displayFolder(HFSCatalogNodeID folderID, Volume* volume) {
	CatalogRecordList* list;
	CatalogRecordList* theList;
	HFSPlusCatalogFolder* folder;
	HFSPlusCatalogFile* file;
	
	theList = list = getFolderContents(folderID, volume);
	
	while(list != NULL) {
		if(list->record->recordType == kHFSPlusFolderRecord) {
			folder = (HFSPlusCatalogFolder*)list->record;
			bufferPrintf("%06o ", folder->permissions.fileMode);
			bufferPrintf("%3d ", folder->permissions.ownerID);
			bufferPrintf("%3d ", folder->permissions.groupID);
			bufferPrintf("%12d ", folder->valence);
		} else if(list->record->recordType == kHFSPlusFileRecord) {
			file = (HFSPlusCatalogFile*)list->record;
			bufferPrintf("%06o ", file->permissions.fileMode);
			bufferPrintf("%3d ", file->permissions.ownerID);
			bufferPrintf("%3d ", file->permissions.groupID);
			bufferPrintf("%12Ld ", file->dataFork.logicalSize);
		}
		
		bufferPrintf("                 ");

		printUnicode(&list->name);
		bufferPrintf("\r\n");
		
		list = list->next;
	}
	
	releaseCatalogRecordList(theList);
}

void displayFileLSLine(HFSPlusCatalogFile* file, const char* name) {
	bufferPrintf("%06o ", file->permissions.fileMode);
	bufferPrintf("%3d ", file->permissions.ownerID);
	bufferPrintf("%3d ", file->permissions.groupID);
	bufferPrintf("%12Ld ", file->dataFork.logicalSize);
	bufferPrintf("                 ");
	bufferPrintf("%s\r\n", name);
}

uint32_t readHFSFile(HFSPlusCatalogFile* file, uint8_t** buffer, Volume* volume) {
	io_func* io;
	size_t bytesLeft;

	io = openRawFile(file->fileID, &file->dataFork, (HFSPlusCatalogRecord*)file, volume);
	if(io == NULL) {
		hfs_panic("error opening file");
		free(buffer);
		return 0;
	}

	bytesLeft = file->dataFork.logicalSize;
	*buffer = malloc(bytesLeft);
	if(!(*buffer)) {
		hfs_panic("error allocating memory");
		return 0;
	}
	
	if(!READ(io, 0, bytesLeft, *buffer)) {
		hfs_panic("error reading");
	}

	CLOSE(io);

	return file->dataFork.logicalSize;
}

void hfs_ls(Volume* volume, const char* path) {
	HFSPlusCatalogRecord* record;
	char* name;

	record = getRecordFromPath(path, volume, &name, NULL);
	
	bufferPrintf("%s: \r\n", name);
	if(record != NULL) {
		if(record->recordType == kHFSPlusFolderRecord)
			displayFolder(((HFSPlusCatalogFolder*)record)->folderID, volume);  
		else
			displayFileLSLine((HFSPlusCatalogFile*)record, name);
	} else {
		bufferPrintf("No such file or directory\r\n");
	}

	bufferPrintf("Total filesystem size: %Ld, free: %Ld\r\n", ((uint64_t)volume->volumeHeader->totalBlocks - volume->volumeHeader->freeBlocks) * volume->volumeHeader->blockSize, (uint64_t)volume->volumeHeader->freeBlocks * volume->volumeHeader->blockSize);
	
	free(record);
}

void fs_cmd_ls(int argc, char** argv) {
	if(argc < 2) {
		bufferPrintf("usage: %s <device> <partition> <directory>\r\n", argv[0]);
		return;
	}

	bdevfs_device_t *dev = bdevfs_open(parseNumber(argv[1]), parseNumber(argv[2]));
	if(!dev)
	{
		bufferPrintf("fs: Failed to open partition.\n");
		return;
	}

	if(argc > 2)
		hfs_ls(dev->volume, argv[3]);
	else
		hfs_ls(dev->volume, "/");

	bdevfs_close(dev);
}
COMMAND("fs_ls", "list files and folders", fs_cmd_ls);

void fs_cmd_cat(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("usage: %s <device> <partition> <file>\r\n", argv[0]);
		return;
	}

	bdevfs_device_t *dev = bdevfs_open(parseNumber(argv[1]), parseNumber(argv[2]));
	if(!dev)
	{
		bufferPrintf("fs: Failed to open partition.\n");
		return;
	}

	HFSPlusCatalogRecord *record = getRecordFromPath(argv[2], dev->volume, NULL, NULL);

	if(record != NULL)
	{
		if(record->recordType == kHFSPlusFileRecord)
		{
			uint8_t* buffer;
			uint32_t size = readHFSFile((HFSPlusCatalogFile*)record, &buffer, dev->volume);
			buffer = realloc(buffer, size + 1);
			buffer[size] = '\0';
			bufferPrintf("%s\r\n", buffer);
			free(buffer);
		}
		else
			bufferPrintf("Not a file, record type: %x\r\n", record->recordType);
	}
	else
		bufferPrintf("No such file or directory\r\n");
	
	free(record);
	bdevfs_close(dev);
}
COMMAND("fs_cat", "display a file", fs_cmd_cat);

int fs_extract(int device, int partition, const char* file, void* location) {
	int ret;

	bdevfs_device_t *dev = bdevfs_open(device, partition);
	if(!dev)
	{
		bufferPrintf("fs: Cannot open partition hd%d,%d.\n", device, partition);
		return -1;
	}

	HFSPlusCatalogRecord* record = getRecordFromPath(file, dev->volume, NULL, NULL);

	if(record != NULL)
	{
		if(record->recordType == kHFSPlusFileRecord)
		{
			uint8_t* buffer;
			uint32_t size = readHFSFile((HFSPlusCatalogFile*)record, &buffer, dev->volume);
			memcpy(location, buffer, size);
			free(buffer);
			ret = size;
		}
		else
			ret = -1;
	}
	else
		ret = -1;

	free(record);
	bdevfs_close(dev);

	return ret;
}

void fs_cmd_extract(int argc, char** argv)
{
	if(argc < 5)
	{
		bufferPrintf("usage: %s <device> <partition> <file> <location>\r\n", argv[0]);
		return;
	}

	fs_extract(parseNumber(argv[1]), parseNumber(argv[2]), argv[3], (void*)parseNumber(argv[4]));
}
COMMAND("fs_extract", "extract a file into memory", fs_cmd_extract);

void fs_cmd_add(int argc, char** argv)
{
	if(argc < 6)
	{
		bufferPrintf("usage: %s <device> <partition> <file> <location> <size>\r\n", argv[0]);
		return;
	}

	bdevfs_device_t *dev = bdevfs_open(parseNumber(argv[1]), parseNumber(argv[2]));
	uint32_t address = parseNumber(argv[4]);
	uint32_t size = parseNumber(argv[5]);

	if(add_hfs(dev->volume, (uint8_t*) address, size, argv[3]))
	{
		bufferPrintf("%d bytes of 0x%x stored in %s\r\n", size, address, argv[3]);
	}
	else
	{
		bufferPrintf("add_hfs failed for %s!\r\n", argv[3]);
	}

	if(block_device_sync(dev->handle) < 0)
		bufferPrintf("FS sync error!\n");

	bdevfs_close(dev);
}
COMMAND("fs_add", "store a file from memory", fs_cmd_add);

ExtentList* fs_get_extents(int device, int partition, const char* fileName) {
	unsigned int partitionStart;
	unsigned int physBlockSize;
	ExtentList* list = NULL;

	bdevfs_device_t *dev = bdevfs_open(device, partition);
	if(!dev)
		return NULL;

	physBlockSize = block_device_block_size(dev->handle->device);
	partitionStart = block_device_get_start(dev->handle);

	HFSPlusCatalogRecord* record = getRecordFromPath(fileName, dev->volume, NULL, NULL);

	if(record != NULL) {
		if(record->recordType == kHFSPlusFileRecord) {
			io_func* fileIO;
			HFSPlusCatalogFile* file = (HFSPlusCatalogFile*) record;
			unsigned int allocationBlockSize = dev->volume->volumeHeader->blockSize;
			int numExtents = 0;
			Extent* extent;
			int i;

			fileIO = openRawFile(file->fileID, &file->dataFork, record, dev->volume);
			if(!fileIO)
				goto out_free;

			extent = ((RawFile*)fileIO->data)->extents;
			while(extent != NULL)
			{
				numExtents++;
				extent = extent->next;
			}

			list = (ExtentList*) malloc(sizeof(ExtentList));
			list->numExtents = numExtents;

			extent = ((RawFile*)fileIO->data)->extents;
			for(i = 0; i < list->numExtents; i++)
			{
				list->extents[i].startBlock = partitionStart + (extent->startBlock * (allocationBlockSize / physBlockSize));
				list->extents[i].blockCount = extent->blockCount * (allocationBlockSize / physBlockSize);
				extent = extent->next;
			}

			CLOSE(fileIO);
		} else {
			goto out_free;
		}
	} else {
		goto out_close;
	}

out_free:
	free(record);

out_close:
	bdevfs_close(dev);

	return list;
}

