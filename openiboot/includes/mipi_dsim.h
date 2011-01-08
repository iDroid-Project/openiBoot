#ifndef MIPIDSIM_H
#define MIPIDSIM_H

#include "openiboot.h"
#include "lcd.h"

int mipi_dsim_init(LCDInfo* LCDTable);
void mipi_dsim_quiesce();
void mipi_dsim_framebuffer_on_off(OnOff on_off);
int mipi_dsim_read_write(int a1, uint8_t* buffer, uint32_t* read);
int mipi_dsim_write_data(uint8_t data_id, uint8_t data0, uint8_t data1);

#endif
