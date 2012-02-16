/*
 * nand.c - OpeniBoot Menu
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

#include "nand.h"
#include "hardware/platform.h"
#include "util.h"

void nand_device_init(nand_device_t *_nand)
{
	memset(_nand, 0, sizeof(*_nand));

	device_init(&_nand->device);
	_nand->device.fourcc = FOURCC('N', 'A', 'N', 'D');
        _nand->device.name = "NAND";

	device_register(&_nand->device);
}

void nand_device_cleanup(nand_device_t *_nand)
{
}

error_t nand_device_register(nand_device_t *_nand)
{
	error_t ret = device_register(&_nand->device);
	if(FAILED(ret))
		return ret;

	return SUCCESS;
}

void nand_device_unregister(nand_device_t *_nand)
{
	device_unregister(&_nand->device);
}

error_t nand_device_read_single_page(nand_device_t *_dev, uint32_t _chip, uint32_t _block,
		uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer, uint32_t _disable_aes)
{
	if(!_dev->read_single_page)
		return ENOENT;

	return _dev->read_single_page(_dev, _chip, _block, _page, _buffer, _spareBuffer, _disable_aes);
}

error_t nand_device_write_single_page(nand_device_t *_dev, uint32_t _chip, uint32_t _block,
		uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer)
{
	if(!_dev->write_single_page)
		return ENOENT;

	return _dev->write_single_page(_dev, _chip, _block, _page, _buffer, _spareBuffer);
}

error_t nand_device_erase_single_block(nand_device_t *_dev, uint32_t _chip, uint32_t _block)
{
	if(!_dev->erase_single_block)
		return ENOENT;

	return _dev->erase_single_block(_dev, _chip, _block);
}

error_t nand_device_read_special_page(nand_device_t *_dev, uint32_t _ce, char _page[16], uint8_t *_buffer, size_t _amt)
{
	uint16_t bankAddressSpace;
	uint32_t bytesPerPage, blocksPerCE, pagesPerBlock, blocksPerBank;

	error_t ret = nand_device_get_info(_dev, diBytesPerPage, &bytesPerPage, sizeof(bytesPerPage));
	if(FAILED(ret))
		return EINVAL;

	ret = nand_device_get_info(_dev, diBlocksPerCE, &blocksPerCE, sizeof(blocksPerCE));
	if(FAILED(ret))
		return EINVAL;

	ret = nand_device_get_info(_dev, diPagesPerBlock, &pagesPerBlock, sizeof(pagesPerBlock));
	if(FAILED(ret))
		return EINVAL;

	ret = nand_device_get_info(_dev, diBlocksPerBank_dw, &blocksPerBank, sizeof(blocksPerBank));
	if(FAILED(ret))
		return EINVAL;

	ret = nand_device_get_info(_dev, diBankAddressSpace, &bankAddressSpace, sizeof(bankAddressSpace));
	if(FAILED(ret))
		return EINVAL;

	uint8_t* buffer = memalign(DMA_ALIGN, bytesPerPage);
	int lowestBlock = blocksPerCE - (blocksPerCE / 10);
	int block;
	for(block = blocksPerCE - 1; block >= lowestBlock; block--)
	{
		int page;
		int badBlockCount = 0;
		for(page = 0; page < pagesPerBlock; page++)
		{
			uint32_t physicalBlock = (block / blocksPerBank) * bankAddressSpace + (block % blocksPerBank);

			if(badBlockCount > 2)
			{
				DebugPrintf("vfl: read_special_page - too many bad pages, skipping block %d\r\n", block);
				break;
			}

			int ret = nand_device_read_single_page(_dev, _ce, physicalBlock, page, buffer, NULL, 0);
			if(ret != 0)
			{
				if(ret == 1)
				{
					DebugPrintf("vfl: read_special_page - found 'badBlock' on ce %d, page %d\r\n", _ce, (block * pagesPerBlock) + page);
					badBlockCount++;
				}

				DebugPrintf("vfl: read_special_page - skipping ce %d, page %d\r\n", _ce, (block * pagesPerBlock) + page);
				continue;
			}

			if(memcmp(buffer, _page, sizeof(_page)) == 0)
			{
				if(_buffer)
				{
					size_t amt = ((size_t*)buffer)[13];
					if(amt > _amt)
						amt = _amt;

					memcpy(_buffer, buffer + 0x38, amt);
				}

				free(buffer);
				return SUCCESS;
			}
			else
				DebugPrintf("vfl: did not find signature on ce %d, page %d\r\n", _ce, (block * pagesPerBlock) + page);
		}
	}

	free(buffer);
	return ENOENT;
}

error_t nand_device_enable_encryption(nand_device_t *_dev, int _enabled)
{
	if(_dev->enable_encryption)
		return _dev->enable_encryption(_dev, _enabled);

	return ENOENT;
}

error_t nand_device_enable_data_whitening(nand_device_t *_dev, int _enabled)
{
	if(_dev->enable_data_whitening)
		return _dev->enable_data_whitening(_dev, _enabled);

	return ENOENT;
}

error_t nand_device_set_ftl_region(nand_device_t *_dev, uint32_t _lpn, uint32_t _a2, uint32_t _count, void *_buf)
{
	if(_dev->set_ftl_region)
	{
		_dev->set_ftl_region(_lpn, _a2, _count, _buf);
		return SUCCESS;
	}

	return ENOENT;
}
