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
#include "framebuffer.h"
#include "openiboot-asmhelpers.h"

static GammaTableDescriptor PinotGammaTables[] = {
	// 3GS Gamma Tables
	{0xE50486, 0xFFFFFF,
		{{0x4C2822, 0x1C40011D, 0x1C71D00, 0x74DC701C, 0x4DDDDC73, 0xD03, 0xDDDC71C7, 0x403400D0, 0x37740007, 0xD00D034D, 0x1C034000, 0x70000, 0x7000000, 0x71C7070, 0x70000007, 0xDDDDDC70, 0xDD, 0}},
		{{0x401DD422, 0x74000107, 0x374D0000, 0xC003434D, 0x434D34DD, 0x70034003, 0x77771C70, 0xD0034D3, 0xD341DDD0, 0x43434D34, 0xD00D03, 0x1C000000, 0xD0034700, 0x34000, 0x1C0740, 0xDDDC1CC7, 0xD, 0}},
		{{0x74D422, 0x4000775D, 0xD000007, 0xC0D00D0D, 0x4DD3771D, 0x3434D3, 0xDC707000, 0xD0D0DDD, 0xD7401DD, 0x701C001C, 0x34DDC70, 0xC7070700, 0x1C71C1C1, 0x777771C7, 0x701C1DC, 0x4D34D000, 3, 0}},
	},

	{0xD10485, 0xFFFFFF,
	    {{0x710DCE62, 0x40007100, 0x71C71DD3, 0xD034DDDC, 0xD034D34C, 0x340, 0xDC770700, 0x1C71DD, 0x7000007, 0xC0700700, 0x771C1C01, 0xC71DD377, 0xC7071C71, 0xC71C0701, 0x434034DD, 0xC771DDD3, 0xDD, 0}},
		{{0x32BD0A62, 0x774D71D, 0x340D0007, 0x7034034, 0x34DC, 0xD0000, 0x1C070000, 0x771C71C, 0x1C007, 0x1C000, 0x1C00, 0x74DC771C, 0xC001C1DC, 0x70700701, 0x74DDDC70, 0x774010D3, 0xC77, 0}},
		{{0x740DCE62, 0x707400, 0x77774D34, 0x340D377, 0xDD34D34C, 0xD34DDDDD, 0x1C000000, 0xD00D0700, 0xD00D0, 0, 0xD3770700, 0x1DC4D374, 0xC771DC77, 0x3771C71D, 0xD00D034, 0x40D0D034, 3, 0}},
	},

	{0x80C20485, 0x80FFFFFF,
		{{0x4D1022, 0x7401D, 0xD000071D, 0x3434D34, 0xD37070D, 0x34D34D0D, 0x1F40, 0xDDC77707, 0x340001D, 0xD000, 0, 0xD034371C, 0x71DD3400, 0x777771DC, 0x774D377, 0x29501C07, 0xD5, 0}},
		{{0x4D1022, 0x4007401D, 0xD001C13, 0x3434D34D, 0xD31C000, 0xD0D34D0D, 0, 0x1DC71C1C, 0x340D0007, 0xD000D00, 0x34000340, 0xDDDDC707, 0x1C00071D, 0x701C01C0, 0x3434070, 0xDDC01D0, 0xC, 0}},
		{{0x1341022, 0x74D71D, 0x1C7134, 0x3400D0, 0x434DC1C0, 0xD0003, 0x1C700000, 0x374DC71C, 0xD00774DD, 0x3400, 0xD0000D, 0x71C700D0, 0x71C77, 0xC1C00000, 0x71C71, 0x1D50740D, 0xC, 0}},
	},

	{0xC20485, 0xFFFFFF,
		{{0x40134422, 0x3401D35C, 0x34340071, 0xD34DDDD, 0x3771C0D, 0, 0x71C1C71C, 0xD374DDDC, 0x7134, 0x701C0070, 0xC1C01C00, 0x34DDC1, 0x3400D000, 0x34D374DD, 0xD0D0D0D, 0x7071C74D, 7, 0}},
		{{0x401D0422, 0x34077407, 0xD000001D, 0xD34D34, 0xD0C7000, 0x700001C, 0x1C1C7070, 0x4DD371DC, 0xC0001D37, 0x700001, 0x70007000, 0x4DDDC1C0, 0x1C774DD3, 0x1C771C1C, 0x1C771C7, 0xC4000D00, 0xCA, 0}},
		{{0x740422, 0x7741C4, 0x1C71340, 0x1C0001C0, 0x1C34DDDC, 0x71C71C70, 0x4C771C1C, 0xD377777, 0x3400340, 0x7000071, 0xC001C000, 0x700701, 0x74DDDDC7, 0x1C07137, 0xDDDC71C0, 0x341C71D, 0, 0}},
	},

	{0x8A0485, 0xFFFFFF,
		{{0x1041D422, 0x741DD740, 0x32BA2B07, 0x771CD000, 0xC1C03703, 0xD00D31, 0xDC7771C0, 0xDC71DC71, 0x4D0D374D, 0xD343, 0xD00000D, 0x4343434D, 0xDDDD3743, 0x71DC7774, 0x4D34DDDC, 0x1DD34D37, 0xD34, 0}},
		{{0x4401422, 0xB1001010, 0x77432BA2, 0x1CD0DDDC, 0xC70377D3, 0x34DDDD, 0xC0700700, 0xDDC71C71, 0x4D0D0D34, 0x34000D03, 0x43434000, 0x4D0D0D03, 0x74D0D343, 0xDDDDD377, 0xD34DD374, 0xD374D0, 0xD37, 0}},
		{{0x1074422, 0x74D71004, 0xD34D0000, 0x377000D0, 0x71C71DC0, 0x340DDC, 0xDDDDDC00, 0x1C71C71D, 0x4D37771C, 0x34D0D3, 0x3400, 0x4D3434D0, 0xDDDDDDD3, 0x7071DDDD, 0xDDDC71C0, 0x402B4771, 0x37, 0}},
	},
	// End 3GS Gamma Tables
};

static LCDInfo LCDInfoTable[] = {
	{"n88", 0xB, 0xA4CB80, 0xA3, 0x140, 0xC, 0xC, 0x10, 0x1E0, 6, 6, 8, 0, 0, 0, 0, 0x18, 3, 0x31571541},
};

volatile uint32_t* CurFramebuffer;

static LCDInfo* LCDTable;
static uint32_t TimePerMillionFrames = 0;

static int gammaVar1;
static int gammaVar2;
static uint8_t gammaVar3;

Window* currentWindow;

OnOff SyncFramebufferToDisplayActivated = OFF;
uint32_t framebufferLastFill = 0;

int pinot_init(LCDInfo* LCDTable, ColorSpace colorspace, uint32_t* panelID, Window* currentWindow);

void installGammaTables(int panelID, int maxi, uint32_t buffer1, uint32_t buffer2, uint32_t buffer3);
void setWindowBuffer(int window, uint32_t* buffer);
static Window* createWindow(int zero0, int zero2, int width, int height, ColorSpace colorSpace);
static void createFramebuffer(Framebuffer* framebuffer, uint32_t framebufferAddr, int width, int height, int lineWidth, ColorSpace colorSpace);

void displaytime_sleep(uint32_t time)
{
	bufferPrintf("task_sleep %d\n", time * TimePerMillionFrames / 1000);
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
	// Empty on 3GS
}

int signed_calculate_remainder(uint64_t x, uint64_t y) {
	return (int)(x - y*(x/y));
}

uint32_t calculate_remainder(uint64_t x, uint64_t y) {
	return (uint32_t)(x - y*(x/y));
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

void lcd_fill_switch(OnOff on_off, uint32_t color) {
	if (on_off == ON) {
		if (color)
			framebufferLastFill = color;

		framebuffer_fill(&currentWindow->framebuffer, 0, 0, currentWindow->framebuffer.width, currentWindow->framebuffer.height, color);

		SET_REG(CLCD_BASE + 0x0, GET_REG(CLCD_BASE + 0x0) | 1);
		SET_REG(CLCD_BASE + 0x1B10, GET_REG(CLCD_BASE + 0x1B10) | 1);
	} else {
		if (SyncFramebufferToDisplayActivated == OFF) // It used to return, when on_off == SyncFramebufferToDisplayActivated == ON, too -- Bluerise
			return;

		SET_REG(CLCD_BASE + 0x0, GET_REG(CLCD_BASE + 0x0) & (~1));
		while (!(GET_REG(CLCD_BASE + 0x0) & 2)) ;
		SET_REG(CLCD_BASE + 0x1B10, GET_REG(CLCD_BASE + 0x1B10) & (~1));
		while (!(GET_REG(CLCD_BASE + 0x1B10) & 2)) ;
	}
	SyncFramebufferToDisplayActivated = on_off;
}

void lcd_fill(uint32_t color) {
	lcd_fill_switch(ON, color);
}

int displaypipe_init() {
	int result = 0;
	uint32_t panelID;

	if (!LCDTable)
		LCDTable = &LCDInfoTable[DISPLAYID];

	clock_gate_switch(CLCD_CLOCKGATE_1, ON);
	clock_gate_switch(CLCD_CLOCKGATE_2, ON);
	clock_gate_switch(CLCD_CLOCKGATE_3, ON);


	bufferPrintf("clcd: base=0x%08x\r\n", GET_REG(CLCD_BASE));

	//SET_REG(CLCD_BASE, 0x100);
	//while ((GET_REG(CLCD_BASE) & 0x100)); // TODO: Sort out threaded loading of boot-time modules so we can yield -- Ricky26
	udelay(1);

	SET_REG(CLCD_BASE + 0x0, 0x20084);
	SET_REG(CLCD_BASE + 0x300, 0x80000001);
	if (LCDTable->bitsPerPixel <= 0x12)
		SET_REG(CLCD_BASE + 0x300, GET_REG(CLCD_BASE + 0x300) | 0x1110000);

	SET_REG(CLCD_BASE + 0x1B10, 0);
	SET_REG(CLCD_BASE + 0x1B14, (LCDTable->IVClk << VIDCON1_IVCLKSHIFT) | (LCDTable->IHSync << VIDCON1_IHSYNCSHIFT) | (LCDTable->IVSync << VIDCON1_IVSYNCSHIFT) | (LCDTable->IVDen << VIDCON1_IVDENSHIFT));
	SET_REG(CLCD_BASE + 0x1B1C, ((LCDTable->verticalBackPorch - 1) << VIDTCON_BACKPORCHSHIFT) | ((LCDTable->verticalFrontPorch - 1) << VIDTCON_FRONTPORCHSHIFT) | ((LCDTable->verticalSyncPulseWidth - 1) << VIDTCON_SYNCPULSEWIDTHSHIFT));
	SET_REG(CLCD_BASE + 0x1B20, ((LCDTable->horizontalBackPorch - 1) << VIDTCON_BACKPORCHSHIFT) | ((LCDTable->horizontalFrontPorch - 1) << VIDTCON_FRONTPORCHSHIFT) | ((LCDTable->horizontalSyncPulseWidth - 1) << VIDTCON_SYNCPULSEWIDTHSHIFT));
	SET_REG(CLCD_BASE + 0x1B24, ((LCDTable->width - 1) << VIDTCON2_HOZVALSHIFT) | ((LCDTable->height - 1) << VIDTCON2_LINEVALSHIFT));
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

	ColorSpace colorSpace;
//XXX:	It normally grabs it from nvram var "display-color-space" as string. -- Bluerise
	colorSpace = RGB888;

	currentWindow = createWindow(0, 0, LCDTable->width, LCDTable->height, colorSpace);
	if (!currentWindow)
		return -1;

//XXX:	It sets the framebuffer address into nvram var "framebuffer". -- Bluerise
//	nvram_setvar("framebuffer", currentWindow->framebuffer.buffer, 0);
	result = pinot_init(LCDTable, colorSpace, &panelID, currentWindow);
	if (result) {
		lcd_fill_switch(OFF, 0);
		LCDTable = 0;
		return result;
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

	installGammaTables(panelID, 257, (uint32_t)buffer1, (uint32_t)buffer2, (uint32_t)buffer3);
	setWindowBuffer(4, buffer1);
	setWindowBuffer(5, buffer2);
	setWindowBuffer(6, buffer3);

	free(buffer1);
	free(buffer2);
	free(buffer3);

	SET_REG(CLCD_BASE + 0x400, 1);
	SET_REG(CLCD_BASE + 0x1B38, 0x80);

	CurFramebuffer = currentWindow->framebuffer.buffer;
	framebuffer_setup();
	framebuffer_setdisplaytext(TRUE);

	return result;
}

void display_module_init()
{
	displaypipe_init();
}
MODULE_INIT(display_module_init);

static uint8_t PanelIDInfo[6];
static uint32_t DotPitch;
static uint8_t dword_5FF3AE0C;

int pinot_init(LCDInfo* LCDTable, ColorSpace colorspace, uint32_t* panelID, Window* currentWindow) {
	uint32_t pinot_default_color = 0;
	uint32_t pinot_backlight_cal = 0;
	uint32_t pinot_panel_id = 0;

	bufferPrintf("pinot_init()\r\n");
	DotPitch = LCDTable->DotPitch;
	gpio_pin_output(0x500, 0);
	task_sleep(10);
	mipi_dsim_init(LCDTable);
	gpio_pin_output(0x500, 1);
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

	bufferPrintf("s\r\n");

	gpio_switch(0x1507, 0);
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
	gpio_pin_output(0x500, 0);
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

	SET_REG(CLCD_BASE + 0x408, window_bit | 0x10000);

	for (cur_size = 0; cur_size < size; cur_size++) {
		to_set = *buffer;
		if ((window - 4) <= 2)
			to_set = (*(buffer+1) & 0x3FF) | ((*buffer << 10) & 0xFFC00);
		SET_REG(CLCD_BASE + 0x40C, to_set | (1 << 31));
		buffer++;
	}

	SET_REG(CLCD_BASE + 0x408, 0);
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
			reg_bit = 7;
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

	createFramebuffer(&newWindow->framebuffer, (uint32_t)malloc(newWindow->lineBytes*height), width, height, width, colorSpace);
	bufferPrintf("clcd: buffer 0x%08x\r\n", newWindow->framebuffer.buffer);

	SET_REG(CLCD_BASE + 0x20, (reg_bit << 8) | 0x200000);
	SET_REG(CLCD_BASE + 0x24, (uint32_t)newWindow->framebuffer.buffer);
	SET_REG(CLCD_BASE + 0x28, width);
	SET_REG(CLCD_BASE + 0x2C, 0);
	SET_REG(CLCD_BASE + 0x30, (width << 16) | height);
	SET_REG(CLCD_BASE + 0x38, 0);
	SET_REG(CLCD_BASE + 0x3C, 0);
	SET_REG(CLCD_BASE + 0x34, (zero0 << 16) | zero2);

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
