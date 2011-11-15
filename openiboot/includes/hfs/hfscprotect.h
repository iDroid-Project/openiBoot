/**
 * hfs/hfscprotect.h
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

#ifndef HFSCPROTECT_H
#define HFSCPROTECT_H

typedef struct HFSPlusCprotectV2
{
	uint16_t xattr_version; //=2 (version?)
	uint16_t zero ; //=0
	uint32_t unknown; // leaks stack dword in one code path :)
	uint32_t protection_class_id;
	uint32_t wrapped_length; //0x28
	uint8_t wrapped_key[0x28];
} __attribute__ ((packed)) HFSPlusCprotectV2;

typedef struct HFSPlusCprotectV4
{
	uint16_t xattr_version; //=2 (version?)
	uint16_t zero ; //=0
	uint32_t xxx_length; // 0xc
	uint32_t protection_class_id;
	uint32_t wrapped_length; //0x28
	uint8_t xxx_junk[20]; //uninitialized ?
	uint8_t wrapped_key[0x28];
} __attribute__ ((packed)) HFSPlusCprotectV4;

#endif
