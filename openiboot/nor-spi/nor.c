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

	error_t (*block_write_function)(struct _nor_device*, uint32_t, uint8_t*);
	uint32_t block_size;
	uint32_t block_protect_bits;
	uint32_t page_size;

	int block_protect_count;
} nor_device_t;

// Functions

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

static error_t nor_prepare(mtd_t *_dev)
{
	nor_device_t *dev = nor_device_get(_dev);
	spi_set_baud(dev->spi, 12000000, SPIWordSize8, 1, 0, 0);
	return SUCCESS;
}

static void nor_finish(mtd_t *_dev)
{
}

static uint8_t nor_get_status(nor_device_t *_dev)
{
	uint8_t data[1];
	uint8_t command[1];
	command[0] = NOR_SPI_RDSR;

	nor_spi_txrx(_dev, command, sizeof(command), data, sizeof(data));

	return data[0];
}

static error_t nor_wait_for_ready(nor_device_t *_dev, int timeout)
{
	if((nor_get_status(_dev) & (1 << NOR_SPI_SR_BUSY)) == 0)
		return SUCCESS;

	uint64_t startTime = timer_get_system_microtime();
	while((nor_get_status(_dev) & (1 << NOR_SPI_SR_BUSY)) != 0) {
		if(has_elapsed(startTime, timeout * 1000)) {
			bufferPrintf("nor: timed out waiting for ready\r\n");
			return ETIMEDOUT;
		}
	}

	return SUCCESS;
}

static void nor_write_enable(nor_device_t *_dev)
{
	// The write enable latch needs to be enabled before pretty much everything other than reading.
	// From AT25DF081A datasheet: "When the CS pin is deasserted, the WEL (write enable latch) bit
	// in the Status Register will be set to a logical '1'."
	// This therefore needs to be set everytime the CS pin is pulled low. iBoot implements a
	// counter to count enables/disables and only enable if the count is 0 but that is wrong.
	// Apple actually hacked it to not use the counter! If something really weird is going on, it
	// probably has to do with this. --kleemajo

	uint8_t command[1];
	command[0] = NOR_SPI_WREN;
	nor_spi_tx(_dev, command, sizeof(command));
}

static void nor_write_disable(nor_device_t *_dev)
{
	// See nor_write_enable note. This function is essentially useless (on the AT25DF081A chip at
	// least). It is implemented in case some other chip has different behaviour. --kleemajo
	
	uint8_t command[1];
	command[0] = NOR_SPI_WRDI;
	nor_spi_tx(_dev, command, sizeof(command));
}

static void nor_write_status(nor_device_t *_dev, uint8_t status)
{
	error_t ret = nor_wait_for_ready(_dev, 100);
	if(FAILED(ret))
		return;

	nor_write_enable(_dev);

	uint8_t command[2];
	command[0] = NOR_SPI_WRSR;
	command[1] = status;

	nor_spi_tx(_dev, command, sizeof(command));
	
	nor_write_disable(_dev);
}

static error_t nor_write_byte(nor_device_t *_dev, uint32_t offset, uint8_t data)
{
	error_t ret = nor_wait_for_ready(_dev, 100);
	if(FAILED(ret))
		return ret;

	nor_write_enable(_dev);

	uint8_t command[5];
	command[0] = NOR_SPI_PRGM;
	command[1] = (offset >> 16) & 0xFF;
	command[2] = (offset >> 8) & 0xFF;
	command[3] = offset & 0xFF;
	command[4] = data & 0xFF;

	nor_spi_tx(_dev, command, sizeof(command));

	nor_write_disable(_dev);

	return SUCCESS;
}

// This is the default writing function, and should be supported by every chip. It is slow.
static error_t nor_write_block_by_byte(nor_device_t *_dev, uint32_t offset, uint8_t *data)
{
	int j;
	for (j = 0; j < _dev->block_size; j++)
	{
		error_t ret = nor_write_byte(_dev, offset + j, data[j]);
		if(FAILED(ret))
			return ret;
	}
	
	return SUCCESS;
}

// This writing method should be supported by most chips and is usually the fastest option.
static error_t nor_write_block_by_page(nor_device_t *_dev, uint32_t offset, uint8_t *data)
{
	error_t ret;
	int j;
	uint8_t* command = malloc(4 + _dev->page_size);
	for (j = 0; j < _dev->block_size / _dev->page_size; j++)
	{
		ret = nor_wait_for_ready(_dev, 100);
		if(FAILED(ret))
		{
			free(command);
			return ret;
		}

		nor_write_enable(_dev);
	
		uint32_t pageOffset = offset + j * _dev->page_size;
		command[0] = NOR_SPI_PRGM;
		command[1] = (pageOffset >> 16) & 0xFF;
		command[2] = (pageOffset >> 8) & 0xFF;
		command[3] = pageOffset & 0xFF;
		memcpy(command + 4, data + j * _dev->page_size, _dev->page_size);
		
		nor_spi_tx(_dev, command, 4 + _dev->page_size);
		
		nor_write_disable(_dev);
	}
	
	free(command);
	return SUCCESS;
}


static void nor_disable_block_protect(nor_device_t *_dev)
{
	if (_dev->block_protect_count == 0)
	{
		uint8_t status = nor_get_status(_dev);
		nor_write_status(_dev, status & ~NOR_SPI_SR_BP_ALL);
	}
	
	_dev->block_protect_count++;
}


static void nor_enable_block_protect(nor_device_t *_dev)
{
	if (_dev->block_protect_count == 1)
	{
		uint8_t status = nor_get_status(_dev);
		nor_write_status(_dev, status | NOR_SPI_SR_BP_ALL);
	}
	
	if (_dev->block_protect_count > 0)
		_dev->block_protect_count--;
}


static error_t nor_erase_sector(nor_device_t *_dev, uint32_t _offset)
{
	nor_disable_block_protect(_dev);

	error_t ret = nor_wait_for_ready(_dev, 100);
	if(FAILED(ret))
		return ret;

	nor_write_enable(_dev);
	
	uint8_t command[4];
	command[0] = NOR_SPI_ERSE_4KB;
	command[1] = (_offset >> 16) & 0xFF;
	command[2] = (_offset >> 8) & 0xFF;
	command[3] = _offset & 0xFF;

	nor_spi_tx(_dev, command, sizeof(command));

	nor_write_disable(_dev);
	
	nor_enable_block_protect(_dev);

	return SUCCESS;
}

static error_t nor_read(mtd_t *_dev, void *_dest, uint32_t _off, int _amt)
{
	uint8_t command[4];
	uint8_t* data = _dest;
	nor_device_t *dev = nor_device_get(_dev);
	int len = _amt;

	error_t ret = nor_wait_for_ready(dev, 100);
	if(FAILED(ret))
		return ret;

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

	return SUCCESS_VALUE(_amt);
}

static error_t nor_write(mtd_t *_dev, void *_src, uint32_t _off, int _amt)
{
	nor_device_t *dev = nor_device_get(_dev);
	int startSector = _off / dev->block_size;
	int endSector = (_off + _amt) / dev->block_size;

	int numSectors = endSector - startSector + 1;
	uint8_t* sectorsToChange = (uint8_t*) malloc(dev->block_size * numSectors);
	nor_read(_dev, sectorsToChange, startSector * dev->block_size, dev->block_size * numSectors);

	int offsetFromStart = _off - (startSector * dev->block_size);

	memcpy(sectorsToChange + offsetFromStart, _src, _amt);

	nor_disable_block_protect(dev);

	int i;
	error_t ret = SUCCESS;
	for(i = 0; i < numSectors; i++)
	{
		ret = nor_erase_sector(dev, (i + startSector) * dev->block_size);
		if(FAILED(ret))
			break;
		
		uint8_t* curSector = (uint8_t*)(sectorsToChange + (i * dev->block_size));
		ret = dev->block_write_function(dev, (i + startSector) * dev->block_size, curSector);
		if(FAILED(ret))
			break;
	}
	
	nor_enable_block_protect(dev);
	free(sectorsToChange);
	return ret;
}

static int nor_size(mtd_t *_dev)
{
	return 16 * 1024 * 1024;
} 

static int nor_block_size(mtd_t *_dev)
{
	nor_device_t *dev = nor_device_get(_dev);
	return dev->block_size;
}

static error_t nor_setup_chip_info(nor_device_t *_dev)
{
	// Get the vendor and device IDs from the chip
	uint8_t command[1] = {NOR_SPI_JEDECID};
	uint8_t deviceID[3];
	memset(deviceID, 0, sizeof(deviceID));
	nor_spi_txrx(_dev, command, sizeof(command), deviceID, sizeof(deviceID));

	_dev->vendor = deviceID[0];
	_dev->device = deviceID[2];

	if(deviceID[0] == 0)
	{
		bufferPrintf("NOR not detected.\n");
		return ENOENT;
	}

	bufferPrintf("NOR vendor=%x, device=%x\r\n", _dev->vendor, _dev->device);

	Boolean chipRecognized = FALSE;
	switch(_dev->vendor) {
		// massive list of ids: http://flashrom.org/trac/flashrom/browser/trunk/flashchips.h
		case 0xBF:
			// vendor: SST, device: 0x41 or 0x8e
			if (_dev->device != 0x41 && _dev->device != 0x8e) {
				break;
			}
			chipRecognized = TRUE;
			//TODO: this chip supports AAI, and will be much faster when implemented --kleemajo
			_dev->block_write_function = &nor_write_block_by_byte;
			_dev->block_size = 0x1000;
			_dev->block_protect_bits = 0x3C;
			_dev->page_size = 0x1;
			break;
		case 0x1F:
			// vendor: atmel, device: 0x02 -> AT25DF081A
			// datasheet: http://www.atmel.com/dyn/resources/prod_documents/doc8715.pdf
			if (_dev->device != 0x02) {
				break;
			}
			chipRecognized = TRUE;
			_dev->block_write_function = &nor_write_block_by_page;
			_dev->block_size = 0x1000;
			_dev->block_protect_bits = 0xC;
			_dev->page_size = 0x100;
			break;
		case 0x20:
			// vendor: SGS/Thomson, device: 0x18, 0x16 or 0x14
			if (_dev->device != 0x18 && _dev->device != 0x16 && _dev->device != 0x14) {
				break;
			}
			chipRecognized = TRUE;
			_dev->block_write_function = &nor_write_block_by_page;
			_dev->block_size = 0x1000;
			_dev->block_protect_bits = 0x1C;
			_dev->page_size = 0x100;
			break;
		case 0x01:
			// vendor: AMD, device: 0x18
			if (_dev->device != 0x18) {
				break;
			}
			chipRecognized = TRUE;
			_dev->block_write_function = &nor_write_block_by_page;
			_dev->block_size = 0x1000;
			_dev->block_protect_bits = 0x1C;
			_dev->page_size = 0x100;
			break;
		default:
			break;
	}

	if (!chipRecognized) {
		bufferPrintf("Unknown NOR chip!\r\n");
		return EINVAL;
	}
	
	return SUCCESS;
}

static error_t nor_device_init(nor_device_t *_dev)
{
	error_t ret = mtd_init(&_dev->mtd);
	if(FAILED(ret))
		return ret;

	_dev->block_protect_count = 0;
	_dev->block_write_function = &nor_write_block_by_byte;
	_dev->block_protect_bits = NOR_SPI_SR_BP_ALL;
	_dev->page_size = 1;

	nor_prepare(&_dev->mtd);
	
	// Load values specific to the actual NOR chip
	ret = nor_setup_chip_info(_dev);
	if(FAILED(ret))
		return ret;

	// Unprotect NOR
	uint8_t command[1];
	command[0] = NOR_SPI_EWSR;
	nor_spi_tx(_dev, command, sizeof(command));

	uint8_t wrsrCommand[2] = {NOR_SPI_WRSR, 0};
	nor_spi_tx(_dev, wrsrCommand, sizeof(wrsrCommand));

	nor_finish(&_dev->mtd);

	return SUCCESS;
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
