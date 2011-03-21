#ifndef  NAND_H
#define  NAND_H

#include "openiboot.h"

typedef struct _nand_geometry
{
	uint32_t field_0;
	uint16_t num_ce;
	uint16_t blocks_per_ce;
	uint16_t pages_per_block;
	uint16_t bytes_per_page;
	uint16_t bbt_format;
	uint16_t bytes_per_spare;
	uint16_t banks_per_ce_vfl;
	uint16_t banks_per_ce;
	uint16_t blocks_per_bank;
	uint16_t unk14;
	uint16_t pages_per_block_2;
	uint16_t unk18;
	uint16_t unk1A;
	uint32_t unk1C;
	uint32_t vendorType;
	uint8_t ecc_bits;
	uint8_t ecc_tag;

	uint32_t num_fmi;
	uint32_t blocks_per_bank_32;
	uint32_t banks_per_ce_32;
	uint32_t pages_per_block_32;
	uint32_t pages_per_block_2_32;

	uint32_t page_number_bit_width;
	uint32_t page_number_bit_width_2;
	uint32_t is_ppn;
	uint32_t num_ecc_bytes;
	uint32_t meta_per_logical_page;
	uint32_t field_60;
	uint32_t pages_per_ce;

} nand_geometry_t;

// NAND Device Prototypes
struct _nand_device;

typedef nand_geometry_t *(*nand_device_get_geometry_t)(struct _nand_device *);

typedef int (*nand_device_read_t)(struct _nand_device *, uint32_t _chip, uint32_t _block,
		uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer);

typedef int (*nand_device_write_t)(struct _nand_device *, uint32_t _chip, uint32_t _block,
		uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer);

typedef void (*nand_device_enable_encryption_t)(struct _nand_device *, int _enabled);

// NAND Device Struct
typedef struct _nand_device
{
	int encryption_enabled;

	nand_device_get_geometry_t get_geometry;

	nand_device_read_t read;
	nand_device_write_t write;

	nand_device_enable_encryption_t enable_encryption;
} nand_device_t;

// NAND Device Functions
void nand_device_init(nand_device_t *_nand);
void nand_device_cleanup(nand_device_t *_nand);

nand_device_t *nand_device_allocate();

nand_geometry_t *nand_device_get_geometry(nand_device_t *_dev);

int nand_device_read(nand_device_t *_dev, uint32_t _chip, uint32_t _block,
		uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer);

int nand_device_write(nand_device_t *_dev, uint32_t _chip, uint32_t _block,
		uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer);

void nand_device_enable_encryption(nand_device_t *_dev, int _enabled);

#endif //NAND_H
