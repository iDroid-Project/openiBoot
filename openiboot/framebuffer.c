/*
 * framebuffer.c - OpeniBoot Framebuffer
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

#include "openiboot.h"
#include "commands.h"
#include "framebuffer.h"
#include "lcd.h"
#include "util.h"
#if defined(BIG_FONT)
#include "pcf/9x15.h"
#else
#include "pcf/6x10.h"
#endif
#include "arm/arm.h"

static int TWidth;
static int THeight;
static int X;
static int Y;
static OpenIBootFont* Font;
static printf_handler_t prev_printf_handler = NULL;

static uint32_t FBWidth;
static uint32_t FBHeight;

int FramebufferHasInit = FALSE;
static int DisplayText = FALSE;

static uint32_t BackgroundColor;
static uint32_t ForegroundColor;

#define BGR16(x) ((((((x) >> 16) & 0xFF) >> 3) << 11) | (((((x) >> 8) & 0xFF) >> 2) << 5) | (((x) & 0xFF) >> 3))
#define BGR32(x) ((((((x) >> 11) & 0x1F) << 3) << 16) | (((((x) >> 5) & 0x3F) << 2) << 8) | (((x) & 0x1F) << 3))

#define RGB16(x) (((((x) & 0xFF) >> 3) << 11) | (((((x) >> 8) & 0xFF) >> 2) << 5) | ((((x) >> 16) & 0xFF) >> 3))

#define RGBA2BGR(x) ((((x) >> 16) & 0xFF) | ((((x) >> 8) & 0xFF) << 8) | (((x) & 0xFF) << 16))

inline int getCharPixel(int ch, int x, int y) {
	register int bitIndex = ((fontWidth * fontHeight) * ch) + (fontWidth * y) + x;
	return (fontData[bitIndex / 8] >> (bitIndex % 8)) & 0x1;
}

inline volatile uint32_t* PixelFromCoords(register uint32_t x, register uint32_t y) {
	return CurFramebuffer + (y * FBWidth) + x;
}

inline volatile uint16_t* PixelFromCoords565(register uint32_t x, register uint32_t y) {
	return ((uint16_t*)CurFramebuffer) + (y * FBWidth) + x;
}

void framebuffer_printf_handler(const char *text)
{
	if(FramebufferHasInit)
		framebuffer_print(text);

	if(prev_printf_handler)
		prev_printf_handler(text);
}

int framebuffer_setup() {
	Font = (OpenIBootFont*)fontData; //(OpenIBootFont*) malloc(sizeof(OpenIBootFont)); // Someone explain why Bluerise did this -- Ricky26
	BackgroundColor = COLOR_BLACK;
	ForegroundColor = COLOR_WHITE;
	FBWidth = currentWindow->framebuffer.width;
	FBHeight = currentWindow->framebuffer.height;
	TWidth = FBWidth / fontWidth;
	THeight = FBHeight / fontHeight;
	framebuffer_clear();
	prev_printf_handler = addPrintfHandler(framebuffer_printf_handler);
	FramebufferHasInit = TRUE;
	return 0;
}

void framebuffer_setcolors(uint32_t fore, uint32_t back) {
	ForegroundColor = fore;
	BackgroundColor = back;
}

void framebuffer_setdisplaytext(int onoff) {
	DisplayText = onoff;
}

void framebuffer_clear() {
	lcd_fill(BackgroundColor);
	X = 0;
	Y = 0;
}

int framebuffer_width() {
	return FBWidth;
}

int framebuffer_height() {
	return FBHeight;
}

int framebuffer_x() {
	return X;
}

int framebuffer_y() {
	return Y;
}

void framebuffer_setloc(int x, int y) {
	X = x;
	Y = y;
}

void framebuffer_print(const char* str) {
	if(!DisplayText)
		return;

	size_t len = strlen(str);
	int i;
	for(i = 0; i < len; i++) {
		framebuffer_putc(str[i]);
	}
}

void framebuffer_printf(const char* format, ...)
{
	static char buffer[1000];
	va_list args;

	buffer[0] = '\0';
	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);
	framebuffer_print(buffer);
}

void framebuffer_print_force(const char* str) {
	size_t len = strlen(str);
	int i;
	for(i = 0; i < len; i++) {
		framebuffer_putc(str[i]);
	}
}

static void scrollup888() {
	register volatile uint32_t* newFirstLine = PixelFromCoords(0, fontHeight);
	register volatile uint32_t* oldFirstLine = PixelFromCoords(0, 0);
	register volatile uint32_t* end = oldFirstLine + (FBWidth * FBHeight);
	while(newFirstLine < end) {
		*(oldFirstLine++) = *(newFirstLine++);
	}
	while(oldFirstLine < end) {
		*(oldFirstLine++) = BackgroundColor;
	}
	Y--;
	//Y = 0;
}

static void scrollup565() {
	register volatile uint16_t* newFirstLine = PixelFromCoords565(0, fontHeight);
	register volatile uint16_t* oldFirstLine = PixelFromCoords565(0, 0);
	register volatile uint16_t* end = oldFirstLine + (FBWidth * FBHeight);
	uint16_t bgcolor = BGR16(BackgroundColor);
	while(newFirstLine < end) {
		*(oldFirstLine++) = *(newFirstLine++);
	}
	while(oldFirstLine < end) {
		*(oldFirstLine++) = bgcolor;
	}
	Y--;
	//Y = 0;
}

void framebuffer_putc888(int c) {
	if(c == '\r') {
		X = 0;
	} else if(c == '\n') {
		X = 0;
		Y++;
	} else {
		register uint32_t sx;
		register uint32_t sy;
		for(sy = 0; sy < fontHeight; sy++) {
			for(sx = 0; sx < fontWidth; sx++) {
				if(getCharPixel(c, sx, sy)) {
					*(PixelFromCoords(sx + (fontWidth * X), sy + (fontHeight * Y))) = ForegroundColor;
				} else {
					*(PixelFromCoords(sx + (fontWidth * X), sy + (fontHeight * Y))) = BackgroundColor;
				}
			}
		}

		X++;
	}

	if(X == TWidth) {
		X = 0;
		Y++;
	}

	if(Y == THeight) {
		scrollup888();
	}
}

void framebuffer_putc565(int c) {
	uint16_t fgcolor = BGR16(ForegroundColor);
	uint16_t bgcolor = BGR16(BackgroundColor);

	if(c == '\r') {
		X = 0;
	} else if(c == '\n') {
		X = 0;
		Y++;
	} else {
		register uint32_t sx;
		register uint32_t sy;
		for(sy = 0; sy < fontHeight; sy++) {
			for(sx = 0; sx < fontWidth; sx++) {
				if(getCharPixel(c, sx, sy)) {
					*(PixelFromCoords565(sx + (fontWidth * X), sy + (fontHeight * Y))) = fgcolor;
				} else {
					*(PixelFromCoords565(sx + (fontWidth * X), sy + (fontHeight * Y))) = bgcolor;
				}
			}
		}

		X++;
	}

	if(X == TWidth) {
		X = 0;
		Y++;
	}

	if(Y == THeight) {
		scrollup565();
	}
}

void framebuffer_putc(int c)
{
	if(currentWindow->framebuffer.colorSpace == RGB888)
		framebuffer_putc888(c);
	else
		framebuffer_putc565(c);
}

static void framebuffer_draw_image888(uint32_t* image, int x, int y, int width, int height) {
	register uint32_t sx;
	register uint32_t sy;
	for(sy = 0; sy < height; sy++) {
		for(sx = 0; sx < width; sx++) {
			*(PixelFromCoords(sx + x, sy + y)) = RGBA2BGR(image[(sy * width) + sx]);
		}
	}
}

static void framebuffer_draw_image565(uint32_t* image, int x, int y, int width, int height) {
	register uint32_t sx;
	register uint32_t sy;
	for(sy = 0; sy < height; sy++) {
		for(sx = 0; sx < width; sx++) {
			*(PixelFromCoords565(sx + x, sy + y)) = RGB16(image[(sy * width) + sx]);
		}
	}
}

void framebuffer_draw_image(uint32_t* image, int x, int y, int width, int height)
{
	if(currentWindow->framebuffer.colorSpace == RGB888)
		framebuffer_draw_image888(image, x, y, width, height);
	else
		framebuffer_draw_image565(image, x, y, width, height);
}

static void framebuffer_draw_image888_clip(uint32_t* image, int x, int y, int width, int height, int dw, int dh) {
	register uint32_t sx;
	register uint32_t sy;
	for(sy = 0; sy < height; sy++) {
		for(sx = 0; sx < width; sx++) {
			*(PixelFromCoords(sx + x, sy + y)) = RGBA2BGR(image[(sy * width) + sx]);
		}
	}
}

static void framebuffer_draw_image565_clip(uint32_t* image, int x, int y, int width, int height, int dw, int dh) {
	register uint32_t sx;
	register uint32_t sy;

	for(sy = 0; sy < dh; sy++) {
		for(sx = 0; sx < dw; sx++) {
			*(PixelFromCoords565(sx + x, sy + y)) = RGB16(image[(sy * width) + sx]);
		}
	}
}

void framebuffer_draw_image_clip(uint32_t* image, int x, int y, int width, int height, int dw, int dh)
{
	if(currentWindow->framebuffer.colorSpace == RGB888)
		framebuffer_draw_image888_clip(image, x, y, width, height, dw, dh);
	else
		framebuffer_draw_image565_clip(image, x, y, width, height, dw, dh);
}

static void framebuffer_capture_image888(uint32_t* image, int x, int y, int width, int height) {
	register uint32_t sx;
	register uint32_t sy;
	for(sy = 0; sy < height; sy++) {
		for(sx = 0; sx < width; sx++) {
			image[(sy * width) + sx] = *(PixelFromCoords(sx + x, sy + y));
		}
	}
}

static void framebuffer_capture_image565(uint32_t* image, int x, int y, int width, int height) {
	register uint32_t sx;
	register uint32_t sy;
	for(sy = 0; sy < height; sy++) {
		for(sx = 0; sx < width; sx++) {
			image[(sy * width) + sx] = BGR32(*(PixelFromCoords565(sx + x, sy + y)));
		}
	}
}

void framebuffer_capture_image(uint32_t* image, int x, int y, int width, int height)
{
	if(currentWindow->framebuffer.colorSpace == RGB888)
		framebuffer_capture_image888(image, x, y, width, height);
	else
		framebuffer_capture_image565(image, x, y, width, height);
}

void framebuffer_draw_rect(uint32_t color, int x, int y, int width, int height) {
	currentWindow->framebuffer.hline(&currentWindow->framebuffer, x, y, width, color);
	currentWindow->framebuffer.hline(&currentWindow->framebuffer, x, y + height, width, color);
	currentWindow->framebuffer.vline(&currentWindow->framebuffer, y, x, height, color);
	currentWindow->framebuffer.vline(&currentWindow->framebuffer, y, x + width, height, color);
}

void framebuffer_fill_rect(uint32_t color, int x, int y, int width, int height) {
	int i;
	for(i = 0; i < height; i++)
		currentWindow->framebuffer.hline(&currentWindow->framebuffer, x, y+i, width, color);
}

#ifndef SMALL
#ifndef NO_STBIMAGE
#include "stb_image.h"

uint32_t* framebuffer_load_image(const char* data, int len, int* width, int* height, int alpha) {
	int components;
	uint32_t* stbiData = (uint32_t*) stbi_load_from_memory((stbi_uc const*)data, len, width, height, &components, 4);
	if(!stbiData) {
		bufferPrintf("framebuffer: %s\r\n", stbi_failure_reason());
		return NULL;
	}
	return stbiData;
}
#endif
#endif

void framebuffer_blend_image(uint32_t* dst, int dstWidth, int dstHeight, uint32_t* src, int srcWidth, int srcHeight, int x, int y) {
	register uint32_t sx;
	register uint32_t sy;
	for(sy = 0; sy < srcHeight; sy++) {
		for(sx = 0; sx < srcWidth; sx++) {
			register uint32_t* dstPixel = &dst[((sy + y) * dstWidth) + (sx + x)];
			register uint32_t* srcPixel = &src[(sy * srcWidth) + sx];
			*dstPixel =
				((((*dstPixel & 0xFF) * (0x100 - (*srcPixel >> 24))) >> 8) + ((((*srcPixel >> 16) & 0xFF) * ((*srcPixel >> 24) + 1)) >> 8)) << 16
				| (((((*dstPixel >> 8) & 0xFF) * (0x100 - (*srcPixel >> 24))) >> 8) + ((((*srcPixel >> 8) & 0xFF) * ((*srcPixel >> 24) + 1)) >> 8)) << 8
				| (((((*dstPixel >> 16) & 0xFF) * (0x100 - (*srcPixel >> 24))) >> 8) + (((*srcPixel & 0xFF) * ((*srcPixel >> 24) + 1)) >> 8));
		}
	}
}	

void framebuffer_draw_rect_hgradient(int starting, int ending, int x, int y, int width, int height) {
	int step = (ending - starting) * 1000 / height;
	int level = starting * 1000;
	int i;
	for(i = 0; i < height; i++) {
		int color = level / 1000;
		currentWindow->framebuffer.hline(&currentWindow->framebuffer, x, y + i, width, (color & 0xFF) | ((color & 0xFF) << 8) | ((color & 0xFF) << 16));
		level += step;
	}
}

static error_t cmd_text(int argc, char** argv)
{
	if(argc < 2) {
		bufferPrintf("Usage: %s <on|off>\r\n", argv[0]);
		return EINVAL;
	}

	if(strcmp(argv[1], "on") == 0)
	{
		framebuffer_setdisplaytext(ON);
		bufferPrintf("Text display ON\r\n");
		return SUCCESS;
	}
	else if(strcmp(argv[1], "off") == 0)
	{
		framebuffer_setdisplaytext(OFF);
		bufferPrintf("Text display OFF\r\n");
		return SUCCESS;
	}
	else
	{
		bufferPrintf("Unrecognized option: %s\r\n", argv[1]);
		return EINVAL;
	}
}
COMMAND("text", "turns text display on or off", cmd_text);

static error_t cmd_bgcolor(int argc, char** argv)
{
	if(argc < 4) {
		bufferPrintf("Usage: %s <red> <green> <blue>\r\n", argv[0]);
		return EINVAL;
	}

	uint8_t red = parseNumber(argv[1]);
	uint8_t green = parseNumber(argv[2]);
	uint8_t blue = parseNumber(argv[3]);

	lcd_fill((red << 16) | (green << 8) | blue);

	return 0;
}
COMMAND("bgcolor", "fill the framebuffer with a color", cmd_bgcolor);

static error_t cmd_clear(int argc, char** argv)
{
	framebuffer_clear();
	return 0;
}
COMMAND("clear", "clears the screen", cmd_clear);
