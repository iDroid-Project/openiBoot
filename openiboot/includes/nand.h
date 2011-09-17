#ifndef  NAND_H
#define  NAND_H

#include "openiboot.h"
#include "device.h"

enum
{
	ENAND_EMPTY = ERROR(0x23),
	ENAND_ECC = ERROR(0x24),
};

// NAND Device Prototypes
struct _nand_device;

typedef error_t (*nand_device_read_single_page_t)(struct _nand_device *, uint32_t _chip,
		uint32_t _block, uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer, uint32_t _disable_aes);

typedef error_t (*nand_device_write_single_page_t)(struct _nand_device *, uint32_t _chip,
		uint32_t _block, uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer);

typedef error_t (*nand_device_erase_single_block_t)(struct _nand_device *, uint32_t _chip,
		uint32_t _block);

typedef error_t (*nand_device_enable_encryption_t)(struct _nand_device *, int _enabled);
typedef error_t (*nand_device_enable_data_whitening_t)(struct _nand_device *, int _enabled);

typedef void (*nand_device_set_ftl_region_t)(uint32_t _lpn, uint32_t _a2, uint32_t _count, void* _buf);


// NAND Device Struct
typedef struct _nand_device
{
	device_t device;

	nand_device_read_single_page_t read_single_page;
	nand_device_write_single_page_t write_single_page;
	nand_device_erase_single_block_t erase_single_block;

	nand_device_enable_encryption_t enable_encryption;
	nand_device_enable_data_whitening_t enable_data_whitening;

	nand_device_set_ftl_region_t set_ftl_region;

} nand_device_t;

// NAND Device Functions
void nand_device_init(nand_device_t *_nand);
void nand_device_cleanup(nand_device_t *_nand);

error_t nand_device_register(nand_device_t *_nand);
void nand_device_unregister(nand_device_t *_nand);

error_t nand_device_read_single_page(nand_device_t *_dev, uint32_t _chip, uint32_t _block,
		uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer, uint32_t _disable_aes);

error_t nand_device_write_single_page(nand_device_t *_dev, uint32_t _chip, uint32_t _block,
		uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer);

error_t nand_device_erase_single_block(nand_device_t *_dev, uint32_t _chip, uint32_t _block);

error_t nand_device_read_special_page(nand_device_t *_dev, uint32_t _ce, char _page[16],
		uint8_t* _buffer, size_t _amt);

error_t nand_device_enable_encryption(nand_device_t *_dev, int _enabled);

error_t nand_device_enable_data_whitening(nand_device_t *_dev, int _enabled);

error_t nand_device_set_ftl_region(nand_device_t *_dev, uint32_t _lpn, uint32_t _a2, uint32_t _count, void *_buf);

#define nand_device_get_info(dev, item, val, sz) (device_get_info(&(dev)->device, (item), (val), (sz)))
#define nand_device_set_info(dev, item, val, sz) (device_set_info(&(dev)->device, (item), (val), (sz)))

#endif //NAND_H
