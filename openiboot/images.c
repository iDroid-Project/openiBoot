/*
 * images.c - Handles img3 and img2's from NOR devices
 *
 * Copyright 2010 iDroid Project
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

#include "openiboot.h"
#include "commands.h"
#include "images.h"
#include "util.h"
#include "aes.h"
#include "sha1.h"
#include "nvram.h"
#include "mtd.h"

static const uint32_t NOREnd = 0xFC000;

Image* imageList = NULL;
mtd_t *imagesDevice = NULL;

static uint32_t MaxOffset = 0;
static uint32_t ImagesStart = 0;
static uint32_t SegmentSize = 0;

static const uint8_t Img2HashPadding[] = {	0xAD, 0x2E, 0xE3, 0x8D, 0x2D, 0x9B, 0xE4, 0x35, 0x99, 4,
						0x44, 0x33, 0x65, 0x3D, 0xF0, 0x74, 0x98, 0xD8, 0x56, 0x3B,
						0x4F, 0xF9, 0x6A, 0x55, 0x45, 0xCE, 0x82, 0xF2, 0x9A, 0x5A,
						0xC2, 0xBC, 0x47, 0x61, 0x6D, 0x65, 0x4F, 0x76, 0x65, 0x72,
						0xA6, 0xA0, 0x99, 0x13};

static int IsImg3 = FALSE;

static void calculateHash(Img2Header* header, uint8_t* hash);
static void calculateDataHash(void* buffer, int len, uint8_t* hash);

static mtd_t *images_device()
{
	if(!imagesDevice)
	{
		mtd_t *dev = NULL;
		while((dev = mtd_find(dev)))
		{
			if(dev->usage == mtd_boot_images)
			{
				imagesDevice = dev;
				break;
			}
		}
	}

	return imagesDevice;
}

static int img3_setup(mtd_t *_dev) {
	Image* curImage = NULL;
	AppleImg3RootHeader* rootHeader = (AppleImg3RootHeader*) malloc(sizeof(AppleImg3RootHeader));

	uint32_t offset = ImagesStart;
	uint32_t index = 0;
	while(offset < NOREnd) {
		mtd_read(_dev, rootHeader, offset, sizeof(AppleImg3RootHeader));
		if(rootHeader->base.magic != IMG3_MAGIC)
			break;

		if(curImage == NULL) {
			curImage = (Image*) malloc(sizeof(Image));
			imageList = curImage;
		} else {
			curImage->next = (Image*) malloc(sizeof(Image));
			curImage = curImage->next;
		}

		curImage->type = rootHeader->extra.name;
		curImage->offset = offset;
		curImage->length = rootHeader->base.dataSize;
		curImage->padded = rootHeader->base.size;
		curImage->index = index++;
		curImage->hashMatch = TRUE;

		curImage->next = NULL;

		if((offset + curImage->padded) > MaxOffset) {
			MaxOffset = offset + curImage->padded;
		}

		offset += curImage->padded;
	}

	free(rootHeader);

	return 0;
}

int images_setup() {
	IMG2* header;
	Img2Header* curImg2;
	uint8_t hash[0x20];
	mtd_t *dev = images_device();
	if(!dev)
		return -1;

	mtd_prepare(dev);

	MaxOffset = 0;

	header = (IMG2*) malloc(sizeof(IMG2));

	uint32_t IMG2Offset = 0x0;
	for(IMG2Offset = 0; IMG2Offset < NOREnd; IMG2Offset += 4096) {
		mtd_read(dev, header, IMG2Offset, sizeof(IMG2));
		if(header->signature == IMG2Signature) {
			break;
		}
	}

	SegmentSize = header->segmentSize;
	ImagesStart = (header->imagesStart + header->dataStart) * SegmentSize;

	AppleImg3Header* img3Header = (AppleImg3Header*) malloc(sizeof(AppleImg3Header));
	mtd_read(dev, img3Header, ImagesStart, sizeof(AppleImg3Header));
	if(img3Header->magic == IMG3_MAGIC) {
		img3_setup(dev);
		mtd_finish(dev);
		free(img3Header);
		free(header);
		IsImg3 = TRUE;
		return 0;
	} else {
		free(img3Header);
		IsImg3 = FALSE;
	}

	curImg2 = (Img2Header*) malloc(sizeof(Img2Header));

	Image* curImage = NULL;

	uint32_t curOffset;
	for(curOffset = ImagesStart; curOffset < NOREnd; curOffset += SegmentSize) {
		mtd_read(dev, curImg2, curOffset, sizeof(Img2Header));
		if(curImg2->signature != Img2Signature)
			continue;

		uint32_t checksum = 0;
		crc32(&checksum, curImg2, 0x64);

		if(checksum != curImg2->header_checksum) {
			bufferPrintf("Checksum mismatch at %x\r\n", curOffset);
			continue;
		}

		if(curImage == NULL) {
			curImage = (Image*) malloc(sizeof(Image));
			imageList = curImage;
		} else {
			curImage->next = (Image*) malloc(sizeof(Image));
			curImage = curImage->next;
		}

		curImage->type = curImg2->imageType;
		curImage->offset = curOffset;
		curImage->length = curImg2->dataLen;
		curImage->padded = curImg2->dataLenPadded;
		curImage->index = curImg2->index;

		memcpy(curImage->dataHash, curImg2->dataHash, 0x40);

		calculateHash(curImg2, hash);

		if(memcmp(hash, curImg2->hash, 0x20) == 0) {
			curImage->hashMatch = TRUE;
		} else {
			curImage->hashMatch = FALSE;
		}

		curImage->next = NULL;

		if((curOffset + curImage->padded) > MaxOffset) {
			MaxOffset = curOffset + curImage->padded;
		}
	}

	free(curImg2);
	free(header);

	mtd_finish(dev);

	return 0;
}

void images_list() {
	Image* curImage = imageList;
	if(curImage == NULL)
	{
		bufferPrintf("images: No boot images.\n");
		return;
	}

	while(curImage != NULL) {
		print_fourcc(curImage->type);
		bufferPrintf("(%d/%d): offset: 0x%x, length: 0x%x, padded: 0x%x\r\n", curImage->index, curImage->hashMatch, curImage->offset, curImage->length, curImage->padded);
		curImage = curImage->next;
	}
}

Image* images_get(uint32_t type) {
	Image* curImage = imageList;

	while(curImage != NULL) {
		if(type == curImage->type) {
			return curImage;
		}
		curImage = curImage->next;
	}

	return NULL;
}

Image* images_get_last_apple_image()
{
    Image* curImage = imageList;
    Image* lastImage=NULL;
    
    while(curImage != NULL) {
        lastImage = curImage;
        curImage = curImage->next;
        if(curImage !=NULL && (curImage->type == fourcc("mtz2") || curImage->type == fourcc("mtza"))) {
            return lastImage;
        }
    }
    
    return lastImage;
}

void images_append(void* data, int len) {
	mtd_t *dev = images_device();
	if(!dev)
		return;

	mtd_prepare(dev);

	if(MaxOffset >= 0xfc000 || (MaxOffset + len) >= 0xfc000) {
		bufferPrintf("**ABORTED** Writing image of size %d at %x would overflow NOR!\r\n", len, MaxOffset);
	} else {
		mtd_write(dev, data, MaxOffset, len);

		// Destroy any following image
		if((MaxOffset + len) < 0xfc000) {
			uint8_t zero = 0;
			mtd_write(dev, &zero, MaxOffset + len, 1);
		}

		images_release();
		images_setup();
	}

	mtd_finish(dev);
}

void images_rewind() {
	MaxOffset = ImagesStart;
}

void images_release() {
	Image* curImage = imageList;
	Image* toRelease = NULL;
	while(curImage != NULL) {
		toRelease = curImage;
		curImage = curImage->next;
		free(toRelease);
	}
	imageList = NULL;
}

void images_duplicate(Image* image, uint32_t type, int index) {
	if(image == NULL)
		return;

	mtd_t *dev = images_device();
	if(!dev)
		return;

	mtd_prepare(dev);

	uint32_t offset = MaxOffset + (SegmentSize - (MaxOffset % SegmentSize));

	uint32_t totalLen = sizeof(Img2Header) + image->padded;
	uint8_t* buffer = (uint8_t*) malloc(totalLen);

	mtd_read(dev, buffer, image->offset, totalLen);
	Img2Header* header = (Img2Header*) buffer;
	header->imageType = type;

	if(index >= 0)
		header->index = index;

	calculateDataHash(buffer + sizeof(Img2Header), image->padded, header->dataHash);

	uint32_t checksum = 0;
	crc32(&checksum, buffer, 0x64);
	header->header_checksum = checksum;

	calculateHash(header, header->hash);

	mtd_write(dev, buffer, offset, totalLen);

	free(buffer);

	mtd_finish(dev);

	images_release();
	images_setup();
}

void images_duplicate_at(Image* image, uint32_t type, int index, int offset) {
	if(image == NULL)
		return;

	mtd_t *dev = images_device();
	if(!dev)
		return;

	mtd_prepare(dev);

	uint32_t totalLen = sizeof(Img2Header) + image->padded;
	uint8_t* buffer = (uint8_t*) malloc(totalLen);

	mtd_read(dev, buffer, image->offset, totalLen);
	Img2Header* header = (Img2Header*) buffer;
	header->imageType = type;

	if(index >= 0)
		header->index = index;

	calculateDataHash(buffer + sizeof(Img2Header), image->padded, header->dataHash);

	uint32_t checksum = 0;
	crc32(&checksum, buffer, 0x64);
	header->header_checksum = checksum;

	calculateHash(header, header->hash);

	mtd_write(dev, buffer, offset, totalLen);

	free(buffer);

	mtd_finish(dev);

	images_release();
	images_setup();
}

void images_from_template(Image* image, uint32_t type, int index, void* dataBuffer, unsigned int len, int encrypt) {
	if(image == NULL)
		return;

	mtd_t *dev = images_device();
	if(!dev)
		return;

	mtd_prepare(dev);

	uint32_t offset = MaxOffset + (SegmentSize - (MaxOffset % SegmentSize));
	uint32_t padded = len;
	if((len & 0xF) != 0) {
		padded = (padded & ~0xF) + 0x10;
	}

	uint32_t totalLen = sizeof(Img2Header) + padded;
	uint8_t* buffer = (uint8_t*) malloc(totalLen);

	mtd_read(dev, buffer, image->offset, sizeof(Img2Header));
	Img2Header* header = (Img2Header*) buffer;
	header->imageType = type;

	if(index >= 0)
		header->index = index;

	header->dataLen = len;
	header->dataLenPadded = padded;

	memcpy(buffer + sizeof(Img2Header), dataBuffer, len);
	if(encrypt)
		aes_838_encrypt(buffer + sizeof(Img2Header), padded, NULL);

	calculateDataHash(buffer + sizeof(Img2Header), image->padded, header->dataHash);

	uint32_t checksum = 0;
	crc32(&checksum, buffer, 0x64);
	header->header_checksum = checksum;

	calculateHash(header, header->hash);

	mtd_write(dev, buffer, offset, totalLen);

	free(buffer);

	mtd_finish(dev);

	images_release();
	images_setup();
}

void images_write(Image* image, void* data, unsigned int length, int encrypt) {
	bufferPrintf("images_write(%x, %x, %x)\r\n", image, data, length);
	if(image == NULL)
		return;

	mtd_t *dev = images_device();
	if(!dev)
		return;

	mtd_prepare(dev);

	uint32_t padded = length;
	if((length & 0xF) != 0) {
		padded = (padded & ~0xF) + 0x10;
	}

	if(image->next != NULL && (image->offset + sizeof(Img2Header) + padded) >= image->next->offset) {
		bufferPrintf("**ABORTED** requested length greater than available space.\r\n");
		return;
	}

	uint32_t totalLen = sizeof(Img2Header) + padded;
	uint8_t* writeBuffer = (uint8_t*) malloc(totalLen);

	mtd_read(dev, writeBuffer, image->offset, sizeof(Img2Header));

	memcpy(writeBuffer + sizeof(Img2Header), data, length);

	if(encrypt)
		aes_838_encrypt(writeBuffer + sizeof(Img2Header), padded, NULL);

	Img2Header* header = (Img2Header*) writeBuffer;
	header->dataLen = length;
	header->dataLenPadded = padded;

	calculateDataHash(writeBuffer + sizeof(Img2Header), padded, header->dataHash);

	uint32_t checksum = 0;
	crc32(&checksum, writeBuffer, 0x64);
	header->header_checksum = checksum;

	calculateHash(header, header->hash);

	bufferPrintf("mtd_write(0x%p, %x, %x, %x)\r\n", dev, writeBuffer, image->offset, totalLen);

	mtd_write(dev, writeBuffer, image->offset, totalLen);

	bufferPrintf("mtd_write(0x%p, %x, %x, %x) done\r\n", dev, writeBuffer, image->offset, totalLen);

	free(writeBuffer);

	mtd_finish(dev);

	images_release();
	images_setup();

}

unsigned int images_read(Image* image, void** data) {
	if(image == NULL) {
		*data = NULL;
		return 0;
	}

	mtd_t *dev = images_device();
	if(!dev)
	{
		*data = NULL;
		return 0;
	}

	mtd_prepare(dev);

	*data = malloc(image->padded);
	if(!IsImg3) {
		mtd_read(dev, *data, image->offset + sizeof(Img2Header), image->length);
		aes_838_decrypt(*data, image->length, NULL);
		mtd_finish(dev);
		return image->length;
	} else {
		mtd_read(dev, *data, image->offset, image->padded);

		uint32_t dataOffset = 0;
		uint32_t dataLength = 0;
		uint32_t kbagOffset = 0;
		uint32_t kbagLength = 0;
		uint32_t offset = (uint32_t)(*data + sizeof(AppleImg3RootHeader));
		while((offset - (uint32_t)(*data + sizeof(AppleImg3RootHeader))) < image->length) {
			AppleImg3Header* header = (AppleImg3Header*) offset;
			if(header->magic == IMG3_DATA_MAGIC) {
				dataOffset = offset + sizeof(AppleImg3Header);
				dataLength = header->dataSize;
			}
			if(header->magic == IMG3_KBAG_MAGIC) {
				kbagOffset = offset + sizeof(AppleImg3Header);
				kbagLength = header->dataSize;
			}
			offset += header->size;
		}

		AppleImg3KBAGHeader* kbag = (AppleImg3KBAGHeader*) kbagOffset;

		if(kbag != 0) {
			if(kbag->key_modifier == 1) {
				aes_decrypt((void*)(kbagOffset + sizeof(AppleImg3KBAGHeader)), 16 + (kbag->key_bits / 8), AESGID, NULL, 0, NULL);
			}

			AESKeyLen keyLen;
			switch(kbag->key_bits)
			{
				case 128:
					keyLen = AES128;
					break;
				case 192:
					keyLen = AES192;
					break;
				case 256:
					keyLen = AES256;
					break;
				default:
					keyLen = AES128;
					break;
			}

			aes_decrypt((void*)dataOffset, (dataLength / 16) * 16, AESCustom, (uint8_t*)(kbagOffset + sizeof(AppleImg3KBAGHeader) + 16), keyLen, (uint8_t*)(kbagOffset + sizeof(AppleImg3KBAGHeader)));
		}

		uint8_t* newBuf = malloc(dataLength);
		memcpy(newBuf, (void*)dataOffset, dataLength);
		free(*data);
		*data = newBuf;

		mtd_finish(dev);
		return dataLength;
	}
}

void images_install(void* newData, size_t newDataLen, uint32_t newFourcc, uint32_t replaceFourcc) {
	ImageDataList* list = NULL;
	ImageDataList* cur = NULL;
	ImageDataList* toReplace = NULL;
    ImageDataList* verify = NULL;
    
	int isReplace = (replaceFourcc != newFourcc) ? TRUE : FALSE;
	int isUpgrade = FALSE;

	mtd_t *dev = images_device();
	if(!dev)
		return;

	mtd_prepare(dev);

	Image* curImage = imageList;
    
	while(curImage != NULL) {
		if(cur == NULL) {
			list = cur = verify = malloc(sizeof(ImageDataList));
		} else {
			cur->next = malloc(sizeof(ImageDataList));
			cur = cur->next;
		}

		bufferPrintf("Reading: ");
		print_fourcc(curImage->type);
		bufferPrintf(" (%d bytes)\r\n", curImage->padded);

		cur->type = curImage->type;
		cur->next = NULL;
		cur->data = malloc(curImage->padded);
		mtd_read(dev, cur->data, curImage->offset, curImage->padded);

		if(isReplace && cur->type == replaceFourcc) {
			isUpgrade = TRUE;
		} else if(cur->type == newFourcc) {
			toReplace = cur;
		}

		curImage = curImage->next;
	}

	mtd_finish(dev);

	if(!isUpgrade) {
		bufferPrintf("Performing installation... (%d bytes)\r\n", newDataLen);

		ImageDataList* ibox = malloc(sizeof(ImageDataList));
		ibox->type = replaceFourcc;
		ibox->data = toReplace->data;
		ibox->next = toReplace->next;

		toReplace->next = ibox;
		toReplace->data = images_inject_img3(toReplace->data, newData, newDataLen);
		images_change_type(ibox->data, ibox->type);
	} else {
		bufferPrintf("Performing upgrade... (%d bytes)\r\n", newDataLen);
		void* newIBoot = images_inject_img3(toReplace->data, newData, newDataLen);
		free(toReplace->data);
		toReplace->data = newIBoot;
	}

    //check for size and availability
    size_t newPaddedDataLen=0;
    size_t totalBytes=0;
    //if somebody can find how to get padded length for new ibot maybe this loop not needed
    while(verify != NULL) {
        cur = verify;
        verify = verify->next;
        AppleImg3RootHeader* header = (AppleImg3RootHeader*) cur->data;
        totalBytes += header->base.size;
        
        if(cur->type == newFourcc) {
            newPaddedDataLen = header->base.size;
        }
    }
    
    bufferPrintf("Total size to be written %d\r\n",totalBytes);
    if((ImagesStart + totalBytes) >= 0xfc000) {
        bufferPrintf("**ABORTED** Writing total image size: 0x%x, new ibot size: 0x%x at 0x%x would overflow NOR!\r\n", totalBytes, newPaddedDataLen,ImagesStart);
        images_rewind();
        images_release();
        images_setup();
        return;
    }
	
	bufferPrintf("Flashing...\r\n");

	images_rewind();
	while(list != NULL) {
		cur = list;
		list = list->next;
		AppleImg3RootHeader* header = (AppleImg3RootHeader*) cur->data;

		bufferPrintf("Flashing: ");
		print_fourcc(cur->type);
		bufferPrintf(" (%x, %d bytes)\r\n", cur->data, header->base.size);

		images_append(cur->data, header->base.size);

		free(cur->data);
		free(cur);
	}
	bufferPrintf("Flashing Complete, Free space after flashing %d\r\n",0xfc000-MaxOffset);

	images_release();
	images_setup();

    //bufferPrintf("Configuring openiBoot settings...\r\n");

/*#ifndef CONFIG_S5L8720	//TODO: add this back in once FTL is up and running
    Volume* volume;
    io_func* io;

    io = bdev_open(0);
    volume = openVolume(io);

    char buffer [sizeof(XSTRINGIFY(OPENIBOOT_VERSION))];
    strcpy(buffer, XSTRINGIFY(OPENIBOOT_VERSION));
    add_hfs(volume, (uint8_t*)buffer, sizeof(buffer), "/openiboot");

    closeVolume(volume);
    CLOSE(io);

    ftl_sync();
#endif*/ // TODO: This is broken now, move into menu.c for next release -- Ricky26

  /*  if(!nvram_getvar("opib-temp-os")) {
    	nvram_setvar("opib-temp-os", "0");
    }
    
    if(!nvram_getvar("opib-default-os")) {
		nvram_setvar("opib-default-os", "1");
    }

    if(!nvram_getvar("opib-menu-timeout")) {
		nvram_setvar("opib-menu-timeout", "10000");
    }

    nvram_save();*/ // TODO: The defaults should NOT be written to NVRAM. -- Ricky26

    //bufferPrintf("openiBoot installation complete.\r\n");
}

void images_uninstall(uint32_t _fourcc, uint32_t _unreplace) {
	ImageDataList* list = NULL;
	ImageDataList* cur = NULL;
	ImageDataList* oldImage = NULL;

	mtd_t *dev = images_device();
	if(!dev)
		return;

	mtd_prepare(dev);

	Image* curImage = imageList;

	while(curImage != NULL) {
		if(curImage->type != _fourcc) {
			if(cur == NULL) {
				list = cur = malloc(sizeof(ImageDataList));
			} else {
				cur->next = malloc(sizeof(ImageDataList));
				cur = cur->next;
			}

			bufferPrintf("Reading: ");
			print_fourcc(curImage->type);
			bufferPrintf(" (%d bytes)\r\n", curImage->padded);

			cur->type = curImage->type;
			cur->next = NULL;
			cur->data = malloc(curImage->padded);
			mtd_read(dev, cur->data, curImage->offset, curImage->padded);

			if(_fourcc != _unreplace && cur->type == _unreplace) {
				oldImage = cur;
			}
		} else {
			bufferPrintf("Skipping: ");
			print_fourcc(curImage->type);
			bufferPrintf(" (%d bytes)\r\n", curImage->padded);
		}

		curImage = curImage->next;
	}

	mtd_finish(dev);

	if(_fourcc != _unreplace && oldImage == NULL) {
		bufferPrintf("No openiBoot installation was found.\n");
		while(list != NULL) {
			cur = list;
			list = list->next;
			free(cur->data);
			free(cur);
		}
		return;
	}

	oldImage->type = _fourcc;
	images_change_type(oldImage->data, _fourcc);

	images_rewind();
	while(list != NULL) {
		cur = list;
		list = list->next;
		AppleImg3RootHeader* header = (AppleImg3RootHeader*) cur->data;

		bufferPrintf("Flashing: ");
		print_fourcc(cur->type);
		bufferPrintf(" (%x, %d bytes)\r\n", cur->data, header->base.size);

		images_append(cur->data, header->base.size);

		free(cur->data);
		free(cur);
	}

	bufferPrintf("Images uninstalled.\r\n");

	images_release();
	images_setup();

	bufferPrintf("Uninstall complete.\r\n");
}

void images_change_type(const void* img3Data, uint32_t type) {
	AppleImg3RootHeader* header = (AppleImg3RootHeader*) img3Data;
	header->extra.name = type;
}

void* images_inject_img3(const void* img3Data, const void* newData, size_t newDataLen) {
	uint8_t IVKey[16 + (256 / 8)];
	uint8_t* IV = IVKey;
	uint8_t* Key = &IVKey[16];

	uint32_t dataOffset = 0;
	uint32_t dataLength = 0;
	uint32_t kbagOffset = 0;
	uint32_t kbagLength = 0;
	uint32_t offset = (uint32_t)(img3Data + sizeof(AppleImg3RootHeader));

	size_t contentsLength = ((AppleImg3RootHeader*) img3Data)->base.dataSize;

	while((offset - (uint32_t)(img3Data + sizeof(AppleImg3RootHeader))) < contentsLength) {
		AppleImg3Header* header = (AppleImg3Header*) offset;
		if(header->magic == IMG3_DATA_MAGIC) {
			dataOffset = offset + sizeof(AppleImg3Header);
			dataLength = header->size;
		}
		if(header->magic == IMG3_KBAG_MAGIC) {
			kbagOffset = offset + sizeof(AppleImg3Header);
			kbagLength = header->dataSize;
		}
		offset += header->size;
	}

	AppleImg3KBAGHeader* kbag = (AppleImg3KBAGHeader*) kbagOffset;

	if(kbag != 0 && kbag->key_modifier == 1) {
		memcpy(IVKey, (void*)(kbagOffset + sizeof(AppleImg3KBAGHeader)), 16 + (kbag->key_bits / 8));
		aes_decrypt(IVKey, 16 + (kbag->key_bits / 8), AESGID, NULL, 0, NULL);
	}

	void* newImg3 = malloc(sizeof(AppleImg3RootHeader));

	memcpy(newImg3, img3Data, sizeof(AppleImg3RootHeader));
	AppleImg3RootHeader* rootHeader = (AppleImg3RootHeader*) newImg3;

	rootHeader->base.dataSize = rootHeader->base.dataSize - dataLength + (((newDataLen + 3)/4)*4) + sizeof(AppleImg3Header);
	rootHeader->base.size = (((rootHeader->base.dataSize + sizeof(AppleImg3RootHeader)) + 0x3F)/0x40)*0x40;
	newImg3 = realloc(newImg3, rootHeader->base.size);
	rootHeader = (AppleImg3RootHeader*) newImg3;
	void* cursor = newImg3 + sizeof(AppleImg3RootHeader);
	memset(cursor, 0, rootHeader->base.size - sizeof(AppleImg3RootHeader));

	offset = (uint32_t)(img3Data + sizeof(AppleImg3RootHeader));
	while((offset - (uint32_t)(img3Data + sizeof(AppleImg3RootHeader))) < contentsLength) {
		AppleImg3Header* header = (AppleImg3Header*) offset;
		if(header->magic == IMG3_DATA_MAGIC) {
			memcpy(cursor, (void*) offset, sizeof(AppleImg3Header));
			AppleImg3Header* newHeader = (AppleImg3Header*) cursor;
			newHeader->dataSize = newDataLen;
			newHeader->size = sizeof(AppleImg3Header) + (((newHeader->dataSize + 3)/4)*4);

			memcpy(cursor + sizeof(AppleImg3Header), newData, newDataLen);
      if(kbag != 0) {
  			aes_encrypt(cursor + sizeof(AppleImg3Header), (newDataLen / 16) * 16, AESCustom, Key, AES256, IV);
      }
			cursor += newHeader->size;
		} else {
			if(header->magic == IMG3_SHSH_MAGIC) {
				rootHeader->extra.shshOffset = (uint32_t)cursor - (uint32_t)newImg3 - sizeof(AppleImg3RootHeader);
			}
			memcpy(cursor, (void*) offset, header->size);
			cursor += header->size;
		}
		offset += header->size;
	}

	return newImg3;
}

static void calculateHash(Img2Header* header, uint8_t* hash) {
	SHA1_CTX context;
	SHA1Init(&context);
	SHA1Update(&context, (uint8_t*) header, 0x3E0);
	SHA1Final(hash, &context);
	memcpy(hash + 20, Img2HashPadding, 32 - 20);
	aes_img2verify_encrypt(hash, 32, NULL);
}

static void calculateDataHash(void* buffer, int len, uint8_t* hash) {
	SHA1_CTX context;
	SHA1Init(&context);
	SHA1Update(&context, buffer, len);
	SHA1Final(hash, &context);
	memcpy(hash + 20, Img2HashPadding, 64 - 20);
	aes_img2verify_encrypt(hash, 64, NULL);
}

int images_verify(Image* image) {
	uint8_t hash[0x40];
	int retVal = 0;

	if(image == NULL) {
		return 1;
	}

	mtd_t *dev = images_device();
	if(!dev)
	{
		return 1;
	}

	mtd_prepare(dev);

	if(!image->hashMatch)
		retVal |= 1 << 2;

	void* data = malloc(image->padded);
	mtd_read(dev, data, image->offset + sizeof(Img2Header), image->padded);
	calculateDataHash(data, image->padded, hash);
	free(data);

	if(memcmp(hash, image->dataHash, 0x40) != 0)
		retVal |= 1 << 3;

	mtd_finish(dev);

	return retVal;
}

void cmd_install(int argc, char** argv) {
	if((argc > 2 && argc < 4) || argc > 4)
	{
		bufferPrintf("Usage: %s <address> <len>\n", argv[0]);
		return;
	}

	if(argc == 4)
	{
		uint32_t offset = parseNumber(argv[1]);
		uint32_t len = parseNumber(argv[2]);
		bufferPrintf("Installing OIB from 0x%08x:%d.\n", offset, len);
		images_install((void*)offset, len, fourcc("ibot"), fourcc("ibox"));
	}
	else
	{
		bufferPrintf("Starting Install/Upgrade...\r\n");
		images_install(&_start, (uint32_t)&OpenIBootEnd - (uint32_t)&_start, fourcc("ibot"), fourcc("ibox"));  
	}
}
COMMAND("install", "install openiboot onto the device", cmd_install);

void cmd_uninstall(int argc, char** argv) {
    images_uninstall(fourcc("ibot"), fourcc("ibox"));
}
COMMAND("uninstall", "uninstall openiboot from the device", cmd_uninstall);

void cmd_images_list(int argc, char** argv) {
	images_list();
}
COMMAND("images_list", "list the images available on NOR", cmd_images_list);

void cmd_images_read(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <type> <address>\r\n", argv[0]);
		return;
	}

	Image* image = images_get(fourcc(argv[1]));
	void* imageData;
	size_t length = images_read(image, &imageData);
	uint32_t address = parseNumber(argv[2]);
	memcpy((void*)address, imageData, length);
	free(imageData);
	bufferPrintf("Read %d of %s to 0x%x - 0x%x\r\n", length, argv[1], address, address + length);
}
COMMAND("images_read", "read an image on NOR", cmd_images_read);

void cmd_images_install(int argc, char** argv) {
	if(argc < 4) {
		bufferPrintf("Usage: %s <tag> <address> <len>\r\n", argv[0]);
		return;
	}

	uint32_t tag = fourcc(argv[1]);
	uint32_t address = parseNumber(argv[2]);
	uint32_t len = parseNumber(argv[3]);

	bufferPrintf("Installing image %s to 0x%08x:%d.\n", argv[1], address, len);
	images_install((void*)address, len, tag, tag);
	bufferPrintf("Done.\r\n");
}
COMMAND("images_install", "install a boot image", cmd_images_install);

void cmd_images_uninstall(int argc, char** argv) {
	if(argc < 4) {
		bufferPrintf("Usage: %s <tag>\r\n", argv[0]);
		return;
	}

	uint32_t tag = fourcc(argv[1]);
	bufferPrintf("Uninstalling image %s.\n", argv[1]);
	images_uninstall(tag, tag);
	bufferPrintf("Done.\r\n");
}
COMMAND("images_uninstall", "uninstall a boot image", cmd_images_uninstall);

