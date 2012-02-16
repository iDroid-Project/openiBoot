/**
 * framebuffer.h
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

#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include "openiboot.h"

#define COLOR_WHITE 0xffffff
#define COLOR_BLACK 0x0

typedef struct OpenIBootFont {
	uint32_t width;
	uint32_t height;
	uint8_t data[];
} OpenIBootFont;

extern int FramebufferHasInit;

int framebuffer_setup();
void framebuffer_setdisplaytext(int onoff);
int framebuffer_width();
int framebuffer_height();
int framebuffer_x();
int framebuffer_y();
void framebuffer_putc(int c);
void framebuffer_print(const char* str);
void framebuffer_print_force(const char* str);
void framebuffer_printf(const char* fmt, ...);
void framebuffer_setloc(int x, int y);
void framebuffer_clear();
uint32_t* framebuffer_load_image(const char* data, int len, int* width, int* height, int alpha);
void framebuffer_blend_image(uint32_t* dst, int dstWidth, int dstHeight, uint32_t* src, int srcWidth, int srcHeight, int x, int y);
void framebuffer_draw_image(uint32_t* image, int x, int y, int width, int height);
void framebuffer_draw_image_clip(uint32_t* image, int x, int y, int width, int height, int dw, int dh);
void framebuffer_capture_image(uint32_t* image, int x, int y, int width, int height);
void framebuffer_draw_rect(uint32_t color, int x, int y, int width, int height);
void framebuffer_fill_rect(uint32_t color, int x, int y, int width, int height);
void framebuffer_draw_rect_hgradient(int starting, int ending, int x, int y, int width, int height);
void framebuffer_setcolors(uint32_t fore, uint32_t back);

void framebuffer_hook();
void framebuffer_set_init();
void framebuffer_show_number(uint32_t number);

#endif
