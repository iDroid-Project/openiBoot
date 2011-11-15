/**
 * pmu.h
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

#ifndef PMU_H
#define PMU_H

#include "openiboot.h"

typedef struct PMURegisterData {
	uint8_t reg;
	uint8_t data;
} PMURegisterData;

typedef enum PowerSupplyType {
	PowerSupplyTypeError,
	PowerSupplyTypeBattery,
	PowerSupplyTypeFirewire,
	PowerSupplyTypeUSBHost,
	PowerSupplyTypeUSBBrick500mA,
	PowerSupplyTypeUSBBrick1000mA,
	PowerSupplyTypeUSBBrick2000mA
} PowerSupplyType;

#define PMU_IBOOTSTATE 0xF
#define PMU_IBOOTDEBUG 0x0
#define PMU_IBOOTSTAGE 0x1
#define PMU_IBOOTERRORCOUNT 0x2
#define PMU_IBOOTERRORSTAGE 0x3

void pmu_poweroff();
void pmu_set_iboot_stage(uint8_t stage);
int pmu_get_gpmem_reg(int reg, uint8_t* out);
int pmu_set_gpmem_reg(int reg, uint8_t data);
int pmu_get_reg(int reg);
int pmu_get_regs(int reg, uint8_t* out, int count);
int pmu_write_reg(int reg, int data, int verify);
int pmu_write_regs(const PMURegisterData* regs, int num);
int pmu_get_battery_voltage();
PowerSupplyType pmu_get_power_supply();
void pmu_charge_settings(int UseUSB, int SuspendUSB, int StopCharger);
uint64_t pmu_get_epoch();
void epoch_to_date(uint64_t epoch, int* year, int* month, int* day, int* day_of_week, int* hour, int* minute, int* second);
void pmu_date(int* year, int* month, int* day, int* day_of_week, int* hour, int* minute, int* second);
const char* get_dayofweek_str(int day);

#endif
