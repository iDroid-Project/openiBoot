/*
 * mtd.c - OpeniBoot Menu
 *
 * Copyright 2010 iDroid Project
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

#include "mtd.h"
#include "arm/arm.h"
#include "commands.h"
#include "util.h"

static int mtd_init_done = 0;
static LinkedList mtd_list;

#define mtd_get(ptr)		(CONTAINER_OF(mtd_t, list_ptr, (ptr)))
#define mtd_get_bdev(ptr) 	(CONTAINER_OF(mtd_t, bdev, (ptr)))

static error_t mtd_bdev_prepare(block_device_t *_dev)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	return mtd_prepare(dev);
}

static void mtd_bdev_finish(block_device_t *_dev)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	mtd_finish(dev);
}

static error_t mtd_bdev_seek(block_device_t *_dev, seek_mode_t _mode, int64_t _amt)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	switch(_mode)
	{
	case seek_begin:
		dev->bdev_addr = (int64_t)_amt;
		break;

	case seek_end:
		{
			int sz = mtd_size(dev);
			if(sz <= 0)
				return EIO;
			
			dev->bdev_addr = sz - (int64_t)_amt;
			break;
		}

	case seek_offset:
		dev->bdev_addr += (int64_t)_amt;
		break;
	}

	return SUCCESS;
}

static error_t mtd_bdev_read(block_device_t *_dev, void *_dest, int _sz)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	int ret = mtd_read(dev, _dest, dev->bdev_addr, _sz);
	if(ret >= 0)
		dev->bdev_addr += _sz;

	return ret;
}

static error_t mtd_bdev_write(block_device_t *_dev, void *_src, int _sz)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	int ret = mtd_write(dev, _src, dev->bdev_addr, _sz);
	if(ret >= 0)
		dev->bdev_addr += _sz;

	return ret;
}

static int64_t mtd_bdev_size(block_device_t *_dev)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	return mtd_size(dev);
}

static int64_t mtd_bdev_block_size(block_device_t *_dev)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	return mtd_block_size(dev);
}

error_t mtd_init(mtd_t *_mtd)
{
	error_t ret = device_init(&_mtd->device);
	if(ret != 0)
		return ret;

	ret = block_device_init(&_mtd->bdev);
	if(ret != 0)
		return ret;

	_mtd->bdev.prepare = mtd_bdev_prepare;
	_mtd->bdev.finish = mtd_bdev_finish;

	_mtd->bdev.read = mtd_bdev_read;
	_mtd->bdev.write = mtd_bdev_write;
	_mtd->bdev.seek = mtd_bdev_seek;

	_mtd->bdev.size = mtd_bdev_size;
	_mtd->bdev.block_size = mtd_bdev_block_size;

	_mtd->list_ptr.next = NULL;
	_mtd->list_ptr.prev = NULL;
	_mtd->prepare_count = 0;
	_mtd->bdev_addr = 0;

	return SUCCESS;
}

void mtd_cleanup(mtd_t *_mtd)
{
}

error_t mtd_register(mtd_t *_mtd)
{
	error_t ret = device_register(&_mtd->device);
	if(FAILED(ret))
		return ret;

	EnterCriticalSection();

	if(!mtd_init_done)
	{
		mtd_init_done = 1;
		mtd_list.prev = &mtd_list;
		mtd_list.next = &mtd_list;
	}

	LinkedList *prev = mtd_list.prev;
	_mtd->list_ptr.prev = prev;
	_mtd->list_ptr.next = &mtd_list;
	prev->next = &_mtd->list_ptr;
	mtd_list.prev = &_mtd->list_ptr;
	LeaveCriticalSection();

	bufferPrintf("mtd: New device, '%s', registered.\n", _mtd->device.name);

	if(_mtd->usage == mtd_filesystem)
	{
		ret = block_device_register(&_mtd->bdev);
		if(FAILED(ret))
			return ret;
	}

	return SUCCESS;
}

void mtd_unregister(mtd_t *_mtd)
{
	if(_mtd->usage == mtd_filesystem)
		block_device_unregister(&_mtd->bdev);

	if(_mtd->list_ptr.next != NULL && _mtd->list_ptr.prev != NULL)
	{
		EnterCriticalSection();
		LinkedList *prev = _mtd->list_ptr.prev;
		LinkedList *next = _mtd->list_ptr.next;
		next->prev = prev;
		prev->next = next;
		_mtd->list_ptr.prev = NULL;
		_mtd->list_ptr.next = NULL;
		LeaveCriticalSection();
	}

	device_unregister(&_mtd->device);
}

mtd_t *mtd_find(mtd_t *_prev)
{
	LinkedList *ptr;
	if(_prev == NULL)
		ptr = mtd_list.next;
	else
		ptr = _prev->list_ptr.next;

	if(ptr == &mtd_list)
		return NULL;

	return mtd_get(ptr);
}

error_t mtd_prepare(mtd_t *_mtd)
{
	int ret = 0;
	if(_mtd->prepare_count == 0 && _mtd->prepare != NULL)
		ret = _mtd->prepare(_mtd);

	_mtd->prepare_count++;
	return SUCCESS;
}

void mtd_finish(mtd_t *_mtd)
{
	if(_mtd->prepare_count == 1 && _mtd->finish != NULL)
		_mtd->finish(_mtd);

	_mtd->prepare_count--;
	if(_mtd->prepare_count < 0)
		_mtd->prepare_count = 0;
}

int64_t mtd_size(mtd_t *_mtd)
{
	if(_mtd->size)
		return _mtd->size(_mtd);

	return -1;
}

int64_t mtd_block_size(mtd_t *_mtd)
{
	if(_mtd->block_size)
		return _mtd->block_size(_mtd);
	
	return -1;
}

error_t mtd_read(mtd_t *_mtd, void *_dest, uint64_t _off, int _sz)
{
	if(_mtd->read == NULL)
		return ENOENT;

	return _mtd->read(_mtd, _dest, _off, _sz);
}

error_t mtd_write(mtd_t *_mtd, void *_src, uint64_t _off, int _sz)
{
	if(_mtd->write == NULL)
		return ENOENT;

	return _mtd->write(_mtd, _src, _off, _sz);
}

error_t mtd_list_devices()
{
	mtd_t *mtd = NULL;
	int i = 0;

	bufferPrintf("mtd: Listing Devices:\n");
	while((mtd = mtd_find(mtd)))
	{
		bufferPrintf("mtd:    Device #%d '%s'.\n", i, mtd->device.name);
		i++;
	}

	return 0;
}

static error_t cmd_mtd_list(int argc, char **argv)
{
	return mtd_list_devices();
}
COMMAND("mtd_list", "List all MTD devices.", cmd_mtd_list);

static error_t cmd_mtd_read(int argc, char **argv)
{
	if(argc != 5)
	{
		bufferPrintf("Usage: %s [device] [destination] [offset] [size].\n", argv[0]);
		return EINVAL;
	}
	
	int idx = parseNumber(argv[1]);
	mtd_t *dev = mtd_find(NULL);
	while(idx > 0 && dev != NULL) {
		dev = mtd_find(dev);
		idx--;
	}

	if(!dev)
	{
		bufferPrintf("Invalid MTD index.\n");
		return ENOENT;
	}

	bufferPrintf("OK, we are going to read from device '%s'\r\n", dev->device.name);

	void *dest = (void*)parseNumber(argv[2]);
	uint32_t offset = parseNumber(argv[3]);
	int len = parseNumber(argv[4]);

	mtd_prepare(dev);
	error_t ret = mtd_read(dev, dest, offset, len);
	mtd_finish(dev);

	return ret;
}
COMMAND("mtd_read", "Read from a MTD device.", cmd_mtd_read);

static error_t cmd_mtd_write(int argc, char **argv)
{
	if(argc != 5)
	{
		bufferPrintf("Usage: %s [device] [source] [offset] [size].\n", argv[0]);
		return EINVAL;
	}
	
	int idx = parseNumber(argv[1]);
	mtd_t *dev = mtd_find(NULL);
	while(idx > 0 && dev != NULL) {
		dev = mtd_find(dev);
		idx--;
	}

	if(!dev)
	{
		bufferPrintf("Invalid MTD index.\n");
		return ENOENT;
	}

	bufferPrintf("OK, we are going to write to device '%s'\r\n", dev->device.name);

	void *src = (void*)parseNumber(argv[2]);
	uint32_t offset = parseNumber(argv[3]);
	int len = parseNumber(argv[4]);

	mtd_prepare(dev);
	error_t ret = mtd_write(dev, src, offset, len);
	mtd_finish(dev);

	return ret;
}
COMMAND("mtd_write", "Write to a MTD device.", cmd_mtd_write);
