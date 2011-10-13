/*
 * vfl.c
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
#include "chipid.h"
#include "util.h"

// VFL types
#ifdef CONFIG_VFL_VFL
#include "vfl/vfl.h"
#endif

#ifdef CONFIG_VFL_VSVFL
#include "vfl/vsvfl.h"
#endif

void vfl_init(vfl_device_t *_vfl)
{
	memset(_vfl, 0, sizeof(*_vfl));
}

void vfl_cleanup(vfl_device_t *_vfl)
{
}

vfl_device_t *vfl_allocate()
{
	vfl_device_t *ret = malloc(sizeof(*ret));
	vfl_init(ret);
	return ret;
}

error_t vfl_open(vfl_device_t *_vfl, nand_device_t *_dev)
{
	if(!_vfl->open)
		return ENOENT;

	return _vfl->open(_vfl, _dev);
}

void vfl_close(vfl_device_t *_vfl)
{
	if(!_vfl->close)
		return;

	_vfl->close(_vfl);
}

error_t vfl_read_single_page(vfl_device_t *_vfl, uint32_t _page, uint8_t* buffer, uint8_t* spare,
		int empty_ok, int* refresh_page, uint32_t disable_aes)
{
	if(!_vfl->read_single_page)
		return ENOENT;

	return _vfl->read_single_page(_vfl, _page, buffer, spare, empty_ok, refresh_page, disable_aes);
}

error_t vfl_write_single_page(vfl_device_t *_vfl, uint32_t _page, uint8_t* buffer, uint8_t* spare,
		int _scrub)
{
	if(!_vfl->write_single_page)
		return ENOENT;

	return _vfl->write_single_page(_vfl, _page, buffer, spare, _scrub);
}

error_t vfl_erase_single_block(vfl_device_t *_vfl, uint32_t _block, int _replace_bad_block)
{
	if(!_vfl->erase_single_block)
		return ENOENT;

	return _vfl->erase_single_block(_vfl, _block, _replace_bad_block);
}

uint16_t *vfl_get_ftl_ctrl_block(vfl_device_t *_vfl)
{
	if(!_vfl->get_ftl_ctrl_block) {
		return NULL;
	}

	return _vfl->get_ftl_ctrl_block(_vfl);
}

error_t vfl_get_info(vfl_device_t *_vfl, vfl_info_t _item, void *_result, size_t _sz)
{
	if(!_vfl->get_info) {
		return ENOENT;
	}

	return _vfl->get_info(_vfl, _item, _result, _sz);
}

error_t vfl_detect(vfl_device_t **_vfl, nand_device_t *_nand, vfl_signature_style_t _sign)
{
	uint8_t sigbuf[264];

	error_t ret;
	switch(_sign)
	{
	case vfl_old_signature:
		// TODO: this. -- ricky26
		bufferPrintf("vfl: Old-style signature not supported.\r\n");
		return ENOENT;

	case vfl_new_signature:
		 ret = nand_device_read_special_page(_nand, 0, "NANDDRIVERSIGN\0\0", sigbuf, sizeof(sigbuf));
		 break;

	default:
		return EINVAL;
	}

	if(FAILED(ret))
		return ret;


	// Starting from iOS5 there's a change in behaviour at chipid_get_nand_epich().
	if((!chipid_get_nand_epoch() && sigbuf[0] != '1' && sigbuf[0] != '2') || sigbuf[3] != 'C'
			|| sigbuf[1] > '1' || sigbuf[2] > '1'
			 || sigbuf[4] > 6)
	{
		bufferPrintf("vfl: Incompatible signature.\r\n");
		return ENOENT;
	}

	uint32_t flags = *(uint32_t*)&sigbuf[4];
	if((_sign & 0x800) && (!(flags & 0x10000) ||
				((_sign >> 10) & 1) ||
				!((!(flags & 0x10000)) & ((_sign >> 10) & 1))
			))
	{
		bufferPrintf("vfl: warning: metadata whitening mismatch.\r\n");
	}

	if(sigbuf[1] == '1')
	{
		bufferPrintf("vfl: Detected VSVFL.\r\n");
#ifndef CONFIG_VFL_VSVFL
		bufferPrintf("vfl: VSVFL support not included!\r\n");
		return ENOENT;
#else
		*_vfl = &vfl_vsvfl_device_allocate()->vfl;

		if(_sign != vfl_new_signature)
			system_panic("vfl: VSVFL requires new signature!\r\n");

		int whitening = flags & 0x10000;
		if(FAILED(nand_device_enable_data_whitening(_nand, whitening))
				&& whitening)
			system_panic("vfl: Failed to enable data whitening!\r\n");
#endif
	}
	else if(sigbuf[1] == '0')
	{
		bufferPrintf("vfl: Detected old-style VFL.\r\n");
#ifndef CONFIG_VFL_VFL
		bufferPrintf("vfl: Standard VFL support not included!\r\n");
		return ENOENT;
#else
		*_vfl = &vfl_vfl_device_allocate()->vfl;
#endif
	}
	else
	{
		bufferPrintf("vfl: No valid VFL signature found!\r\n");
		return ENOENT;
	}

	if(!*_vfl)
		return ENOENT;

	return vfl_open(*_vfl, _nand);
}
