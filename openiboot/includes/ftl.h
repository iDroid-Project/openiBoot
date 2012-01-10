/**
 * ftl.h
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

/**
 *  @file Header file for OiB's Flash Translation Layer
 *
 */

#ifndef  FTL_H
#define  FTL_H

#include "mtd.h"
#include "vfl.h"

/**
 *  FTL Device Prototypes
 *
 */
struct _ftl_device;

typedef error_t (*ftl_device_open_t)(struct _ftl_device *, vfl_device_t *_vfl);
typedef void (*ftl_device_close_t)(struct _ftl_device *);

typedef error_t (*ftl_read_single_page_t)(struct _ftl_device *, uint32_t _page, uint8_t *_buffer);
typedef error_t (*ftl_write_single_page_t)(struct _ftl_device *, uint32_t _page, uint8_t *_buffer);

/**
 *  FTL Device Structure
 *
 */
typedef struct _ftl_device
{
	mtd_t mtd;
	
	size_t block_size;

	ftl_device_open_t open;
	ftl_device_close_t close;

	ftl_read_single_page_t read_single_page;
	ftl_write_single_page_t write_single_page;

} ftl_device_t;

/**
 *  FTL Device Functions
 *
 */
error_t ftl_init(ftl_device_t *_dev);
void ftl_cleanup(ftl_device_t *_dev);

error_t ftl_register(ftl_device_t *_dev);
void ftl_unregister(ftl_device_t *_dev);

error_t ftl_open(ftl_device_t *_dev, vfl_device_t *_vfl);
void ftl_close(ftl_device_t *_dev);

error_t ftl_read_single_page(ftl_device_t *_dev, uint32_t _page, uint8_t *_buffer);
error_t ftl_write_single_page(ftl_device_t *_dev, uint32_t _page, uint8_t *_buffer);

error_t ftl_detect(ftl_device_t **_dev, vfl_device_t *_vfl);

#endif //FTL_H
