#include "openiboot.h"
#include "hardware/lcd.h"
#include "lcd.h"
#include "util.h"
#include "framebuffer.h"

Window* currentWindow;

volatile uint32_t* CurFramebuffer;

//static int numWindows = 0;
uint32_t NextFramebuffer = 0;

static int calculateStrideLen(ColorSpace colorSpace, int extra, int width) {
	int nibblesPerPixel;

	switch(colorSpace) {
		case RGB565:
			nibblesPerPixel = 4;
			break;
		case RGB888:
			nibblesPerPixel = 8;
			break;
		default:
			return -1;
	}

	int ddPerLine = (nibblesPerPixel * width)/(sizeof(uint32_t) * 2);

	if(ddPerLine & 0x3) {
		// ddPerLine is not a multiple of 4
		return (sizeof(uint32_t) - (ddPerLine & 0x3));
	} else if((nibblesPerPixel * width) & 0x7) {
		return 4;
	} else {
		// already aligned
		return 0;
	}
}

static void framebuffer_fill(Framebuffer* framebuffer, int x, int y, int width, int height, int fill) {
	if(x >= framebuffer->width)
		return;

	if(y >= framebuffer->height)
		return;

	if((x + width) > framebuffer->width) {
		width = framebuffer->width - x;
	}

	int maxLine;
	if((y + height) > framebuffer->height) {
		maxLine = framebuffer->height;
	} else {
		maxLine = y + height;
	}

	int line;
	for(line = y; line < maxLine; line++) {
		framebuffer->hline(framebuffer, x, line, width, fill);
	}
}

void lcd_fill(uint32_t color) {
	framebuffer_fill(&currentWindow->framebuffer, 0, 0, currentWindow->framebuffer.width, currentWindow->framebuffer.height, color);
}

static void hline_rgb888(Framebuffer* framebuffer, int start, int line_no, int length, int fill) {
	int i;
	volatile uint32_t* line;

	fill = fill & 0xffffff;	// no alpha
	line = &framebuffer->buffer[line_no * framebuffer->lineWidth];

	int stop = start + length;
	for(i = start; i < stop; i++) {
		line[i] = fill;
	}
}

static void vline_rgb888(Framebuffer* framebuffer, int start, int line_no, int length, int fill) {
	int i;
	volatile uint32_t* line;

	fill = fill & 0xffffff;	// no alpha
	line = &framebuffer->buffer[line_no];

	int stop = start + length;
	for(i = start; i < stop; i++) {
		line[i * framebuffer->lineWidth] = fill;
	}
}

static void hline_rgb565(Framebuffer* framebuffer, int start, int line_no, int length, int fill) {
	int i;
	volatile uint16_t* line;
	uint16_t fill565;

	fill565= ((((fill >> 16) & 0xFF) >> 3) << 11) | ((((fill >> 8) & 0xFF) >> 2) << 5) | ((fill & 0xFF) >> 3);
	line = &((uint16_t*)framebuffer->buffer)[line_no * framebuffer->lineWidth];

	int stop = start + length;
	for(i = start; i < stop; i++) {
		line[i] = fill565;
	}
}

static void vline_rgb565(Framebuffer* framebuffer, int start, int line_no, int length, int fill) {
	int i;
	volatile uint16_t* line;
	uint16_t fill565;

	fill565= ((((fill >> 16) & 0xFF) >> 3) << 11) | ((((fill >> 8) & 0xFF) >> 2) << 5) | ((fill & 0xFF) >> 3);
	line = &((uint16_t*)framebuffer->buffer)[line_no];

	int stop = start + length;
	for(i = start; i < stop; i++) {
		line[i * framebuffer->lineWidth] = fill565;
	}
}

/*static void setWindow(int window, int wordSetting, int zero1, ColorSpace colorSpace, int width1, uint32_t framebuffer, int zero2, int width2, int zero3, int height) {
	int nibblesPerPixel;

	switch(colorSpace) {
		case RGB565:
			nibblesPerPixel = 4;
			break;
		case RGB888:
			nibblesPerPixel = 8;
			break;
		default:
			return;
	}

	int x = (((width2 * nibblesPerPixel) >> 3) - ((zero2 * nibblesPerPixel) >> 3)) & 0x3;
	int stride;
	if(x) {
		stride = 4 - x;
	} else if((width2 * nibblesPerPixel) & 0x7) {
		stride = 4;
	} else {
		stride = 0;
	}

	uint32_t sfn = ((stride & 0x7) << 24) | ((wordSetting & 0x7) << 16) | ((zero1 & 0x3) << 12) | ((colorSpace & 0x7) << 8) | ((zero2 * nibblesPerPixel) & 0x7);
	uint32_t addr = (zero3 * ((((width1 * nibblesPerPixel) & 0x7) ? (((width1 * nibblesPerPixel) >> 3) + 1) : ((width1 * nibblesPerPixel) >> 3)) << 2)) +  framebuffer + (((zero2 * nibblesPerPixel) >> 3) << 2);
	uint32_t size = (height - zero3) | ((width2 - zero2) << 16);
	uint32_t hspan = (((width1 * nibblesPerPixel) & 0x7) ? (((width1 * nibblesPerPixel) >> 3) + 1) : ((width1 * nibblesPerPixel) >> 3)) << 2;
	uint32_t qlen = ((width2 * nibblesPerPixel) & 0x7) ? (((width2 * nibblesPerPixel) >> 3) + 1) : ((width2 * nibblesPerPixel) >> 3);
	bufferPrintf("SFN: 0x%x, Addr: 0x%x, Size: 0x%x, hspan: 0x%x, QLEN: 0x%x\r\n", sfn, addr, size, hspan, qlen);

	uint32_t windowBase;
	switch(window) {
		case 1:
			windowBase = LCD + 0x58;
			break;
		case 2:
			windowBase = LCD + 0x70;
			break;
		case 3:
			windowBase = LCD + 0x88;
			break;
		case 4:
			windowBase = LCD + 0xA0;
			break;
		case 5:
			windowBase = LCD + 0xB8;
			break;
		default:
			return;
	}

	SET_REG(windowBase + 4, sfn);
	SET_REG(windowBase + 8, addr);
	SET_REG(windowBase + 12, size);
	SET_REG(windowBase, hspan);
	SET_REG(windowBase + 16, qlen);
}*/

void lcd_window_address(int window, uint32_t framebuffer) {
	uint32_t windowBase;
	switch(window) {
		case 1:
			windowBase = LCD + 0x58;
			break;
		case 2:
			windowBase = LCD + 0x70;
			break;
		case 3:
			windowBase = LCD + 0x88;
			break;
		case 4:
			windowBase = LCD + 0xA0;
			break;
		case 5:
			windowBase = LCD + 0xB8;
			break;
		default:
			return;
	}

	SET_REG(windowBase + 8, framebuffer);
}

/*static void setLayer(int window, int zero0, int zero1) {
	uint32_t data = zero0 << 16 | zero1;
	switch(window) {
		case 1:
			SET_REG(LCD + 0x6C, data);
			break;
		case 2:
			SET_REG(LCD + 0x84, data);
			break;
		case 3:
			SET_REG(LCD + 0x9C, data);
			break;
		case 4:
			SET_REG(LCD + 0xB4, data);
			break;
		case 5:
			SET_REG(LCD + 0xCC, data);
			break;
	}
}*/



static void createFramebuffer(Framebuffer* framebuffer, uint32_t framebufferAddr, int width, int height, int lineWidth, ColorSpace colorSpace) {
	framebuffer->buffer = (uint32_t*) framebufferAddr;
	framebuffer->width = width;
	framebuffer->height = height;
	framebuffer->lineWidth = lineWidth;
	framebuffer->colorSpace = colorSpace;
	if(colorSpace == RGB888)
	{
		framebuffer->hline = hline_rgb888;
		framebuffer->vline = vline_rgb888;
	} else
	{
		framebuffer->hline = hline_rgb565;
		framebuffer->vline = vline_rgb565;
	}
}

/*static Window* createWindow(int zero0, int zero1, int width, int height, ColorSpace colorSpace) {
	Window* newWindow;
	int currentWindowNo = numWindows;

	if(currentWindowNo > 5)
		return NULL;

	numWindows++;

	newWindow = (Window*) malloc(sizeof(Window));
	newWindow->created = FALSE;
	newWindow->lcdConPtr = newWindow->lcdCon;
	newWindow->lcdCon[0] = currentWindowNo;
	newWindow->lcdCon[1] = (1 << (5 - currentWindowNo));
	newWindow->width = width;
	newWindow->height = height;

	int lineWidth = width + calculateStrideLen(colorSpace, 0, width);
	int lineBytes;

	switch(colorSpace) {
		case RGB565:
			lineBytes = lineWidth * 2;
			break;
		case RGB888:
			lineBytes = lineWidth * 4;
			break;
		default:
			return NULL;
	}

	newWindow->lineBytes = lineBytes;

	uint32_t currentFramebuffer = NextFramebuffer;

	NextFramebuffer = (currentFramebuffer
		+ (lineBytes * height)	// size we need
		+ 0xFFF)		// round up
		& 0xFFFFF000;		// align

	createFramebuffer(&newWindow->framebuffer, currentFramebuffer, width, height, lineWidth, colorSpace);

	if(colorSpace == RGB565)
		setWindow(currentWindowNo, 1, 0, colorSpace, width, currentFramebuffer, 0, width, 0, height);
	else
		setWindow(currentWindowNo, 0, 0, colorSpace, width, currentFramebuffer, 0, width, 0, height);

	setLayer(currentWindowNo, zero0, zero1);

	SET_REG(LCD + LCD_CON2, GET_REG(LCD + LCD_CON2) | ((newWindow->lcdConPtr[1] & 0x3F) << 2));

	newWindow->created = TRUE;

	return newWindow;
}*/

void framebuffer_hook() {
        Window* newWindow;
        newWindow = (Window*) malloc(sizeof(Window));
	memset(newWindow, 0, sizeof(Window));
        newWindow->created = FALSE;
	#if defined(CONFIG_IPAD)
        newWindow->width = 1024;
        newWindow->height = 768;
	#else
        newWindow->width = 640;
        newWindow->height = 960;
	#endif
        newWindow->lineBytes = (newWindow->width + calculateStrideLen(RGB888, 0, newWindow->width))*4;
        createFramebuffer(&newWindow->framebuffer, 0x5F700000, newWindow->width, newWindow->height, newWindow->lineBytes/4, RGB888);
	newWindow->created = TRUE;
        currentWindow = newWindow;
	CurFramebuffer = currentWindow->framebuffer.buffer;

	framebuffer_setup();
}

void framebuffer_show_number(uint32_t number) {
	int shift;
	for (shift = 31; shift >= 0; shift--) {
		int x = 0;
		int y;
		if (shift < 16) {
			x = 320;
			y = 15 - shift;
		} else {
			y = 31 - shift;
		}
		y *= 60;
		if (((number>>shift) & 0x1) == 1) {
			framebuffer_fill(&currentWindow->framebuffer, x, y, 320, 59, 0xFF1493);
		} else {
			framebuffer_fill(&currentWindow->framebuffer, x, y, 320, 59, 0x90EE90);
		}
		framebuffer_fill(&currentWindow->framebuffer, x, y+59, 320, 2, 0x0);
	}
	framebuffer_fill(&currentWindow->framebuffer, 320, 0, 1, 960, 0x0);
}
