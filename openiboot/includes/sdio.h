/**
 * sdio.h
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

#ifndef SDIO_H
#define SDIO_H

#include "openiboot.h"
#include "interrupt.h"

void sdio_status();

int sdio_set_block_size(int function, int blocksize);
int sdio_claim_irq(int function, InterruptServiceRoutine handler, uint32_t token);
int sdio_enable_func(int function);
uint8_t sdio_readb(int function, uint32_t address, int* err_ret);
void sdio_writeb(int function, uint8_t val, uint32_t address, int* err_ret);
int sdio_writesb(int function, uint32_t address, void* src, int count);
int sdio_readsb(int function, void* dst, uint32_t address, int count);
int sdio_memcpy_toio(int function, uint32_t address, void* src, int count);
int sdio_memcpy_fromio(int function, void* dst, uint32_t address, int count);

#endif
