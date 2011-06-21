#include "openiboot.h"
#include "commands.h"
#include "hfs/common.h"
#include "hfs/bdev.h"
#include "hfs/fs.h"
#include "hfs/hfsplus.h"
#include "hfs/hfscompress.h"
#include "hfs/hfscprotect.h"
#include "util.h"
#if defined(CONFIG_A4) || defined(CONFIG_S5L8920)
#include "h2fmi.h"
#include "aes.h"
#endif

#define BUFSIZE 1024*1024

int HasFSInit = FALSE;

static int silence = 0;

void hfs_setsilence(int s) {
	silence = s;
}

void writeToHFSFile(HFSPlusCatalogFile* file, uint8_t* buffer, size_t bytesLeft, Volume* volume) {
	io_func* io = NULL;

	if(file->permissions.ownerFlags & UF_COMPRESSED) {
		hfs_panic("file is compressed");
//		io = openHFSPlusCompressed(volume, file);
		if(io == NULL) {
			hfs_panic("error opening file");
			return;
		}
	} else {
		io = openRawFile(file->fileID, &file->dataFork, (HFSPlusCatalogRecord*)file, volume);
		if(io == NULL) {
			hfs_panic("error opening file");
			return;
		}
		allocate((RawFile*)io->data, bytesLeft);
	}

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

void grow_hfs(Volume* volume, uint64_t newSize) {
	uint32_t newBlocks;
	uint32_t blocksToGrow;
	uint64_t newMapSize;
	uint64_t i;
	unsigned char zero;
	
	zero = 0;	
	
	newBlocks = newSize / volume->volumeHeader->blockSize;
	
	if(newBlocks <= volume->volumeHeader->totalBlocks) {
		bufferPrintf("Cannot shrink volume\r\n");
		return;
	}

	blocksToGrow = newBlocks - volume->volumeHeader->totalBlocks;
	newMapSize = newBlocks / 8;
	
	if(volume->volumeHeader->allocationFile.logicalSize < newMapSize) {
		if(volume->volumeHeader->freeBlocks
		   < ((newMapSize - volume->volumeHeader->allocationFile.logicalSize) / volume->volumeHeader->blockSize)) {
			bufferPrintf("Not enough room to allocate new allocation map blocks\r\n");
			return;
		}
		
		allocate((RawFile*) (volume->allocationFile->data), newMapSize);
	}
	
	/* unreserve last block */	
	setBlockUsed(volume, volume->volumeHeader->totalBlocks - 1, 0);
	/* don't need to increment freeBlocks because we will allocate another alternate volume header later on */
	
	/* "unallocate" the new blocks */
	for(i = ((volume->volumeHeader->totalBlocks / 8) + 1); i < newMapSize; i++) {
		ASSERT(WRITE(volume->allocationFile, i, 1, &zero), "WRITE");
	}
	
	/* grow backing store size */
	ASSERT(WRITE(volume->image, newSize - 1, 1, &zero), "WRITE");
	
	/* write new volume information */
	volume->volumeHeader->totalBlocks = newBlocks;
	volume->volumeHeader->freeBlocks += blocksToGrow;
	
	/* reserve last block */	
	setBlockUsed(volume, volume->volumeHeader->totalBlocks - 1, 1);
	
	updateVolume(volume);
}

void removeAllInFolder(HFSCatalogNodeID folderID, Volume* volume, const char* parentName) {
	CatalogRecordList* list;
	CatalogRecordList* theList;
	char fullName[1024];
	char* name;
	char* pathComponent;
	int pathLen;
	char isRoot;
	
	HFSPlusCatalogFolder* folder;
	theList = list = getFolderContents(folderID, volume);
	
	strcpy(fullName, parentName);
	pathComponent = fullName + strlen(fullName);
	
	isRoot = FALSE;
	if(strcmp(fullName, "/") == 0) {
		isRoot = TRUE;
	}
	
	while(list != NULL) {
		name = unicodeToAscii(&list->name);
		if(isRoot && (name[0] == '\0' || strncmp(name, ".HFS+ Private Directory Data", sizeof(".HFS+ Private Directory Data") - 1) == 0)) {
			free(name);
			list = list->next;
			continue;
		}
		
		strcpy(pathComponent, name);
		pathLen = strlen(fullName);
		
		if(list->record->recordType == kHFSPlusFolderRecord) {
			folder = (HFSPlusCatalogFolder*)list->record;
			fullName[pathLen] = '/';
			fullName[pathLen + 1] = '\0';
			removeAllInFolder(folder->folderID, volume, fullName);
		} else {
			bufferPrintf("%s\r\n", fullName);
			removeFile(fullName, volume);
		}
		
		free(name);
		list = list->next;
	}
	
	releaseCatalogRecordList(theList);
	
	if(!isRoot) {
		*(pathComponent - 1) = '\0';
		bufferPrintf("%s\r\n", fullName);
		removeFile(fullName, volume);
	}
}

void displayFolder(HFSCatalogNodeID folderID, Volume* volume) {
	CatalogRecordList* list;
	CatalogRecordList* theList;
	HFSPlusCatalogFolder* folder;
	HFSPlusCatalogFile* file;
	HFSPlusDecmpfs* compressData = NULL;
	size_t attrSize;
	
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
			if(file->permissions.ownerFlags & UF_COMPRESSED) {
				// strict-alignment / __atribute__((__packed__)) would fuck us here. Dirty workaround.
				uint8_t* compressFu = NULL;
				attrSize = getAttribute(volume, file->fileID, "com.apple.decmpfs", (uint8_t**)(&compressFu));
				compressData = (HFSPlusDecmpfs*)compressFu;
//				attrSize = getAttribute(volume, file->fileID, "com.apple.decmpfs", (uint8_t**)(&compressData));
				flipHFSPlusDecmpfs(compressData);
				bufferPrintf("%12Ld ", compressData->size);
				free(compressData);
			} else {
				bufferPrintf("%12Ld ", file->dataFork.logicalSize);
			}
		}
			
		bufferPrintf("                 ");

		printUnicode(&list->name);
		bufferPrintf("\r\n");
		
		list = list->next;
	}
	
	releaseCatalogRecordList(theList);
}

void displayFileLSLine(Volume* volume, HFSPlusCatalogFile* file, const char* name) {
	HFSPlusDecmpfs* compressData = NULL;
	
	bufferPrintf("%06o ", file->permissions.fileMode);
	bufferPrintf("%3d ", file->permissions.ownerID);
	bufferPrintf("%3d ", file->permissions.groupID);

	if(file->permissions.ownerFlags & UF_COMPRESSED) {
		uint8_t* compressFu = NULL;
		getAttribute(volume, file->fileID, "com.apple.decmpfs", (uint8_t**)(&compressFu));
		compressData = (HFSPlusDecmpfs*)compressFu;
//		getAttribute(volume, file->fileID, "com.apple.decmpfs", (uint8_t**)(&compressData));
		flipHFSPlusDecmpfs(compressData);
		bufferPrintf("%12Ld ", compressData->size);
		free(compressData);
	} else {
		bufferPrintf("%12Ld ", file->dataFork.logicalSize);
	}

	bufferPrintf("                 ");
	bufferPrintf("%s\r\n", name);

	XAttrList* next;
	XAttrList* attrs = getAllExtendedAttributes(file->fileID, volume);
	if(attrs != NULL) {
		bufferPrintf("Extended attributes\r\n");
		while(attrs != NULL) {
			next = attrs->next;
			bufferPrintf("\t%s\r\n", attrs->name);
			if(!strcmp(attrs->name,"com.apple.system.cprotect")) {
				uint8_t* cprotectFu = NULL;
				getAttribute(volume, file->fileID, "com.apple.system.cprotect", (uint8_t**)(&cprotectFu));
				bufferPrintf("\t\twrapped_key: ");
				if(((HFSPlusCprotectV2*)cprotectFu)->xattr_version == 2) {
					HFSPlusCprotectV2* cprotect = (HFSPlusCprotectV2*)cprotectFu;
					bytesToHex(cprotect->wrapped_key, cprotect->wrapped_length);
				} else if (((HFSPlusCprotectV2*)cprotectFu)->xattr_version == 4) {
					HFSPlusCprotectV4* cprotect = (HFSPlusCprotectV4*)cprotectFu;
					bytesToHex(cprotect->wrapped_key, cprotect->wrapped_length);
				} else {
					hfs_panic("cprotect version unknown");
				}
				bufferPrintf("\r\n");
			}
			free(attrs->name);
			free(attrs);
			attrs = next;
		}	
	}	
}

void* cprotect_get(HFSPlusCatalogFile* file, Volume* volume) {
	XAttrList* next;
	XAttrList* attrs = getAllExtendedAttributes(file->fileID, volume);
	if(attrs != NULL) {
		while(attrs != NULL) {
			next = attrs->next;
			bufferPrintf("\t%s\r\n", attrs->name);
			if(!strcmp(attrs->name,"com.apple.system.cprotect")) {
				uint8_t* cprotectFu = NULL;
				getAttribute(volume, file->fileID, "com.apple.system.cprotect", (uint8_t**)(&cprotectFu));
				return (void*)cprotectFu;
			}
			free(attrs->name);
			free(attrs);
			attrs = next;
		}	
	}	
	return NULL;
}

uint32_t readHFSFile(HFSPlusCatalogFile* file, uint8_t** buffer, Volume* volume) {
	io_func* io = NULL;
	size_t bytesLeft;

	if(file->permissions.ownerFlags & UF_COMPRESSED) {
		hfs_panic("It's compressed!");
		return 0;
	} else {
		io = openRawFile(file->fileID, &file->dataFork, (HFSPlusCatalogRecord*)file, volume);
		if(io == NULL) {
			hfs_panic("error opening file");
			free(buffer);
			return 0;
		}
	}

	bytesLeft = file->dataFork.logicalSize;
	*buffer = malloc(bytesLeft);
	if(!(*buffer)) {
		hfs_panic("error allocating memory");
		return 0;
	}
	
#if defined(CONFIG_A4) || defined(CONFIG_S5L8920)
	void* cprotectFu = (void*)cprotect_get(file, volume);
	if(cprotectFu) {
		uint8_t cprotect_key[32];
		if(((HFSPlusCprotectV2*)cprotectFu)->xattr_version == 2) {
			HFSPlusCprotectV2* cprotect = (HFSPlusCprotectV2*)cprotectFu;
			aes_unwrap_key((const unsigned char*)DKey, AES256, NULL, cprotect_key, cprotect->wrapped_key, cprotect->wrapped_length);
			h2fmi_set_key(1, cprotect_key, AES256, 0);
		} else if (((HFSPlusCprotectV2*)cprotectFu)->xattr_version == 4) {
			HFSPlusCprotectV4* cprotect = (HFSPlusCprotectV4*)cprotectFu;
			aes_unwrap_key((const unsigned char*)DKey, AES256, NULL, cprotect_key, cprotect->wrapped_key, cprotect->wrapped_length);
			h2fmi_set_key(1, cprotect_key, AES256, 1);
		} else {
			hfs_panic("cprotect version unknown");
		}
	}
#endif
	if(!READ(io, 0, bytesLeft, *buffer)) {
		hfs_panic("error reading");
	}
#if defined(CONFIG_A4) || defined(CONFIG_S5L8920)
	if(cprotectFu) {
		h2fmi_set_key(0, NULL, 0, 0);
		free(cprotectFu);
	}
#endif

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
			displayFileLSLine(volume, (HFSPlusCatalogFile*)record, name);
	} else {
		bufferPrintf("No such file or directory\r\n");
	}

	bufferPrintf("Total filesystem size: %d, free: %d\r\n", (volume->volumeHeader->totalBlocks - volume->volumeHeader->freeBlocks) * volume->volumeHeader->blockSize, volume->volumeHeader->freeBlocks * volume->volumeHeader->blockSize);
	
	free(record);
}

void fs_cmd_ls(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("usage: %s <device> <partition> [<directory>]\r\n", argv[0]);
		return;
	}

	bdevfs_device_t *dev = bdevfs_open(parseNumber(argv[1]), parseNumber(argv[2]));
	if(!dev)
	{
		bufferPrintf("fs: Failed to open partition.\r\n");
		return;
	}

	if(argc > 3)
		hfs_ls(dev->volume, argv[3]);
	else
		hfs_ls(dev->volume, "/");

	bdevfs_close(dev);
}
COMMAND("fs_ls", "list files and folders", fs_cmd_ls);

void fs_cmd_cat(int argc, char** argv) {
	if(argc < 4) {
		bufferPrintf("usage: %s <device> <partition> <file>\r\n", argv[0]);
		return;
	}

	bdevfs_device_t *dev = bdevfs_open(parseNumber(argv[1]), parseNumber(argv[2]));
	if(!dev)
	{
		bufferPrintf("fs: Failed to open partition.\r\n");
		return;
	}

	HFSPlusCatalogRecord *record = getRecordFromPath(argv[3], dev->volume, NULL, NULL);

	if(record != NULL)
	{
		if(record->recordType == kHFSPlusFileRecord)
		{
			uint8_t* buffer;
			uint32_t size = readHFSFile((HFSPlusCatalogFile*)record, &buffer, dev->volume);
			buffer = realloc(buffer, size + 1);
			buffer[size] = '\0';
			uint32_t i;
			for(i = 0; i < size; i += 16) {
				uint8_t print[17];
				if(size-i < 16) {
					memset(print, 0, sizeof(size-i+1));
					memcpy(print, buffer+i, size-i);
				} else {
					memset(print, 0, sizeof(print));
					memcpy(print, buffer+i, 16);
				}
				bufferPrintf("%s", print);
			}
			bufferPrintf("\r\n");
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
		bufferPrintf("fs: Cannot open partition hd%d,%d.\r\n", device, partition);
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
		bufferPrintf("FS sync error!\r\n");

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

