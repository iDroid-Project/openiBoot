/*
 * ftl.c - OpeniBoot Flash transition layer
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

#include "vfl.h"
#include "ftl.h"

// FTL types
#ifdef CONFIG_FTL_YAFTL
#include "ftl/yaftl.h"
#endif

static error_t ftl_read_mtd(mtd_t *_dev, void *_dest, uint64_t _off, int _amt)
{
	ftl_device_t *ftl = CONTAINER_OF(ftl_device_t, mtd, _dev);
	uint8_t* curLoc = (uint8_t*) _dest;
	uint32_t block_size = ftl->block_size;
	int curPage = _off / block_size;
	int toRead = _amt;
	int pageOffset = _off - (curPage * block_size);
	uint8_t* tBuffer = (uint8_t*) malloc(block_size);
	error_t ret = SUCCESS;

	while(toRead > 0)
    {
		size_t this_amt = toRead;
		if(this_amt > (block_size-pageOffset))
			this_amt = block_size-pageOffset;

		ret = ftl_read_single_page(ftl, curPage, tBuffer);
		if(FAILED(ret))
			break;

		memcpy(curLoc, tBuffer + pageOffset, this_amt);

		curLoc += this_amt;
		toRead -= this_amt;
		pageOffset = 0;
		curPage++;
	}

	if(!FAILED(ret))
		ret = _amt;

	free(tBuffer);
	return ret;
}

static error_t ftl_write_mtd(mtd_t *_dev, void *_src, uint64_t _off, int _amt)
{
	ftl_device_t *ftl = CONTAINER_OF(ftl_device_t, mtd, _dev);
	uint8_t* curLoc = (uint8_t*) _src;
	uint32_t block_size = ftl->block_size;
	int curPage = _off / block_size;
	int toWrite = _amt;
	int pageOffset = _off - (curPage * block_size);

	error_t ret = SUCCESS;
	uint8_t* tBuffer = (uint8_t*) malloc(block_size);

	while(toWrite > 0)
	{
		size_t this_amt = toWrite;
		if(this_amt > (block_size-pageOffset))
			this_amt = block_size-pageOffset;

		memset(tBuffer, 0, block_size);
		ret = ftl_read_single_page(ftl, curPage, tBuffer);
		if(FAILED(ret))
			break;

		memcpy(tBuffer + pageOffset, curLoc, this_amt);

		ret = ftl_write_single_page(ftl, curPage, tBuffer);
		if(FAILED(ret))
			break;

		curLoc += this_amt;
		toWrite -= this_amt;
		pageOffset = 0;
		curPage++;
	}

	if(!FAILED(ret))
		ret = _amt;

	free(tBuffer);
	return ret;
}

static int64_t ftl_block_size(mtd_t *_dev)
{
	ftl_device_t *dev = CONTAINER_OF(ftl_device_t, mtd, _dev);
	return dev->block_size;
}

static error_t ftl_prepare(mtd_t *_dev)
{
	//ftl_device_t *dev = CONTAINER_OF(ftl_device_t, mtd, _dev);
	return SUCCESS;
}

static mtd_t ftl_mtd = {
	.device = {
		.fourcc = FOURCC('F', 'T', 'L', '\0'),
		.name = "FTL Layer",
	},

	.read = ftl_read_mtd,
	.write = ftl_write_mtd,
	.prepare = ftl_prepare,

	.block_size = ftl_block_size,

	.usage = mtd_filesystem,
};

error_t ftl_init(ftl_device_t *_dev)
{
	_dev->open = FALSE;

	memcpy(&_dev->mtd, &ftl_mtd, sizeof(ftl_mtd));
	error_t ret = mtd_init(&_dev->mtd);
	if(FAILED(ret))
		return ret;

	return SUCCESS;
}

void ftl_cleanup(ftl_device_t *_dev)
{
	mtd_cleanup(&_dev->mtd);
}

error_t ftl_register(ftl_device_t *_dev)
{
	error_t ret = mtd_register(&_dev->mtd);
	if(FAILED(ret))
		return ret;

	return SUCCESS;
}

void ftl_unregister(ftl_device_t *_dev)
{
	mtd_unregister(&_dev->mtd);
}

error_t ftl_open(ftl_device_t *_dev, vfl_device_t *_vfl)
{
	if(_dev->open)
		return _dev->open(_dev, _vfl);

	return ENOENT;
}

void ftl_close(ftl_device_t *_dev)
{
	if(_dev->close)
		_dev->close(_dev);
}

error_t ftl_read_single_page(ftl_device_t *_dev, uint32_t _page, uint8_t *_buffer)
{
	if(_dev->read_single_page)
		return _dev->read_single_page(_dev, _page, _buffer);

	return ENOENT;
}

error_t ftl_write_single_page(ftl_device_t *_dev, uint32_t _page, uint8_t *_buffer)
{
	if(_dev->write_single_page)
		return _dev->write_single_page(_dev, _page, _buffer);

	return ENOENT;
}

error_t ftl_detect(ftl_device_t **_dev, vfl_device_t *_vfl)
{
	// TODO: Implement FTL detection! -- Ricky26
	
#ifdef CONFIG_FTL_YAFTL
	ftl_yaftl_device_t *yaftl = ftl_yaftl_device_allocate();
	*_dev = &yaftl->ftl;

	ftl_open(*_dev, _vfl);
	ftl_register(*_dev);

	return SUCCESS;
#endif

	return ENOENT;
}

