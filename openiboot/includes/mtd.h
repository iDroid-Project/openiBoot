/**
 * mtd.h
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

#ifndef  MTD_H
#define  MTD_H

#include "device.h"
#include "bdev.h"

struct _mtd;
typedef error_t (*mtd_prepare_t)(struct _mtd *);
typedef void (*mtd_finish_t)(struct _mtd *);
typedef error_t (*mtd_read_t)(struct _mtd *, void *_dest, uint64_t _off, int _sz);
typedef error_t (*mtd_write_t)(struct _mtd *, void *_src, uint64_t _off, int _sz);

typedef int64_t (*mtd_get_attribute_t)(struct _mtd *);

typedef enum _mtd_use
{
	mtd_boot_images,
	mtd_filesystem,
	mtd_other,

} mtd_usage_t;

typedef struct _mtd
{
	device_t device;
	block_device_t bdev;
	uint64_t bdev_addr;

	LinkedList list_ptr;

	mtd_prepare_t prepare;
	mtd_finish_t finish;
	mtd_read_t read;
	mtd_write_t write;

	mtd_get_attribute_t size;
	mtd_get_attribute_t block_size;

	mtd_usage_t usage;
	int prepare_count;
} mtd_t;

// Used by drivers
error_t mtd_init(mtd_t *_mtd);
void mtd_cleanup(mtd_t *_mtd);

error_t mtd_register(mtd_t *_mtd);
void mtd_unregister(mtd_t *_mtd);

// Used by clients
mtd_t *mtd_find(mtd_t *_prev);

error_t mtd_prepare(mtd_t *_mtd);
void mtd_finish(mtd_t *_mtd);

int64_t mtd_size(mtd_t *_mtd);
int64_t mtd_block_size(mtd_t *_mtd);

error_t mtd_read(mtd_t *_mtd, void *_dest, uint64_t _off, int _sz);
error_t mtd_write(mtd_t *_mtd, void *_src, uint64_t _off, int _sz);

int mtd_list_devices();

#endif //MTD_H
