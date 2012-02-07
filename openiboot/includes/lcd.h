/**
 *  @file 
 *
 *  Header file for OpeniBoot's LCD implementation.
 *
 *  This file defines LCD initialisation, shutdown and various control
 *  inplementations including backlight control.
 *
 *  This is a way of displaying things on the LCD screen, if you don't know
 *  what that is, please consider suicide it will save the world of your 
 *  idiocy.
 *
 *  @defgroup LCD
 */

/**
 * lcd.h
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

#ifndef LCD_H
#define LCD_H

#include "openiboot.h"
#include "clock.h"

typedef enum ColorSpace {
	RGB565 = 3,
	RGB888 = 6
} ColorSpace;

struct Framebuffer;

typedef void (*HLineFunc)(struct Framebuffer* framebuffer, int start, int line_no, int length, int fill);
typedef void (*VLineFunc)(struct Framebuffer* framebuffer, int start, int line_no, int length, int fill);

typedef struct Framebuffer {
	volatile uint32_t* buffer;
	uint32_t width;
	uint32_t height;
	uint32_t lineWidth;
	ColorSpace colorSpace;
	HLineFunc hline;
	VLineFunc vline;
} Framebuffer;

typedef struct Window {
	int created;
	int width;
	int height;
	uint32_t lineBytes;
	Framebuffer framebuffer;
	uint32_t* lcdConPtr;
	uint32_t lcdCon[2];
} Window;

typedef struct GammaTable {
	uint32_t data[18];
} GammaTable;

typedef struct GammaTableDescriptor {
	uint32_t panelIDMatch;
	uint32_t panelIDMask;
	GammaTable table0;
	GammaTable table1;
	GammaTable table2;
} GammaTableDescriptor;

extern int LCDInitRegisterCount;
extern const uint16_t* LCDInitRegisters;
extern uint32_t LCDPanelID;

extern Window* currentWindow;
extern volatile uint32_t* CurFramebuffer;
extern uint32_t NextFramebuffer;

int lcd_setup();
void lcd_fill(uint32_t color);
void lcd_shutdown();
void lcd_set_backlight_level(int level);
void lcd_window_address(int window, uint32_t framebuffer);

int displaypipe_init();
void pinot_quiesce();
void lcd_fill_switch(OnOff on_off, uint32_t color);

void framebuffer_hook();

#endif

