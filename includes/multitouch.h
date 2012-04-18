/*
 * multitouch.h
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

#ifndef MULTITOUCH_H
#define MULTITOUCH_H

#include "openiboot.h"

typedef struct MTFrameHeader
{
	uint8_t type;
	uint8_t frameNum;
	uint8_t headerLen;
	uint8_t unk_3;
	uint32_t timestamp;
	uint8_t unk_8;
	uint8_t unk_9;
	uint8_t unk_A;
	uint8_t unk_B;
	uint16_t unk_C;
	uint16_t isImage;

	uint8_t numFingers;
	uint8_t fingerDataLen;
	uint16_t unk_12;
	uint16_t unk_14;
	uint16_t unk_16;
} MTFrameHeader;

typedef struct FingerData
{
	uint8_t id;
	uint8_t event;
	uint8_t unk_2;
	uint8_t unk_3;
	int16_t x;
	int16_t y;
	int16_t velX;
	int16_t velY;
	uint16_t radius2;
	uint16_t radius3;
	uint16_t angle;
	uint16_t radius1;
	uint16_t contactDensity;
	uint16_t unk_16;
	uint16_t unk_18;
	uint16_t unk_1A;
} FingerData;

#ifdef CONFIG_IPHONE_2G
int multitouch_setup(const uint8_t* ASpeedFirmware, int ASpeedFirmwareLen, const uint8_t* mainFirmware, int mainFirmwareLen);
#else
int multitouch_setup(const uint8_t* constructedFirmware, int constructedFirmwareLen);
#endif

void multitouch_on();
void multitouch_run();
int multitouch_ispoint_inside_region(uint16_t x, uint16_t y, int w, int h);
#endif
