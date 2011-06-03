#include "openiboot.h"
#include "commands.h"
#include "util.h"
#include "timer.h"
#include "spi.h"
#include "gpio.h"
#include "mtd.h"
#include "hardware/nor.h"
#include "hardware/spi.h"

#include "images.h"
#include "syscfg.h"
#include "nvram.h"

#define NOR_BLOCK_SIZE 4096

typedef struct _nor_device
{
	mtd_t mtd;

	uint32_t vendor;
	uint32_t device;

	int write_enabled;
} nor_device_t;

static inline nor_device_t *nor_device_get(mtd_t *_dev)
{
	return CONTAINER_OF(nor_device_t, mtd, _dev);
}

static error_t nor_prepare(mtd_t *_dev)
{
	return SUCCESS;
}

static void nor_finish(mtd_t *_dev)
{
}

static error_t nor_device_init(nor_device_t *_dev)
{
	error_t ret = mtd_init(&_dev->mtd);
	if(FAILED(ret))
		return ret;

	_dev->write_enabled = 0;

	SET_REG16(NOR + NOR_COMMAND, COMMAND_UNLOCK);
	SET_REG16(NOR + LOCK, LOCK_UNLOCK);

	SET_REG16(NOR + NOR_COMMAND, COMMAND_IDENTIFY);
	GET_REG16(NOR);
	GET_REG16(NOR);

	_dev->vendor = GET_REG16(NOR + VENDOR);
	_dev->device = GET_REG16(NOR + DEVICE);

	SET_REG16(NOR + NOR_COMMAND, COMMAND_LOCK);
	SET_REG16(NOR, DATA_MODE);
	GET_REG16(NOR);
	GET_REG16(NOR);

	if(_dev->vendor == 0)
	{
		bufferPrintf("NOR not detected.\n");
		return ENOENT;
	}

	bufferPrintf("NOR vendor=%x, device=%x\r\n", _dev->vendor, _dev->device);
	return SUCCESS;
}

static error_t nor_erase_sector(nor_device_t *_dev, uint32_t _offset)
{
	SET_REG16(NOR + NOR_COMMAND, COMMAND_UNLOCK);
	SET_REG16(NOR + LOCK, LOCK_UNLOCK);

	SET_REG16(NOR + NOR_COMMAND, COMMAND_ERASE);

	SET_REG16(NOR + NOR_COMMAND, COMMAND_UNLOCK);
	SET_REG16(NOR + LOCK, LOCK_UNLOCK);

	SET_REG16(NOR + _offset, ERASE_DATA);

	while(TRUE)
	{
		udelay(50000);
		if(GET_REG16(NOR + _offset) != 0xFFFF)
			bufferPrintf("failed to erase NOR sector: %x %x\r\n", (unsigned int) GET_REG16(NOR + _offset), GET_REG(0x20000000));
		else
			break;
	}

	return SUCCESS;
}

static error_t nor_write_short(nor_device_t *_dev, uint32_t offset, uint16_t data)
{
	SET_REG16(NOR + NOR_COMMAND, COMMAND_UNLOCK);
	SET_REG16(NOR + LOCK, LOCK_UNLOCK);

	SET_REG16(NOR + NOR_COMMAND, COMMAND_WRITE);
	SET_REG16(NOR + offset, data);

	while(TRUE)
	{
		udelay(40);
		if(GET_REG16(NOR + offset) != data)
			bufferPrintf("failed to write to NOR\r\n");
		else
			break;
	}

	return SUCCESS;
}

static error_t nor_read(mtd_t *_dev, void *_dest, uint32_t _off, int _amt)
{
	uint16_t* alignedBuffer = (uint16_t*)_dest;
	int len = _amt;
	for(; len >= 2; len -= 2)
	{
		*alignedBuffer = GET_REG16(NOR + _off);
		_off += 2;
		alignedBuffer++;
	}

	if(len > 0) {
		uint16_t lastWord = GET_REG16(NOR + _off);
		uint8_t* unalignedBuffer = (uint8_t*) alignedBuffer;
		*unalignedBuffer = *((uint8_t*)(&lastWord));
	}

	return SUCCESS_VALUE(_amt);
}

static error_t nor_write(mtd_t *_dev, void *_src, uint32_t _off, int _amt)
{
	nor_device_t *dev = nor_device_get(_dev);
	int startSector = _off / NOR_BLOCK_SIZE;
	int endSector = (_off + _amt) / NOR_BLOCK_SIZE;

	int numSectors = endSector - startSector + 1;
	uint8_t* sectorsToChange = (uint8_t*) malloc(NOR_BLOCK_SIZE * numSectors);
	nor_read(_dev, sectorsToChange, startSector * NOR_BLOCK_SIZE, NOR_BLOCK_SIZE * numSectors);

	int offsetFromStart = _off - (startSector * NOR_BLOCK_SIZE);

	memcpy(sectorsToChange + offsetFromStart, _src, _amt);

	error_t ret;
	int i;
	for(i = 0; i < numSectors; i++) {
		ret = nor_erase_sector(dev, (i + startSector) * NOR_BLOCK_SIZE);
		if(FAILED(ret))
			return ret;

		int j;
		uint16_t* curSector = (uint16_t*)(sectorsToChange + (i * NOR_BLOCK_SIZE));
		for(j = 0; j < NOR_BLOCK_SIZE; j += 2)
		{
			ret = nor_write_short(dev, ((i + startSector) * NOR_BLOCK_SIZE) + j, curSector[j/2]);
			if(FAILED(ret))
				return ret;
		}
	}

	free(sectorsToChange);
	return SUCCESS;
}

static int nor_size(mtd_t *_dev)
{
	return 16 * 1024 * 1024;
} 

static int nor_block_size(mtd_t *_dev)
{
	return NOR_BLOCK_SIZE;
}

static nor_device_t nor_device = {
	.mtd = {
		.device = {
			.fourcc = FOURCC('N', 'O', 'R', '\0'),
			.name = "SPI NOR Flash",
		},

		.prepare = nor_prepare,
		.finish = nor_finish,

		.read = nor_read,
		.write = nor_write,

		.size = nor_size,
		.block_size = nor_block_size,

		.usage = mtd_boot_images,
	},
};

static void nor_init()
{
	if(!nor_device_init(&nor_device))
	{
		mtd_register(&nor_device.mtd);

#ifdef S5L8900
		images_setup();
#endif
		nvram_setup();
		syscfg_setup();
	}
}
MODULE_INIT_BOOT(nor_init);
