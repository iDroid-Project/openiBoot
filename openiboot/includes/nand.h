#ifndef  NAND_H
#define  NAND_H

#include "openiboot.h"

typedef enum _nand_device_info
{
	diPagesPerBlock,
	diNumCE,
	diBlocksPerCE,
	diBBTFormat,
	diBytesPerSpare,
	diVendorType,
	diECCBits,
	diBanksPerCE_VFL,
	diTotalBanks_VFL,
	diPagesPerBlock2,
	diECCBits2,
	diBanksPerCE,
	diBlocksPerBank,
	diBlocksPerBank_dw,
	diBanksPerCE_dw,
	diPagesPerBlock_dw,
	diPagesPerBlock2_dw,
	diPageNumberBitWidth,
	diPageNumberBitWidth2,
	diNumBusses,
	diNumCEPerBus,
	diPPN,
	diReturnOne,
	diNumECCBytes,
	diMetaPerLogicalPage,
	diPagesPerCE,
} nand_device_info_t;

// NAND Device Prototypes
struct _nand_device;

typedef int (*nand_device_read_single_page_t)(struct _nand_device *, uint32_t _chip,
		uint32_t _block, uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer);

typedef int (*nand_device_write_single_page_t)(struct _nand_device *, uint32_t _chip,
		uint32_t _block, uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer);

typedef void (*nand_device_enable_encryption_t)(struct _nand_device *, int _enabled);

typedef uint32_t (*nand_device_get_info_t)(struct _nand_device *, nand_device_info_t _item);

// NAND Device Struct
typedef struct _nand_device
{
	nand_device_read_single_page_t read_single_page;
	nand_device_write_single_page_t write_single_page;

	nand_device_enable_encryption_t enable_encryption;

	nand_device_get_info_t get_info;
} nand_device_t;

// NAND Device Functions
void nand_device_init(nand_device_t *_nand);
void nand_device_cleanup(nand_device_t *_nand);

nand_device_t *nand_device_allocate();

int nand_device_read_single_page(nand_device_t *_dev, uint32_t _chip, uint32_t _block,
		uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer);

int nand_device_write_single_page(nand_device_t *_dev, uint32_t _chip, uint32_t _block,
		uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer);

void nand_device_enable_encryption(nand_device_t *_dev, int _enabled);

uint32_t nand_device_get_info(nand_device_t *_dev, nand_device_info_t _item);

#endif //NAND_H
