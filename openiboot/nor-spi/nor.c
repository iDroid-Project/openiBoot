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

typedef struct _nor_device
{
	mtd_t mtd;

	uint32_t gpio;
	uint32_t spi;
	uint32_t vendor;
	uint32_t device;

	int write_enabled;
} nor_device_t;

static inline nor_device_t *nor_device_get(mtd_t *_dev)
{
	return CONTAINER_OF(nor_device_t, mtd, _dev);
}

static int nor_spi_tx(nor_device_t *_dev, void *_tx, int _tx_amt)
{
	gpio_pin_output(_dev->gpio, 0);
	int ret = spi_tx(_dev->spi, _tx, _tx_amt, TRUE, 0);
	gpio_pin_output(_dev->gpio, 1);
	return ret;
}

static int nor_spi_txrx(nor_device_t *_dev, void *_tx, int _tx_amt, void *_rx, int _rx_amt)
{
	gpio_pin_output(_dev->gpio, 0);
	int ret = spi_tx(_dev->spi, _tx, _tx_amt, TRUE, 0);
	if(ret >= 0)
		ret = spi_rx(_dev->spi, _rx, _rx_amt, TRUE, 0);
	gpio_pin_output(_dev->gpio, 1);
	return ret;
}

static uint8_t nor_get_status(nor_device_t *_dev)
{
	uint8_t data[1];
	uint8_t command[1];
	command[0] = NOR_SPI_RDSR;

	nor_spi_txrx(_dev, command, sizeof(command), data, sizeof(data));

	return data[0];
}

static int nor_wait_for_ready(nor_device_t *_dev, int timeout)
{
	if((nor_get_status(_dev) & (1 << NOR_SPI_SR_BUSY)) == 0)
		return 0;

	uint64_t startTime = timer_get_system_microtime();
	while((nor_get_status(_dev) & (1 << NOR_SPI_SR_BUSY)) != 0) {
		if(has_elapsed(startTime, timeout * 1000)) {
			bufferPrintf("nor: timed out waiting for ready\r\n");
			return -1;
		}
	}

	return 0;
}

static int nor_prepare(mtd_t *_dev)
{
	nor_device_t *dev = nor_device_get(_dev);
	spi_set_baud(dev->spi, 12000000, SPIWordSize8, 1, 0, 0);
	return 0;
}

static void nor_finish(mtd_t *_dev)
{
}

static int nor_device_init(nor_device_t *_dev)
{
	nor_prepare(&_dev->mtd);
	uint8_t command[1] = {NOR_SPI_JEDECID};
	uint8_t deviceID[3];
	memset(deviceID, 0, sizeof(deviceID));
	nor_spi_txrx(_dev, command, sizeof(command), deviceID, sizeof(deviceID));

	_dev->vendor = deviceID[0];
	_dev->device = deviceID[2];

	if(deviceID[0] == 0)
	{
		bufferPrintf("NOR not detected.\n");
		return -1;
	}

	// Unprotect NOR
	command[0] = NOR_SPI_EWSR;
	nor_spi_tx(_dev, command, sizeof(command));

	uint8_t wrsrCommand[2] = {NOR_SPI_WRSR, 0};
	nor_spi_tx(_dev, wrsrCommand, sizeof(wrsrCommand));

	nor_finish(&_dev->mtd);

	bufferPrintf("NOR vendor=%x, device=%x\r\n", _dev->vendor, _dev->device);

	return mtd_init(&_dev->mtd);
}

static void nor_write_enable(nor_device_t *_dev)
{
	if(nor_wait_for_ready(_dev, 100) != 0)
		return;

	if(_dev->write_enabled == 0)
	{
		uint8_t command[1];
		command[0] = NOR_SPI_WREN;
		nor_spi_tx(_dev, command, sizeof(command));
	}

	_dev->write_enabled++;
}

static void nor_write_disable(nor_device_t *_dev)
{
	if(nor_wait_for_ready(_dev, 100) != 0)
		return;

	if(_dev->write_enabled == 1)
	{
		uint8_t command[1];
		command[0] = NOR_SPI_WRDI;
		nor_spi_tx(_dev, command, sizeof(command));
	}

	_dev->write_enabled--;
	if(_dev->write_enabled < 0)
		_dev->write_enabled = 0;
}

static int nor_erase_sector(nor_device_t *_dev, uint32_t _offset)
{
	if(nor_wait_for_ready(_dev, 100) != 0)
		return -1;

	nor_write_enable(_dev);

	uint8_t command[4];
	command[0] = NOR_SPI_ERSE_4KB;
	command[1] = (_offset >> 16) & 0xFF;
	command[2] = (_offset >> 8) & 0xFF;
	command[3] = _offset & 0xFF;

	nor_spi_tx(_dev, command, sizeof(command));

	nor_write_disable(_dev);

	return 0;
}

static int nor_write_byte(nor_device_t *_dev, uint32_t offset, uint8_t data)
{
	if(nor_wait_for_ready(_dev, 100) != 0)
		return -1;

	nor_write_enable(_dev);

	uint8_t command[5];
	command[0] = NOR_SPI_PRGM;
	command[1] = (offset >> 16) & 0xFF;
	command[2] = (offset >> 8) & 0xFF;
	command[3] = offset & 0xFF;
	command[4] = data & 0xFF;

	nor_spi_tx(_dev, command, sizeof(command));

	nor_write_disable(_dev);

	return 0;
}

static int nor_read(mtd_t *_dev, void *_dest, uint32_t _off, int _amt)
{
	uint8_t command[4];
	uint8_t* data = _dest;
	nor_device_t *dev = nor_device_get(_dev);
	int len = _amt;

	while(len > 0)
	{
		int toRead = MIN(len, NOR_MAX_READ);

		command[0] = NOR_SPI_READ;
		command[1] = (_off >> 16) & 0xFF;
		command[2] = (_off >> 8) & 0xFF;
		command[3] = _off & 0xFF;

		if(nor_spi_txrx(dev, command, sizeof(command), data, toRead) < 0)
			continue;

		len -= toRead;
		data += toRead;
		_off += toRead;
	}

	return _amt;
}

static int nor_write(mtd_t *_dev, void *_src, uint32_t _off, int _amt)
{
	nor_device_t *dev = nor_device_get(_dev);
	int startSector = _off / NOR_BLOCK_SIZE;
	int endSector = (_off + _amt) / NOR_BLOCK_SIZE;

	int numSectors = endSector - startSector + 1;
	uint8_t* sectorsToChange = (uint8_t*) malloc(NOR_BLOCK_SIZE * numSectors);
	nor_read(_dev, sectorsToChange, startSector * NOR_BLOCK_SIZE, NOR_BLOCK_SIZE * numSectors);

	int offsetFromStart = _off - (startSector * NOR_BLOCK_SIZE);

	memcpy(sectorsToChange + offsetFromStart, _src, _amt);

	int i;
	for(i = 0; i < numSectors; i++) {
		if(nor_erase_sector(dev, (i + startSector) * NOR_BLOCK_SIZE) != 0)
			return -1;

		nor_write_enable(dev);

		int j;
		uint8_t* curSector = (uint8_t*)(sectorsToChange + (i * NOR_BLOCK_SIZE));
		for(j = 0; j < NOR_BLOCK_SIZE; j++)
		{
			if(nor_write_byte(dev, ((i + startSector) * NOR_BLOCK_SIZE) + j, curSector[j]) != 0)
			{
				nor_write_disable(dev);
				return -1;
			}
		}

		nor_write_disable(dev);
	}

	free(sectorsToChange);

	return 0;
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

	.write_enabled = 0,

	.gpio = NOR_CS,
	.spi = NOR_SPI,
};

static void nor_init()
{
	if(!nor_device_init(&nor_device))
	{
		mtd_register(&nor_device.mtd);

		images_setup();
		nvram_setup();
		syscfg_setup();
	}
}
MODULE_INIT_BOOT(nor_init);
