#include "openiboot.h"
#include "util.h"
#include "mipi_dsim.h"
#include "hardware/mipi_dsim.h"
#include "timer.h"
#include "tasks.h"
#include "gpio.h"
#include "openiboot-asmhelpers.h"

static int mipi_dsim_has_init = 0;

void mipi_dsim_framebuffer_on_off(OnOff on_off) {
	if (on_off) {
		SET_REG(MIPI_DSIM + CLKCTRL, GET_REG(MIPI_DSIM + CLKCTRL) | 1 << 31);
		while (!(GET_REG(MIPI_DSIM + STATUS) & STATUS_HS_READY));
	} else {
		SET_REG(MIPI_DSIM + CLKCTRL, GET_REG(MIPI_DSIM + CLKCTRL) & (~(1 << 31)));
		while (GET_REG(MIPI_DSIM + STATUS) & STATUS_HS_READY);
	}
}

int mipi_dsim_write_data(uint8_t data_id, uint8_t data0, uint8_t data1) {
	SET_REG(MIPI_DSIM + PKTHDR, PKTHDR_DATA_ID(data_id) | PKTHDR_DATA0(data0)
		| PKTHDR_DATA1(data1));
	while ((GET_REG(MIPI_DSIM + FIFOCTRL) & FIFOCTRL_UNKN_1) != FIFOCTRL_UNKN_1);
	return 0;
}

int mipi_dsim_read_write(int a1, uint8_t* buffer, uint32_t* read) {
	int i;
	uint32_t to_read = *read;
	SET_REG(MIPI_DSIM + INTSRC, 0xFFFFFFFF);
	mipi_dsim_write_data(a1, *buffer, *(buffer+1));

	for (i = 0; i < 5; i++) {
		if (GET_REG(MIPI_DSIM + INTSRC) & 0x10000 || GET_REG(MIPI_DSIM + INTSRC) & 0x200000)
			return -1;
		if (GET_REG(MIPI_DSIM + INTSRC) & 0x40000)
			break;
		if (i == 4) {
			bufferPrintf("mipi: timing out\r\n");
			return -1;
		}
		udelay(50000);
	}
	while (GET_REG(MIPI_DSIM + FIFOCTRL) & 0x1000000);

	uint32_t read_buffer = GET_REG(MIPI_DSIM + RXFIFO);

	if ((read_buffer & 0x3F) != 26 && (read_buffer & 0x3F) != 28) {
		bufferPrintf("mipi: weird response %08x\r\n", GET_REG(MIPI_DSIM + RXFIFO));
		return -1;
	}

	uint32_t readable_size = (read_buffer >> 8) & 0xFFFF;

	if (to_read > readable_size)
		*read = readable_size;

	for (i = 0; i != readable_size; ++i) {
		if (!(i%4)) {
			while (GET_REG(MIPI_DSIM + FIFOCTRL) & 0x1000000);
			read_buffer = GET_REG(MIPI_DSIM + RXFIFO);
		}

		if (i < to_read)
			*(buffer+i) = (uint8_t)read_buffer;

		read_buffer = read_buffer >> 8;
	}

	bufferPrintf("mipi: read %d bytes.\n", *read);
	return 0;
}

int mipi_dsim_init(LCDInfo* LCDTable) {
	int result;
	uint32_t some_value;
	uint8_t numDataLanes = LCDTable->unkn18 & 0xF;
	uint32_t dataLanesEnabled = (1 << numDataLanes) - 1;

	uint32_t colourMode = (LCDTable->bitsPerPixel > 18) ? 7 : 5;

	bufferPrintf("mipi: data lines %d 0x%08x.\r\n", numDataLanes, dataLanesEnabled);

	bufferPrintf("mipi_dsim_init()\r\n");

	some_value = (GET_BITS(LCDTable->unkn18, 26, 6) << 13) | (GET_BITS(LCDTable->unkn18, 16, 9) << 4) | (GET_BITS(LCDTable->unkn18, 11, 4) & 0xE);
	clock_gate_switch(MIPI_DSIM_CLOCKGATE, ON);

	bufferPrintf("a\n");

	if (some_value)
	{
		SET_REG(MIPI_DSIM + CLKCTRL, CLKCTRL_ESC_PRESCALER(LCDTable->unkn18 >> 4) | CLKCTRL_ESC_CLKEN);
		SET_REG(MIPI_DSIM + PLLCTRL, (LCDTable->unkn18 << 16) & 0xF000000);
		SET_REG(MIPI_DSIM + PLLTMR, 300000);
		SET_REG(MIPI_DSIM + PLLCTRL, GET_REG(MIPI_DSIM + PLLCTRL) | some_value);
		SET_REG(MIPI_DSIM + PLLCTRL, GET_REG(MIPI_DSIM + PLLCTRL) | 0x800000);

		bufferPrintf("b\n");

		while ((GET_REG(MIPI_DSIM + STATUS) & STATUS_PLL_STABLE) != STATUS_PLL_STABLE);
		
		bufferPrintf("c\n");
	}
	else
	{
		SET_REG(MIPI_DSIM + CLKCTRL, CLKCTRL_ESC_PRESCALER(LCDTable->unkn18 >> 4) | CLKCTRL_ESC_CLKEN
			| CLKCTRL_PLL_BYPASS | CLKCTRL_BYTE_CLK_SRC);
		SET_REG(MIPI_DSIM + PLLCTRL, (LCDTable->unkn18 << 16) & 0xF000000);
	
		bufferPrintf("d\n");
	}

	bufferPrintf("e\n");
	
	SET_REG(MIPI_DSIM + SWRST, SWRST_RESET);
	while((GET_REG(MIPI_DSIM + STATUS) & STATUS_SWRST) != STATUS_SWRST);
	
	bufferPrintf("f\n");

	SET_REG(MIPI_DSIM + MDRESOL, DRESOL_VRESOL(LCDTable->height) | DRESOL_HRESOL(LCDTable->width) | DRESOL_STAND_BY);

	SET_REG(MIPI_DSIM + MVPORCH, MVPORCH_VFP(LCDTable->verticalFrontPorch) | MVPORCH_VBP(LCDTable->verticalBackPorch)
		| MVPORCH_CMD_ALLOW(0xD)); //TODO: figure out what the 0xD does, or at least make a constant for it
	SET_REG(MIPI_DSIM + MSYNC, MSYNC_VSPW(LCDTable->verticalSyncPulseWidth));

	SET_REG(MIPI_DSIM + MHPORCH, MHPORCH_HFP(0xF) | MHPORCH_HBP(0xE));
	SET_REG(MIPI_DSIM + MSYNC, GET_REG(MIPI_DSIM + MSYNC) | 1);

	SET_REG(MIPI_DSIM + SDRESOL, DRESOL_VRESOL(0) | DRESOL_HRESOL(10));     //TODO: does this just turn the sub display resolution off (enable bit isn't set)?

	SET_REG(MIPI_DSIM + CONFIG, 
			CONFIG_EN_DATA(DATA_LANES_ENABLED) | CONFIG_NUM_DATA(NUM_DATA_LANES)
			| CONFIG_MAIN_PXL_FMT(colourMode)
		   	| CONFIG_HSE_MODE | CONFIG_AUTO_MODE | CONFIG_VIDEO_MODE | CONFIG_BURST_MODE
		   	| CONFIG_EN_CLK);

	SET_REG(MIPI_DSIM + CLKCTRL, GET_REG(MIPI_DSIM + CLKCTRL) | CLKCTRL_BYTE_CLKEN | CLKCTRL_ESC_EN_CLK
		| CLKCTRL_ESC_EN_DATA(DATA_LANES_ENABLED));

	SET_REG(MIPI_DSIM + FIFOCTRL, FIFOCTRL_MAIN_DISP_FIFO | FIFOCTRL_SUB_DISP_FIFO | FIFOCTRL_I80_FIFO
		| FIFOCTRL_TX_SFR_FIFO | FIFOCTRL_RX_FIFO);
	SET_REG(MIPI_DSIM + FIFOTHLD, 0x1ff);   //TODO: figure out what this does

	SET_REG(MIPI_DSIM + ESCMODE, ESCMODE_FORCE_STOP | ESCMODE_CMD_LP);
	udelay(1000);
	SET_REG(MIPI_DSIM + ESCMODE, GET_REG(MIPI_DSIM + ESCMODE) & ~(ESCMODE_FORCE_STOP));

	bufferPrintf("g\n"); // I've checked up to here -- Ricky26

	while((GET_REG(MIPI_DSIM + STATUS) & (STATUS_STOP | STATUS_DATA_STOP(DATA_LANES_ENABLED)))
		!= (STATUS_STOP | STATUS_DATA_STOP(DATA_LANES_ENABLED)));

	bufferPrintf("h\n");

	SET_REG(MIPI_DSIM + ESCMODE, ESCMODE_CMD_LP | ESCMODE_TX_UIPS_DAT | ESCMODE_TX_UIPS_CLK);
	while((GET_REG(MIPI_DSIM + STATUS) & (STATUS_ULPS | STATUS_DATA_ULPS(DATA_LANES_ENABLED)))
		!= (STATUS_ULPS | STATUS_DATA_ULPS(DATA_LANES_ENABLED)));

	bufferPrintf("i\n");

	SET_REG(MIPI_DSIM + CONFIG, GET_REG(MIPI_DSIM + CONFIG)
		& ~((CONFIG_EN_DATA_MASK << CONFIG_EN_DATA_SHIFT)
			| (CONFIG_NUM_DATA_MASK << CONFIG_NUM_DATA_SHIFT)));

	SET_REG(MIPI_DSIM + CONFIG, GET_REG(MIPI_DSIM + CONFIG) | CONFIG_EN_DATA(dataLanesEnabled)
		| CONFIG_NUM_DATA(numDataLanes));

	SET_REG(MIPI_DSIM + CLKCTRL, GET_REG(MIPI_DSIM + CLKCTRL)
		& ~(CLKCTRL_ESC_EN_DATA_MASK << CLKCTRL_ESC_EN_DATA_SHIFT));
	SET_REG(MIPI_DSIM + CLKCTRL, GET_REG(MIPI_DSIM + CLKCTRL) | CLKCTRL_ESC_EN_DATA(dataLanesEnabled));

	SET_REG(MIPI_DSIM + ESCMODE, GET_REG(MIPI_DSIM + ESCMODE) | ESCMODE_TX_UIPS_EXIT | ESCMODE_TX_UIPS_CLK_EXIT);

	bufferPrintf("j\n");

	while((GET_REG(MIPI_DSIM + STATUS) & (STATUS_ULPS | STATUS_DATA_ULPS(dataLanesEnabled))));
	SET_REG(MIPI_DSIM + ESCMODE, GET_REG(MIPI_DSIM + ESCMODE) & (~0x5));

	udelay(1000);
	
	bufferPrintf("k\n");

	SET_REG(MIPI_DSIM + ESCMODE, GET_REG(MIPI_DSIM + ESCMODE) & (~0xA));
	while((GET_REG(MIPI_DSIM + STATUS) & (STATUS_STOP | STATUS_DATA_STOP(dataLanesEnabled)))
		!= (STATUS_STOP | STATUS_DATA_STOP(dataLanesEnabled)));

	bufferPrintf("l\n");
	
	result = 0;
	mipi_dsim_has_init = 1;
	return result;
}


void mipi_dsim_quiesce() {
	bufferPrintf("mipi_dsim_quiesce()\r\n");
	if (!mipi_dsim_has_init)
		return;

	SET_REG(MIPI_DSIM + SWRST, SWRST_RESET);
	while((GET_REG(MIPI_DSIM + STATUS) & STATUS_SWRST) != STATUS_SWRST);

	SET_REG(MIPI_DSIM + CONFIG, GET_REG(MIPI_DSIM + CONFIG)
		& ~((CONFIG_EN_DATA_MASK << CONFIG_EN_DATA_SHIFT)
			| (CONFIG_NUM_DATA_MASK << CONFIG_NUM_DATA_SHIFT)));
	SET_REG(MIPI_DSIM + CONFIG, GET_REG(MIPI_DSIM + CONFIG) | CONFIG_EN_DATA(DATA_LANES_ENABLED)
		| CONFIG_NUM_DATA(NUM_DATA_LANES) | 1); // why was there no 1?
	SET_REG(MIPI_DSIM + CLKCTRL, GET_REG(MIPI_DSIM + CLKCTRL) & ~(CLKCTRL_TX_REQ_HSCLK
		| (CLKCTRL_ESC_EN_DATA_MASK << CLKCTRL_ESC_EN_DATA_SHIFT)));
	SET_REG(MIPI_DSIM + CLKCTRL, GET_REG(MIPI_DSIM + CLKCTRL) | CLKCTRL_ESC_EN_DATA(DATA_LANES_ENABLED));

	SET_REG(MIPI_DSIM + ESCMODE, ESCMODE_FORCE_STOP | ESCMODE_CMD_LP);
	udelay(1000);
	SET_REG(MIPI_DSIM + ESCMODE, GET_REG(MIPI_DSIM + ESCMODE) & ~(ESCMODE_FORCE_STOP));

	while((GET_REG(MIPI_DSIM + STATUS) & (STATUS_STOP | STATUS_DATA_STOP(DATA_LANES_ENABLED)))
		!= (STATUS_STOP | STATUS_DATA_STOP(DATA_LANES_ENABLED)));
	SET_REG(MIPI_DSIM + ESCMODE, ESCMODE_CMD_LP | ESCMODE_TX_UIPS_DAT | ESCMODE_TX_UIPS_CLK);
	while((GET_REG(MIPI_DSIM + STATUS) & (STATUS_ULPS | STATUS_DATA_ULPS(DATA_LANES_ENABLED)))
		!= (STATUS_ULPS | STATUS_DATA_ULPS(DATA_LANES_ENABLED)));

        SET_REG(MIPI_DSIM + CLKCTRL, CLKCTRL_NONE);
        SET_REG(MIPI_DSIM + CONFIG, CONFIG_VIDEO_MODE);
        SET_REG(MIPI_DSIM + ESCMODE, ESCMODE_NONE);
        SET_REG(MIPI_DSIM + PLLCTRL, PLLCTRL_NONE);

	clock_gate_switch(MIPI_DSIM_CLOCKGATE, OFF);
	mipi_dsim_has_init = 0;
}
