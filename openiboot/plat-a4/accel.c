/**
 * accel.c
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


#include "commands.h"
#include "hardware/accel.h"
#include "i2c.h"
#include "openiboot.h"
#include "util.h"

int accel_get_reg(int reg) {
	uint8_t registers[1];
	uint8_t out[1];

	registers[0] = reg;

	i2c_rx(ACCEL_I2C_BUS, ACCEL_GETADDR, registers, 1, out, 1);
	return out[0];
}

int accel_write_reg(int reg, int data, int verify) {
	uint8_t command[2];

	command[0] = reg;
	command[1] = data;

	i2c_tx(ACCEL_I2C_BUS, ACCEL_SETADDR, command, sizeof(command));

	if(!verify)
		return 0;

	uint8_t accelReg = reg;
	uint8_t buffer = 0;
	i2c_rx(ACCEL_I2C_BUS, ACCEL_GETADDR, &accelReg, 1, &buffer, 1);

	if(buffer == data)
		return 0;
	else
		return -1;
}

void accel_init()
{
	int whoami = accel_get_reg(ACCEL_WHOAMI);
	if(whoami != ACCEL_WHOAMI_VALUE)
	{
		bufferPrintf("accel: incorrect whoami value\n");
		return;
	}

	accel_write_reg(ACCEL_CTRL_REG2, ACCEL_CTRL_REG2_BOOT, FALSE);
	accel_write_reg(ACCEL_CTRL_REG1, ACCEL_CTRL_REG1_PM0 | ACCEL_CTRL_REG1_XEN | ACCEL_CTRL_REG1_YEN | ACCEL_CTRL_REG1_ZEN, FALSE);
}
MODULE_INIT(accel_init);

int accel_get_x()
{
	return (signed char)(accel_get_reg(ACCEL_OUTX));
}

int accel_get_y()
{
	return (signed char)(accel_get_reg(ACCEL_OUTY));
}

int accel_get_z()
{
	return (signed char)(accel_get_reg(ACCEL_OUTZ));
}

void cmd_accel(int argc, char** argv) {
	int x = accel_get_x();
	int y = accel_get_y();
	int z = accel_get_z();

	bufferPrintf("x: %d, y: %d, z: %d\r\n", x, y, z);
}
COMMAND("accel", "display accelerometer data", cmd_accel);
