#ifndef MIPIDSIM_H
#define MIPIDSIM_H

#include "openiboot.h"
#include "lcd.h"

typedef struct LCDInfo {
	char* name;
	uint32_t unkn1;
	uint32_t DrivingClockFrequency;
	uint32_t DotPitch;
	uint32_t width;
	uint32_t horizontalBackPorch;
	uint32_t horizontalFrontPorch;
	uint32_t horizontalSyncPulseWidth;
	uint32_t height;
	uint32_t verticalBackPorch;
	uint32_t verticalFrontPorch;
	uint32_t verticalSyncPulseWidth;
	uint32_t IVClk;
	uint32_t IHSync;
	uint32_t IVSync;
	uint32_t IVDen;
	uint32_t bitsPerPixel;
	uint32_t unkn17;
	uint32_t unkn18;
} LCDInfo;

int mipi_dsim_init(LCDInfo* LCDTable);
void mipi_dsim_quiesce();
void mipi_dsim_framebuffer_on_off(OnOff on_off);
int mipi_dsim_read_write(int a1, uint8_t* buffer, uint32_t* read);
int mipi_dsim_write_data(uint8_t data_id, uint8_t data0, uint8_t data1);

#endif
