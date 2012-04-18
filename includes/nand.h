/**
 *	@file 
 *
 *  Header file for OpeniBoot's NAND device implementation.
 *
 *	Header file for NAND, including reading, writing, setup and NAND specific 
 *  functions.
 *
 *  @defgroup NAND
 */

/*
 * nand.h
 *
 * Copyright 2011 iDroid Project
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

#ifndef  NAND_H
#define  NAND_H

#include "openiboot.h"
#include "device.h"

enum
{
	ENAND_EMPTY = ERROR(0x23),
	ENAND_ECC = ERROR(0x24),
};

/**
 *	NAND Device Prototypes
 *
 *  @ingroup NAND
 */
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

/**
 *	NAND Device Struct
 *
 *  @ingroup NAND
 */
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

/**
 *	NAND Device Functions
 *  
 *  @ingroup NAND
 */
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
