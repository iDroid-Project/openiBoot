#include "openiboot.h"
#include "util.h"
#include "hardware/clcd.h"
#include "hardware/mipi_dsim.h"
#include "timer.h"
#include "i2c.h"
#include "tasks.h"
#include "gpio.h"
#include "lcd.h"
#include "mipi_dsim.h"
#include "openiboot-asmhelpers.h"

static GammaTableDescriptor PinotGammaTables[] = {
	{0x970548, 0xFFFF7F,
		{{0x74CA62, 0x1DD0, 0x4D374D0D, 0x40D000D3, 0x3434003, 0xD00D, 0, 0x7000070, 0xD0001C0, 0xD3743400, 0xD0D34D34, 0xC01C0034, 0x34D0D701, 0x1C00000D, 0x4D0D0000, 0xC0700003, 0xC1, 0}},
		{{0x401DCA62, 0xDD000077, 0xDC771DDD, 0x40D0D0DD, 0x34343403, 0xD000D0, 0x70000000, 0x7007000, 0x3400070, 0xD0D34D00, 0x34343434, 0x71C1C000, 0x40034070, 0x1C71C1C3, 0x340D01C7, 0xC01C0000, 0xC1, 0}},
		{{0x1DCA62, 0x34000074, 0x4DDDDDDD, 0x37, 0xD000000, 0x70000000, 0x70007000, 0x701C0700, 0x1C0, 0x4DD00D00, 0xD0D34DD3, 0x70700034, 0xD34D1C1C, 0x1C3434, 0xD34D001C, 0x3434, 0, 0}},
	},
	{0xC20548, 0xFFFF7F,
		{{0xD071CA62, 0xDD34071D, 0xC71C771D, 0x340D34D, 0xD0, 0x71DC1C00, 0x7771DDC7, 0x7771D377, 0x71DDDC, 0xD0D07000, 0xD374D34D, 0xD0D34DDD, 0xD374D0D0, 0x1C0034D, 0x34001C07, 0x4DD34000, 0x37, 0}},
		{{0x4001CA62, 0xC74D001C, 0x71C07071, 0xC00D0377, 0xD0003401, 0xDC71C1C0, 0xDDC77771, 0xDDC774DD, 0x77771, 0x34000007, 0x4D34D34D, 0xD0D37, 0x77743400, 0x377, 0x40000007, 0x43434003, 3, 0}},
		{{0xCAC0A62, 0xC1D3401D, 0x1C00001, 0xD371C, 0x70, 0x71DC7070, 0xDDDC7777, 0x71DDDDDD, 0x71DDC7, 0xD3407000, 0x374DD34D, 0x3434DD, 0x374D0000, 0x700034D, 0xD00701C, 0, 0, 0}},
	},
	{0xB30689, 0xFFFFBF,
		{{0x4CAC0A62, 0x71DDD000, 0x70034377, 0x701C0700, 0x1DC71C70, 0xDDDC7777, 0x77777771, 0xC77771DC, 0xC71DC71D, 0x1C070071, 0x1C0, 0x70, 0, 0x1C01C1D0, 0xC71C1C1C, 0xD001C771, 0x374, 0}},
		{{0x4CAC0A62, 0xC7137400, 0x70034DD, 0x701C1C70, 0xC771C71C, 0xDDDDDDDD, 0x77777771, 0x77771DC7, 0xC71DC71C, 0x1C070071, 0x1C0, 0, 0, 0x700774, 0x70701C07, 0xD0771C7, 0xDD, 0}},
		{{0x1341CA62, 0x77777400, 0x3437, 0xC001C000, 0x71C1C701, 0x1C71DDC7, 0xC71DC777, 0x71C71C71, 0x1C71C70, 0x7001C0, 0x40000000, 0x40000003, 0xC4000003, 0xC7070701, 0x4D3771C1, 0xDC7774D3, 0xDD, 0}}
	},
	{0x970689, 0xFFFFBF,
		{{0x4DCE22, 0x1C01C740, 0xC034D000, 0x1C0771C1, 0x1C0700, 0xD00000, 0x34D0340D, 0xD34D0D34, 0x71C03434, 0x1C71DDC7, 0x1C1C71C7, 0x700700, 0x10007007, 0x1C1C0000, 0xD37771C, 0x707740D, 0xC7, 0}},
		{{0xB04DCE22, 0x1C1C7432, 0xD1C00, 0x1DC771C7, 0x1C70701C, 0, 0xD0034034, 0xD0D0D0D0, 0x1CD34374, 0x771C771C, 0x71C71C1C, 0x1C001C00, 0x1C000, 0x734001D, 0xDDDC1C1C, 0x1C74D34, 0xC1C, 0}},
		{{0x1D0DCE22, 0x1C774D0, 0xDC7400, 0x70000, 0x40000000, 0x4340D003, 0x377434D3, 0xDDD374DD, 0x71C0D34, 0x1C1C1C77, 0x1C01C71C, 0x70007000, 0x747000, 0x77770070, 0x34377, 0, 0, 0}},
	},
	{0xE50689, 0xFFFFBF,
		{{0xD01DCA62, 0xD340001D, 0x3434, 0xD0D00000, 0x74DD3400, 0x34D34D37, 0x340D034, 0xD00, 0xD, 0x1C71C000, 0x1C7071C7, 0x4D34DC77, 0x701D34D3, 0xD0D1C000, 0xDDD34D34, 0x701C7774, 0xC0, 0}},
		{{0x741DCA62, 0x4D340007, 0x34D3, 0x34001C0, 0x4DD0D000, 0xD0D0D0D3, 0xD00, 0x7A400000, 0xD00000, 0xDC701C00, 0x71C71C71, 0xD37771C7, 0x7434D34, 0x4000001C, 0x713774D3, 0x1C71C, 0, 0}},
		{{0x1DCA62, 0xD3400071, 0xD0D03434, 0x4DD340D0, 0x71DC74D3, 0xD3777777, 0x40D3434D, 0xD00D03, 0x34034, 0x71C1C1C0, 0xC71C1C70, 0x4D34D371, 0x1C01C4D3, 0x40D001C0, 0x40340D03, 0x3403403, 0, 0}},
	},
};

static LCDInfo LCDInfoTable[] = {
	{"n90", 0xA, 0x30EC6A0, 0x146, 640, 0x47, 0x47, 0x49, 960, 0xC, 0xC, 0x10, 0, 0, 0, 0, 24, 3, 0x8391643},
	{"k48", 0xA, 0x413B380, 0x84, 0x400, 0x85, 0x85, 0x87, 0x300, 0xA, 0xA, 0xC, 0, 0, 0, 0, 0x12, 3, 0x644},
};

volatile uint32_t* CurFramebuffer;

static LCDInfo* LCDTable;
static uint32_t TimePerMillionFrames = 0;

static uint32_t dword_5FF3D04C = 0x3D278480;
static uint32_t dword_5FF3D0D0;

static int gammaVar1;
static int gammaVar2;
static uint8_t gammaVar3;

Window* currentWindow;

static int LCDTableunkn1is0xA = 0;

OnOff SyncFramebufferToDisplayActivated = OFF;
uint32_t framebufferLastFill = 0;

int pinot_init(LCDInfo* LCDTable, ColorSpace colorspace, uint32_t* panelID, Window* currentWindow);

void installGammaTables(int panelID, int maxi, uint32_t buffer1, uint32_t buffer2, uint32_t buffer3);
void setWindowBuffer(int window, uint32_t* buffer);
static Window* createWindow(int zero0, int zero2, int width, int height, ColorSpace colorSpace);
static void createFramebuffer(Framebuffer* framebuffer, uint32_t framebufferAddr, int width, int height, int lineWidth, ColorSpace colorSpace);
static void framebuffer_fill(Framebuffer* framebuffer, int x, int y, int width, int height, int fill);
static void hline_rgb888(Framebuffer* framebuffer, int start, int line_no, int length, int fill);
static void hline_rgb565(Framebuffer* framebuffer, int start, int line_no, int length, int fill);
static void vline_rgb888(Framebuffer* framebuffer, int start, int line_no, int length, int fill);
static void vline_rgb565(Framebuffer* framebuffer, int start, int line_no, int length, int fill);

void displaytime_sleep(uint32_t time) {
	task_sleep(time * TimePerMillionFrames / 1000);
}

int pmu_send_buffer(int bus, uint8_t buffer, uint8_t response, int check) {
	uint8_t send_buffer[2] = { buffer, response };
	uint8_t recv_buffer = 0;
	int result;

	i2c_tx(bus, 0xE9, (void*)&send_buffer, 2);
	if (check && (i2c_rx(bus, 0xE8, (void*)&buffer, 1, (void*)&recv_buffer, 1), recv_buffer != response))
		result = -1;
	else
		result = 0;
	return result;
}

void sub_5FF08870(uint8_t arg) {
	uint8_t v1;

	v1 = (1049-50*arg)/50;
	pmu_send_buffer(0, 0x6B, v1, 1);
	task_sleep(10);
	pmu_send_buffer(0, 0x6C, v1 + 4, 1);
}

int signed_calculate_remainder(uint64_t x, uint64_t y) {
	return (int)(x - y*(x/y));
}

uint32_t calculate_remainder(uint64_t x, uint64_t y) {
	return (uint32_t)(x - y*(x/y));
}

void lcd_fill_switch(OnOff on_off, uint32_t color) {
	if (on_off == ON) {
		if (color)
			framebufferLastFill = color;

		framebuffer_fill(&currentWindow->framebuffer, 0, 0, currentWindow->framebuffer.width, currentWindow->framebuffer.height, color);
		if (LCDTableunkn1is0xA)
			SET_REG(0x89200050, GET_REG(0x89200050) | 1);
	} else {
		if (SyncFramebufferToDisplayActivated == OFF) // It used to return, when on_off == SyncFramebufferToDisplayActivated == ON, too -- Bluerise
			return;
		if (LCDTableunkn1is0xA) {
			SET_REG(0x89200050, GET_REG(0x89200050) & (~1));
			while (!(GET_REG(0x89200050) & 2)) ;
		}
	}
	SyncFramebufferToDisplayActivated = on_off;
}

void lcd_fill(uint32_t color) {
	lcd_fill_switch(ON, color);
}

uint32_t configureLCDClock(uint32_t result, int zero0, int zero1, int zero2, int zero3, unsigned int divider) {
	uint32_t v6;
	uint32_t v7;
	uint32_t v8;
	uint32_t v10;

	// I actually hate this part. -- Bluerise
	// dword_5FF3D04C == CalculatedFrequencyTable[4] == 0x3D278480;
	// or 0x5B8D8000 because of different frequencies but I don't think so...
	// <CPICH> it's just result = r0 - r1*(r0/r1)
	// <CPICH> uint64_t r0, r1;

	if (result == 10) {
		v6 = 31;
		v7 = dword_5FF3D04C;
		v8 = dword_5FF3D04C / divider;
		do
		{
			if (!calculate_remainder(v8, v6))
				break;
			--v6;
		}
		while (v6 != 1);
		SET_REG(0xBF100054, (GET_REG(0xBF100054) & 0xFFFFFFE0) | v6);
		v10 = v8 / v6;
		SET_REG(0xBF100094, (GET_REG(0xBF100094) & 0xFFFFFFE0) | v10);
//		unk_5FF3D078 = v7 / v6;
		dword_5FF3D0D0 = v7 / v6 / v10;
		result = dword_5FF3D0D0;
	}
	return result;
}

int displaypipe_init() {
	int result = 0;
	uint32_t clcd_reg;
	uint32_t panelID;
	memset((void*)CLCD_FRAMEBUFFER, 0, 0x800000);

	if (!LCDTable)
		LCDTable = &LCDInfoTable[DISPLAYID];

	if (LCDTable->unkn1 == 0xA) {
		clock_gate_switch(0x12, ON);
		clock_gate_switch(0xF, ON);
		LCDTableunkn1is0xA = 1;
	}
	SET_REG(CLCD + 0x104C, GET_REG(CLCD + 0x104C) | 0x10);
	SET_REG(CLCD + 0x104C, (GET_REG(CLCD + 0x104C) & 0xFFFFF8FF) | 0x100);
	SET_REG(CLCD + 0x104C, (GET_REG(CLCD + 0x104C) & 0xF800FFFF) | 0x4000000);
	SET_REG(CLCD + 0x1030, (LCDTable->width << 16) | LCDTable->height);
	if (CLCD == 0x89000000) {
		clcd_reg = 0x180;
	} else if (CLCD == 0x89100000) {
		clcd_reg = 0x1C0;
	} else {
		system_panic("displaypipe_init: unsupported displaypipe_base_addr: 0x%08lx\r\n", CLCD);
	}
	SET_REG(CLCD + 0x205C, (clcd_reg << 16) | 0x1F0);
	SET_REG(CLCD + 0x2060, 0x90);
	SET_REG(CLCD + 0x105C, 0x13880801);
	SET_REG(CLCD + 0x2064, 0xBFF00000);
	if (LCDTableunkn1is0xA) {
		SET_REG(0x89200000, 256);
		while (GET_REG(0x89200000) & 0x100);
		udelay(1);
		SET_REG(0x89200000, 4);
		SET_REG(0x89200004, 3);
		SET_REG(0x89200014, 0x80000001);
		SET_REG(0x89200018, 0x20408);
		if (LCDTable->bitsPerPixel <= 0x12)
			SET_REG(0x89200014, GET_REG(0x89200014) | 0x1110000);
		SET_REG(0x89200050, 0);
		SET_REG(0x89200054, (LCDTable->IVClk << VIDCON1_IVCLKSHIFT) | (LCDTable->IHSync << VIDCON1_IHSYNCSHIFT) | (LCDTable->IVSync << VIDCON1_IVSYNCSHIFT) | (LCDTable->IVDen << VIDCON1_IVDENSHIFT));
		SET_REG(0x89200058, ((LCDTable->verticalBackPorch - 1) << VIDTCON_BACKPORCHSHIFT) | ((LCDTable->verticalFrontPorch - 1) << VIDTCON_FRONTPORCHSHIFT) | ((LCDTable->verticalSyncPulseWidth - 1) << VIDTCON_SYNCPULSEWIDTHSHIFT));
		SET_REG(0x8920005C, ((LCDTable->horizontalBackPorch - 1) << VIDTCON_BACKPORCHSHIFT) | ((LCDTable->horizontalFrontPorch - 1) << VIDTCON_FRONTPORCHSHIFT) | ((LCDTable->horizontalSyncPulseWidth - 1) << VIDTCON_SYNCPULSEWIDTHSHIFT));
		SET_REG(0x89200060, ((LCDTable->width - 1) << VIDTCON2_HOZVALSHIFT) | ((LCDTable->height - 1) << VIDTCON2_LINEVALSHIFT));
		TimePerMillionFrames = 1000000
			* (LCDTable->verticalBackPorch
				+ LCDTable->verticalFrontPorch
				+ LCDTable->verticalSyncPulseWidth
				+ LCDTable->height)
			* (LCDTable->horizontalBackPorch
				+ LCDTable->horizontalFrontPorch
				+ LCDTable->horizontalSyncPulseWidth
				+ LCDTable->width)
			/ LCDTable->DrivingClockFrequency;
	}

	ColorSpace colorSpace;
//XXX:	It normally grabs it from nvram var "display-color-space" as string. -- Bluerise
	colorSpace = RGB888;

	currentWindow = createWindow(0, 0, LCDTable->width, LCDTable->height, colorSpace);
	if (!currentWindow)
		return -1;

//XXX:	It sets the framebuffer address into nvram var "framebuffer". -- Bluerise
//	nvram_setvar("framebuffer", currentWindow->framebuffer.buffer, 0);
	configureLCDClock(LCDTable->unkn1, 0, 0, 0, 0, LCDTable->DrivingClockFrequency);
	if (LCDTable->unkn17 == 3) {
		result = pinot_init(LCDTable, colorSpace, &panelID, currentWindow);
		if (result) {
			lcd_fill_switch(OFF, 0);
			LCDTable = 0;
			return result;
		}
	} else {
		panelID = 0;
	}

	if (SyncFramebufferToDisplayActivated == OFF)
		lcd_fill_switch(ON, framebufferLastFill);

	uint32_t* buffer1 = malloc(1028);
	uint32_t* buffer2 = malloc(1028);
	uint32_t* buffer3 = malloc(1028);
	uint32_t curBuf;
	buffer1[0] = 1;
	for (curBuf = 0; curBuf != 256; curBuf++) {
		if (signed_calculate_remainder(curBuf+1, (256 >> (10 - (uint8_t)(LCDTable->bitsPerPixel / 3)))) == 1)
			buffer1[curBuf] = buffer1[curBuf] - 1;
		buffer1[curBuf+1] = buffer1[curBuf] + 4;
		buffer2[curBuf] = buffer1[curBuf];
		buffer3[curBuf] = buffer1[curBuf];
		curBuf++;
	}
	buffer3[256] = buffer1[256];
	buffer2[256] = buffer1[256];
	if (LCDTableunkn1is0xA) {
		installGammaTables(panelID, 257, (uint32_t)buffer1, (uint32_t)buffer2, (uint32_t)buffer3);
		setWindowBuffer(4, buffer1);
		setWindowBuffer(5, buffer2);
		setWindowBuffer(6, buffer3);
//XXX	Enabling the Gamma Tables currently fucks some things up -- Bluerise
//		SET_REG(0x8920002C, 1);
	}

	int i;
	bufferPrintf("Buffer1:\n");
	for (i = 0; i <= 256; i++) {
		bufferPrintf("0x%08x\n", buffer1[i]);
	}
	bufferPrintf("\nBuffer2:\n");
	for (i = 0; i <= 256; i++) {
		bufferPrintf("0x%08x\n", buffer2[i]);
	}
	bufferPrintf("\nBuffer3:\n");
	for (i = 0; i <= 256; i++) {
		bufferPrintf("0x%08x\n", buffer3[i]);
	}

	free(buffer1);
	free(buffer2);
	free(buffer3);

	CurFramebuffer = currentWindow->framebuffer.buffer;

	return result;
}

static uint8_t PanelIDInfo[5];
static uint32_t DotPitch;
static uint8_t dword_5FF3AE0C;

int pinot_init(LCDInfo* LCDTable, ColorSpace colorspace, uint32_t* panelID, Window* currentWindow) {
	uint32_t pinot_default_color = 0;
	uint32_t pinot_backlight_cal = 0;
	uint32_t pinot_panel_id = 0;

	bufferPrintf("pinot_init()\r\n");
	DotPitch = LCDTable->DotPitch;
	gpio_pin_output(0x206, 0);
	task_sleep(10);
	mipi_dsim_init(LCDTable);
	gpio_pin_output(0x206, 1);
	task_sleep(6);
	mipi_dsim_write_data(5, 0, 0);
	udelay(10);
	mipi_dsim_framebuffer_on_off(ON);
	task_sleep(25);
//XXX	If there's no PanelID it tries to grab it from nvram entry "pinot-panel-id". We don't have nvram yet. -- Bluerise
	if (!pinot_panel_id) {
		uint32_t read_length;
		read_length = 15;
		PanelIDInfo[0] = 0xB1; // -79
		if (mipi_dsim_read_write(0x14, &PanelIDInfo[0], &read_length) || read_length <= 2) {
			bufferPrintf("pinot_init(): read of pinot panel id failed\r\n");
		} else {
			pinot_panel_id = (PanelIDInfo[0] << 24) | (PanelIDInfo[1] << 16) | ((PanelIDInfo[3] & 0xF0) << 8) | ((PanelIDInfo[2] & 0xF8) << 4) | ((PanelIDInfo[3] & 0xF) << 3) | (PanelIDInfo[2] & 0x7);
			pinot_default_color = (PanelIDInfo[3] & 0x8) ? 0x0 : 0xFFFFFF;
			pinot_backlight_cal = PanelIDInfo[5];
			dword_5FF3AE0C = PanelIDInfo[4];
		}
	}
	bufferPrintf("pinot_init(): pinot_panel_id:      0x%08lx\r\n", pinot_panel_id);
	bufferPrintf("pinot_init(): pinot_default_color: 0x%08lx\r\n", pinot_default_color);
	bufferPrintf("pinot_init(): pinot_backlight_cal: 0x%08lx\r\n", pinot_backlight_cal);
	lcd_fill_switch(ON, pinot_default_color);
	udelay(100);
	if (!pinot_panel_id) {
		lcd_fill_switch(OFF, 0);
		mipi_dsim_quiesce();
		return -1;
	}

	mipi_dsim_write_data(5, 0x11, 0);
	displaytime_sleep(7);
	mipi_dsim_write_data(5, 0x29, 0);
	displaytime_sleep(7);
	gpio_switch(0x207, 0);
	*panelID = pinot_panel_id;

	if (!dword_5FF3AE0C)
		return dword_5FF3AE0C;

	sub_5FF08870(dword_5FF3AE0C);
	return 0;
}

void pinot_quiesce() {
	bufferPrintf("pinot_quiesce()\r\n");
	mipi_dsim_write_data(5, 0x28, 0);
	mipi_dsim_write_data(5, 0x10, 0);
	displaytime_sleep(6);
	mipi_dsim_framebuffer_on_off(OFF);
	mipi_dsim_quiesce();
	gpio_pin_output(0x206, 0);
	return;
}

static uint8_t installGammaTableHelper(uint8_t* table) {
	if(gammaVar2 == 0) {
		gammaVar3 = table[gammaVar1++];
	}

	int toRet = (gammaVar3 >> gammaVar2) & 0x3;
	gammaVar2 += 2;

	if(gammaVar2 == 8)
		gammaVar2 = 0;

	return toRet;
}

static void installGammaTable(int maxi, uint32_t curReg, uint8_t* table) {
        gammaVar1 = 0;
        gammaVar2 = 0;
        gammaVar3 = 0;

        uint8_t r4 = 0;
        uint16_t r6 = 0;
        uint8_t r8 = 0;

        SET_REG(curReg, 0x0);

        curReg += 4;

	int i;
	for (i = 1; i < maxi; i++) {
                switch(installGammaTableHelper(table)) {
                        case 0:
                                r4 = 0;
                                break;
                        case 1:
                                r4 = 1;
                                break;
                        case 2:
                                {
                                        uint8_t a = installGammaTableHelper(table);
                                        uint8_t b = installGammaTableHelper(table);
                                        r4 = a | (b << 2);
                                        if((r4 & (1 << 3)) != 0) {
                                                r4 |= 0xF0;
                                        }
                                }
                                break;
                        case 3:
                                r4 = 0xFF;
                                break;
                }

                if(i == 1) {
                        r4 = r4 + 8;
                }

                r8 += r4;
                r6 += r8;

                SET_REG(curReg, (uint32_t)r6);

                curReg += 4;
        }
}

void installGammaTables(int panelID, int maxi, uint32_t buffer1, uint32_t buffer2, uint32_t buffer3) {
	GammaTableDescriptor* curTable = &PinotGammaTables[0];

	int i;
	for(i = 0; i < (sizeof(PinotGammaTables)/sizeof(GammaTableDescriptor)); i++) {
		if((curTable->panelIDMask & panelID) == curTable->panelIDMatch) {
			bufferPrintf("Found Gamma table 0x%08lx / 0x%08lx\r\n", curTable->panelIDMatch, curTable->panelIDMask);
			installGammaTable(maxi, buffer1, (uint8_t*) curTable->table0.data);
			installGammaTable(maxi, buffer2, (uint8_t*) curTable->table1.data);
			installGammaTable(maxi, buffer3, (uint8_t*) curTable->table2.data);
			return;
		}
		curTable++;
	}
	bufferPrintf("No Gamma table found for display_id: 0x%08lx\r\n", panelID);
}

void setWindowBuffer(int window, uint32_t* buffer) {
	uint32_t size;
	uint32_t cur_size;
	uint32_t to_set;
	uint32_t window_bit = window << 12;
	if (window == 7)
		size = 225;
	else
		size = 256;

	SET_REG(0x89200034, window_bit | 0x10000);

	for (cur_size = 0; cur_size <= size; cur_size++) {
		to_set = *buffer;
		if ((window - 4) <= 2)
			to_set = (*(buffer+1) & 0x3FF) | ((*buffer << 10) & 0xFFC00);
		SET_REG(0x89200038, to_set | (1 << 31));
		buffer++;
	}

	SET_REG(0x89200034, 0);
}

void framebuffer_fill(Framebuffer* framebuffer, int x, int y, int width, int height, int fill) {
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

static Window* createWindow(int zero0, int zero2, int width, int height, ColorSpace colorSpace) {
	uint32_t bitsPerPixel;
	uint32_t reg_bit;

	switch (colorSpace) {
		default:
		case RGB888:
			bitsPerPixel = 32;
			reg_bit = 0;
			break;
		case RGB565:
			bitsPerPixel = 16;
			reg_bit = 4;
			break;
	}

	Window* newWindow;
	newWindow = (Window*) malloc(sizeof(Window));
	newWindow->created = FALSE;
	newWindow->width = width;
	newWindow->height = height;
	newWindow->lineBytes = width * (bitsPerPixel / 8);

	createFramebuffer(&newWindow->framebuffer, 0x5F700000, width, height, width, colorSpace);

	SET_REG(CLCD + 0x4040, (reg_bit << 8) | 1);
	SET_REG(CLCD + 0x4044, (uint32_t)newWindow->framebuffer.buffer);
	SET_REG(CLCD + 0x4048, (newWindow->lineBytes & 0xFFFFFFC0) | 2);
	SET_REG(CLCD + 0x4050, 0);
	SET_REG(CLCD + 0x4060, (width << 16) | height);
	SET_REG(CLCD + 0x2040, 0xFFFF0202);
	SET_REG(CLCD + 0x404C, 1);
	SET_REG(CLCD + 0x4074, 0x200060);
	SET_REG(CLCD + 0x4078, 32);

	SET_REG(CLCD + 0x1038, GET_REG(CLCD + 0x1038) | 0x100);
	newWindow->created = TRUE;

	return newWindow;
}

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
