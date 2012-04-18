/*
 * mmu.h
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

#ifndef MMU_H
#define MMU_H

#include "openiboot.h"

#define MMU_SECTION_SIZE 0x100000
#define MMU_SECTION_MASK 0xFFF00000

#define MMU_SECTION 0x2
#define MMU_AP_NOACCESS 0x0
#define MMU_AP_PRIVILEGEDONLY 0xA00
#define MMU_AP_BOTH 0xB00
#define MMU_AP_BOTHWRITE 0xC00
#define MMU_EXECUTENEVER 0x10
#define MMU_CACHEABLE 0x8
#define MMU_BUFFERABLE 0x4

extern uint32_t* CurrentPageTable;

int mmu_setup();
void mmu_enable();
void mmu_disable();
void mmu_map_section(uint32_t section, uint32_t target, Boolean cacheable, Boolean bufferable);
void mmu_map_section_range(uint32_t rangeStart, uint32_t rangeEnd, uint32_t target, Boolean cacheable, Boolean bufferable);

typedef struct PhysicalAddressMap {
	uint32_t logicalStart;
	uint32_t logicalEnd;
	uint32_t physicalStart;
	uint32_t size;
} PhysicalAddressMap;

PhysicalAddressMap* get_address_map(uint32_t address, uint32_t* address_map_base);
uint32_t get_physical_address(uint32_t address);

#endif
