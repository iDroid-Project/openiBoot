/**
 * @file
 *
 * Header file for OpeniBoot's VFL interface.
 *
 * Functions in this file are for manipulating a NAND device
 * as a contiguous block of data.
 *
 * @defgroup VFL
 */

/*
 * vfl.h
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

#ifndef  VFL_H
#define  VFL_H

#include "openiboot.h"
#include "nand.h"

/**
 * The VFL device info enumeration.
 *
 * This is used by vfl_get_info to determine
 * which info to obtain.
 *
 *  @ingroup VFL
 */
typedef enum _vfl_info
{
	diPagesPerBlockTotalBanks,
	diSomeThingFromVFLCXT,
	diFTLType,
	diBytesPerPageFTL,
	diMetaBytes0xC,
	diUnkn20_1,
	diTotalBanks,
} vfl_info_t;

/**
 * The VFL signature style enumeration.
 *
 * This is used to determine how the signature will
 * be read by vfl_detect.
 *
 *  @ingroup VFL
 */
typedef enum _vfl_signature_style
{
	vfl_old_signature = 0x64,
	vfl_new_signature = 0x65,
} vfl_signature_style_t;

// VFL Device Prototypes
struct _vfl_device;

typedef error_t (*vfl_open_t)(struct _vfl_device *, nand_device_t *_dev);
typedef void (*vfl_close_t)(struct _vfl_device *);

typedef nand_device_t *(*vfl_get_device_t)(struct _vfl_device *);

typedef void* *(*vfl_get_stats_t)(struct _vfl_device *, uint32_t *size);

typedef error_t (*vfl_read_single_page_t)(struct _vfl_device *, uint32_t _page, uint8_t *_buffer,
		uint8_t *_sparebuffer, int _empty_ok, int *_refresh, uint32_t _disable_aes);

typedef error_t (*vfl_write_single_page_t)(struct _vfl_device *, uint32_t _page, uint8_t *_buffer,
		uint8_t *_sparebuffer, int _scrub);

typedef error_t (*vfl_erase_single_block_t)(struct _vfl_device *, uint32_t _block, int _replace_bad_block);

typedef error_t (*vfl_write_context_t)(struct _vfl_device *, uint16_t *_control_block);

typedef uint16_t *(*vfl_get_ftl_ctrl_block_t)(struct _vfl_device *);

typedef error_t (*vfl_get_info_t)(struct _vfl_device *, vfl_info_t _item, void * _result, size_t _sz);

/**
 * The VFL device structure.
 *
 * This structure represents a VFL device,
 * which is basically a driver for a specific
 * VFL on a specific NAND device. One vfl_device_t
 * is created for each NAND device.
 *
 * @ingroup VFL
 */
typedef struct _vfl_device
{
	vfl_open_t open; /**< Used by vfl_open(). */
	vfl_close_t close; /**< Used by vfl_close(). */

	vfl_get_device_t get_device; /**< Used by vfl_get_device(). */
	vfl_get_stats_t get_stats; /**< Used by vfl_get_stats (). */

	vfl_read_single_page_t read_single_page; /**< Used by vfl_read_single_page(). */
	vfl_write_single_page_t write_single_page; /**< Used by vfl_write_single_page(). */
	vfl_erase_single_block_t erase_single_block; /**< Used by vfl_erase_single_block(). */
	vfl_write_context_t write_context; /**< Used by vfl_write_context(). */

	vfl_get_ftl_ctrl_block_t get_ftl_ctrl_block; /**< Used by vfl_get_ftl_ctrl_block(). */

	vfl_get_info_t get_info; /**< Used by vfl_get_info(). */

} vfl_device_t;

//
// VFL Device Functions
//

/**
 * Initialise a VFL device.
 *
 * This should be called before any vfl_device_t structure
 * is used. Usually, VFL device drivers call it in their
 * own init function.
 *
 * @param _vfl the device to initialise.
 *
 * @ingroup VFL
 */
void vfl_init(vfl_device_t *_vfl);

/**
 * Clean up a VFL device.
 *
 * This should be called when a VFL structure is finished
 * with (you should call close seperately too if you've
 * opened it.
 *
 * @param _vfl the device to clean up.
 *
 * @ingroup VFL
 */
void vfl_cleanup(vfl_device_t *_vfl);

/**
 * Open the VFL on a given NAND device.
 *
 * This should be called to open the VFL before
 * any other vfl functions are called (such as vfl_read_single_page).
 *
 * @param _vfl the VFL device to open.
 * @param _dev the NAND device to open it from.
 * @return Whether any problems occurred.
 *
 * @ingroup VFL
 */
error_t vfl_open(vfl_device_t *_vfl, nand_device_t *_dev);

/**
 * Close a VFL device.
 *
 * This should be called when operations on the VFL
 * are finished with.
 *
 * @param _vfl the VFL device to close.
 *
 * @ingroup VFL
 */
void vfl_close(vfl_device_t *_vfl);

/**
 * Get the NAND device associated with this VFL device.
 *
 * @return The NAND device associated with this VFL device.
 *
 * @ingroup VFL
 */
nand_device_t *vfl_get_device(vfl_device_t *_vfl);

/**
 * Get the statistics associated with this VFL device.
 *
 * @return The statistics associated with this VFL device.
 *
 * @ingroup VFL
 */
void* *vfl_get_stats(vfl_device_t *_vfl, uint32_t *size);

/**
 * Read a single page from the VFL.
 *
 * This reads a single page from the VFL, that is given a single page number,
 * the physical location of the page is calculated and the page is read.
 *
 * @param _vfl the VFL device to read from.
 * @param _page the page number to read.
 * @param _buffer the buffer to store the page data into. You should use device_get_info
 * 					to calculate the VFL's page size.
 * @param _spare the buffer to store the spare data, this can be NULL.
 * @param _empty_ok if this is non-zero, an empty page will not return an error.
 * @param _refresh_page if the pointed integer is non-zero the read will be attempted twice.
 * @param _disable_aes if this flag is true, AES decryption won't be done by the hardware.
 * @return Whether an error occurred.
 *
 * @ingroup VFL
 */
error_t vfl_read_single_page(vfl_device_t *_vfl, uint32_t _page, uint8_t* _buffer, uint8_t* _spare,
		int _empty_ok, int* _refresh_page, uint32_t _disable_aes);

/**
 * Write a single page to the VFL.
 *
 * Given a virtual page number, the VFL device calculates the physical location of the page,
 * and writes the given data to it.
 *
 * @param _vfl the VFL device to read from.
 * @param _page the page number to read.
 * @param _buffer the buffer to store into the page. You should use device_get_info
 * 					to calculate the VFL's page size.
 * @param _spare the buffer to read the spare data from.
 * @return Whether an error occurred.
 *
 * @ingroup VFL
 */
error_t vfl_write_single_page(vfl_device_t *_vfl, uint32_t _page, uint8_t* _buffer, uint8_t* _spare, int _scrub);

/**
 * Erase a single block in the VFL.
 *
 * Given a virtual block number, the VFL device erases the content of this block.
 * Erases must be done in a block-level; a single page erase is not possible.
 *
 * @param _vfl the VFL device to erase from.
 * @param _block the block number to erase.
 * @param _replace_bad_block if true, the VFL device will replace a bad block if it detects one.
 * @return Whether an error occurred.
 *
 * @ingroup VFL
 */
error_t vfl_erase_single_block(vfl_device_t *_vfl, uint32_t _block, int _replace_bad_block);

/**
 * Write a new VFL Context.
 *
 * @param _vfl the VFL device to write the context on.
 * @param _control_block pointer to the control block array.
 * @return Whether an error occured.
 *
 * @ingroup VFL
 */
error_t vfl_write_context(vfl_device_t *_vfl, uint16_t *_control_block);

/**
 * Get the FTL control blocks buffer.
 *
 * @return A buffer of three uint16_t block numbers, each is an FTL control block.
 *
 * @ingroup VFL
 */
uint16_t *vfl_get_ftl_ctrl_block(vfl_device_t *_vfl);

/**
 * Get a specific info about the VFL device.
 *
 * Given a vfl_info_t item descriptor, the VFL device will store the data into the given pointer,
 * not exceeding the maximum allowed size.
 *
 * @param _vfl the VFL device to read from.
 * @param _item the item to get.
 * @param _result the buffer in which to save the result.
 * @param _sz maximum number of bytes to write.
 * @return Whether an error occurred.
 *
 * @ingroup VFL
 */
error_t vfl_get_info(vfl_device_t *_vfl, vfl_info_t _item, void *_result, size_t _sz);

/**
 * Attempt to detect and open the VFL on a given NAND device.
 *
 * @param _vfl the pointer to store the output VFL device in.
 * @param _nand the NAND device where the VFL resides.
 * @param _sign the signature style.
 *
 * @ingroup VFL
 */
error_t vfl_detect(vfl_device_t **_vfl, nand_device_t *_nand, vfl_signature_style_t _sign);

#endif //VFL_H
