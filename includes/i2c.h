/*
 * i2c.h
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

#ifndef I2C_H
#define I2C_H

#include "openiboot.h"
#include "clock.h"

typedef enum I2CError {
	I2CNoError = 0
} I2CError;

typedef enum I2CState {
	I2CDone = 0,
	I2CStart = 1,
	I2CSendRegister = 2,
	I2CRegistersDone = 3,
	I2CSetup = 4,
	I2CTx = 5,
	I2CRx = 6,
	I2CRxSetup = 7,
	I2CFinish = 8
} I2CState;


#if defined(CONFIG_A4)||defined(CONFIG_S5L8920)
typedef struct I2CInfo {
	uint32_t unkn0;
	uint32_t unkn1;
	int is_write;
	uint32_t address;
	int operation_result;
	I2CState state;
	const uint8_t* registers;
	int bufferLen;
	uint8_t* buffer;
	uint32_t iic_scl_gpio;
	uint32_t iic_sda_gpio;
	uint8_t interrupt;
	uint32_t register_0;
	uint32_t register_8;
	uint32_t register_C;
	uint32_t register_10;
	uint32_t register_14;
	uint32_t register_18;
	uint32_t register_20;
	uint32_t register_24;
	// This is NOT in iBoot. Added because it's easier to handle.
	uint8_t clockgate;
} I2CInfo;
#else
typedef struct I2CInfo {
	uint32_t field_0;
	uint32_t frequency;
	I2CError error_code;
	int is_write;
	int cursor;
	I2CState state;
	int operation_result;
	uint32_t address;
	uint32_t iiccon_settings;
	uint32_t current_iicstat;
	int num_regs;
	const uint8_t* registers;
	int bufferLen;
	uint8_t* buffer;
	uint32_t iic_scl_gpio;
	uint32_t iic_sda_gpio;
	uint32_t register_IICCON;
	uint32_t register_IICSTAT;
	uint32_t register_IICADD;
	uint32_t register_IICDS;
	uint32_t register_IICLC;
	uint32_t register_14;
	uint32_t register_18;
	uint32_t register_1C;
	uint32_t register_20;
} I2CInfo;
#endif

int i2c_setup();
I2CError i2c_rx(int bus, int iicaddr, const uint8_t* registers, int num_regs, void* buffer, int len);
I2CError i2c_tx(int bus, int iicaddr, void* buffer, int len);

#endif
