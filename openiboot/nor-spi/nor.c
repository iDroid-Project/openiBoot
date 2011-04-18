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
	
	Boolean supports_aai;
	uint32_t erase_block_size;
	uint32_t block_protect_bits;
	uint32_t bytes_per_write;
	uint32_t write_timeout;

	int block_protect_count;
} nor_device_t;


// Functions
static uint8_t nor_get_status(nor_device_t *_dev);
static void nor_write_status(nor_device_t *_dev, uint8_t status);
static int nor_wait_for_ready(nor_device_t *_dev, int timeout);

static void nor_write_enable(nor_device_t *_dev);
static void nor_write_disable(nor_device_t *_dev);

static int nor_setup_chip_info(nor_device_t *_dev);

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

static int nor_prepare(mtd_t *_dev)
{
	nor_device_t *dev = nor_device_get(_dev);
	spi_set_baud(dev->spi, 12000000, SPIWordSize8, 1, 0, 0);
	return 0;
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


static void nor_write_status(nor_device_t *_dev, uint8_t status)
{
	if(nor_wait_for_ready(_dev, 100) != 0)
		return;

	nor_write_enable(_dev);

	uint8_t command[2];
	command[0] = NOR_SPI_WRSR;
	command[1] = status;

	nor_spi_tx(_dev, command, sizeof(command));
	
	nor_write_disable(_dev);
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


static int nor_device_init(nor_device_t *_dev)
{
	nor_prepare(&_dev->mtd);
	
	// Load values specific to the actual NOR chip
	if (nor_setup_chip_info(_dev) != 0)
		return -1;

	// Unprotect NOR
	uint8_t command[1];
	command[0] = NOR_SPI_EWSR;
	nor_spi_tx(_dev, command, sizeof(command));

	uint8_t wrsrCommand[2] = {NOR_SPI_WRSR, 0};
	nor_spi_tx(_dev, wrsrCommand, sizeof(wrsrCommand));

	nor_finish(&_dev->mtd);

	return mtd_init(&_dev->mtd);
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


static int nor_erase_sector(nor_device_t *_dev, uint32_t _offset)
{
	nor_disable_block_protect(_dev);

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
	
	nor_enable_block_protect(_dev);

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

	if(nor_wait_for_ready(dev, 100) != 0)
		return -1;

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
	bufferPrintf("writing %08x to %08x\r\n", _src, _off);
	nor_device_t *dev = nor_device_get(_dev);
	int startSector = _off / NOR_BLOCK_SIZE;
	int endSector = (_off + _amt) / NOR_BLOCK_SIZE;

	int numSectors = endSector - startSector + 1;
	uint8_t* sectorsToChange = (uint8_t*) malloc(NOR_BLOCK_SIZE * numSectors);
	nor_read(_dev, sectorsToChange, startSector * NOR_BLOCK_SIZE, NOR_BLOCK_SIZE * numSectors);

	int offsetFromStart = _off - (startSector * NOR_BLOCK_SIZE);

	memcpy(sectorsToChange + offsetFromStart, _src, _amt);

	nor_disable_block_protect(dev);

	int i;
	for(i = 0; i < numSectors; i++) {
		bufferPrintf("writing sector #%d\r\n", i);
		if(nor_erase_sector(dev, (i + startSector) * NOR_BLOCK_SIZE) != 0)
		{
			nor_enable_block_protect(dev);
			return -1;
		}

		int j;
		uint8_t* curSector = (uint8_t*)(sectorsToChange + (i * NOR_BLOCK_SIZE));
		for(j = 0; j < NOR_BLOCK_SIZE; j++)
		{
			if(nor_write_byte(dev, ((i + startSector) * NOR_BLOCK_SIZE) + j, curSector[j]) != 0)
			{
				nor_enable_block_protect(dev);
				return -1;
			}
		}
	}
	
	nor_enable_block_protect(dev);

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

static int nor_setup_chip_info(nor_device_t *_dev)
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
		return -1;
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
			_dev->supports_aai = TRUE;
			_dev->erase_block_size = 0x1000;
			_dev->block_protect_bits = 0x3C;
			_dev->bytes_per_write = 0x1;
			_dev->write_timeout = 10;
			break;
		case 0x1F:
			// vendor: atmel, device: 0x02 -> AT25DF081A
			// datasheet: http://www.atmel.com/dyn/resources/prod_documents/doc8715.pdf
			if (_dev->device != 0x02) {
				break;
			}
			chipRecognized = TRUE;
			_dev->supports_aai = FALSE;
			_dev->erase_block_size = 0x1000;
			_dev->block_protect_bits = 0xC;
			_dev->bytes_per_write = 0x100;
			_dev->write_timeout = 5000;
			break;
		case 0x20:
			// vendor: SGS/Thomson, device: 0x18, 0x16 or 0x14
			if (_dev->device != 0x18 && _dev->device != 0x16 && _dev->device != 0x14) {
				break;
			}
			chipRecognized = TRUE;
			_dev->supports_aai = FALSE;
			_dev->erase_block_size = 0x1000;
			_dev->block_protect_bits = 0x1C;
			_dev->bytes_per_write = 0x100;
			_dev->write_timeout = 5000;
			break;
		case 0x01:
			// vendor: AMD, device: 0x18
			if (_dev->device != 0x18) {
				break;
			}
			chipRecognized = TRUE;
			_dev->supports_aai = FALSE;
			_dev->erase_block_size = 0x1000;
			_dev->block_protect_bits = 0x1C;
			_dev->bytes_per_write = 0x100;
			_dev->write_timeout = 5000;
			break;
		default:
			break;
	}

	if (!chipRecognized) {
		bufferPrintf("Unknown NOR chip!\r\n");
		return -1;
	}
	
	return 0;
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
	
	.block_protect_count = 0,

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
