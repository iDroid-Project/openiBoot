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
#include "arm/arm.h"
#include "pmu.h"
#include "commands.h"

static GammaTableDescriptor PinotGammaTables[] = {
	{0x970387, 0xFFFFFF,
		{{0x4041D422, 0x4001DD07, 0xDDDC003, 0x1C0, 0xDDDDDC00, 0x7774, 0xDC701C00, 0x74DDC771, 0x704D34D3, 0x7000, 0, 0x1C000000, 0x1DDC7777, 0xD000D00, 0x34D371D, 0x8A5D034D, 0xEB42, 0}},
		{{0x7474422, 0x1C774074, 0x34D31C1C, 0x1C0, 0xDC770070, 0x4001DDDD, 0xC0000D03, 0xC1C70001, 0x71C71D, 0xD00000, 0x40D00034, 0xD00D03, 0xC74D0700, 0x771DC771, 0x3777, 0xE904AD1C, 0xCFAC, 0}},
		{{0x101D422, 0x1C7741D, 0x34C70000, 0x34000, 0xDDDC7000, 0x40001DDD, 0xC0700003, 0x37777071, 0x1DD34DD, 0x1C070707, 0x1C1C1C70, 0x71C70707, 0x74DDD377, 0x3470, 0x377774D0, 0x92804D0D, 0xE843, 0}},
	},
	{0xE50387, 0xFFFFFF,
		{{0x40441122, 0xD0D1C40, 0xC34CDC7D, 0x4DC71C71, 0x40D00D03, 0xD34D347, 0xD, 0xC71C71C0, 0x340D0D01, 0x40340340, 0xC0003403, 0x374DD371, 0x701C071D, 0xDC, 0xDC410000, 0xC0041DDD, 0xE8, 0}},
		{{0x1CAD1122, 0x435C72B1, 0x37DC0037, 0xDC71DC00, 0x343434DD, 0x74, 0xC001C00D, 0xDC71C701, 0xC71D001D, 0xC71DC771, 0x9377771D, 0xD0000DDE, 0xDDD34000, 0x71DDDD, 0x1D00007, 0xA771C000, 0x7A7037, 0}},
		{{0x40441122, 0x34340740, 0xDC3700, 0x4DC71C77, 0xD00003, 0x34D34D1D, 0xD, 0x71C71C70, 0xDDDDDD00, 0xDDDDDDDD, 0x70D374DD, 0x1DDDDDDC, 0x1C01C007, 0x40D03437, 0xD00003, 0x80157400, 0xE9E, 0}},
	},
	{0xC20385, 0xFFFFFF,
		{{0xB4044122, 0xDDDD0772, 0xC3700371, 0x71, 0x1C0000, 0x34000007, 0x70001C00, 0x7000, 0, 0x70000, 0xD0000000, 0x7000000, 0x1C0, 0x40343400, 0x1C707003, 0xDC4D0007, 0xE9C, 0}},
		{{0x74044122, 0x771D001D, 0xC0030037, 0x7001D, 0x7001C0, 0, 0xC70701C0, 0x1C01, 0x1C00, 0x1C001C0, 0xC01C01C0, 0x1C01, 0x1C00700, 0x7001C, 0xC0007007, 0x7D011001, 0x3A, 0}},
		{{0x1D744122, 0xDC74D1C4, 0xC00DD, 7, 0x1C0001C0, 0, 0x701C000, 0x7001C007, 0, 0x700007, 0x70001C0, 0x70701C00, 0x70070, 0x701C007, 0x71C1C1C, 0xCC4411C7, 0xC, 0}},
	},
	{0xD10384, 0xFFFFFF,
		{{0xB4110522, 0x3434772, 0xD374C030, 0x73777771, 0x1C771C, 0x4D343434, 0xD34DD37, 0x7A40000, 0x70000, 0x701C0070, 0xDDC71C70, 0x4343434D, 0xDDC77477, 0x71C707A4, 0xD0D371C, 0x71D34, 0xCDF4, 0}},
		{{0xB4110522, 0xC0D00132, 0x1C00C0D, 0xD34D0D34, 0x400D0000, 0x4D3434D3, 0x403434D3, 3, 0x1C000, 0x1C00000, 0x77070700, 0x1374D377, 0xD00D07, 0xD3771DDC, 0x771C71DD, 0xD000137, 0xF7, 0}},
		{{0x1110522, 0x371C7401, 0x10035F70, 0xD0D37003, 0x771C74D0, 0xDDDDDDDC, 0xD0034, 0x700000, 0xC1C07070, 0x71C71DC1, 0xDDDDDDDC, 0x3471DD, 0xD0D3000, 0xD0D0D, 0x40000340, 0xC0D00D03, 0, 0}},
	},
	{0x970787, 0xFFFFFF,
		{{0x40135022, 0x1D001D00, 0x71D3400, 0x71C07007, 0x771C0D37, 0x34D0D3, 0x71C00000, 0xD01DC71C, 0xC774D374, 0x71DDDDDD, 0xD0CD377, 0xC1DDD34D, 0xDC701C01, 0x37774DDD, 0x34D0D34D, 0x4040340D, 3, 0}},
		{{0xD1DD1022, 0x7401DD71, 0x74D00007, 0xD34DDDD3, 0xDDDC00, 0x4D34D340, 0x34003, 0x1C0070, 0x40D00000, 0xD034343, 0x7343434D, 0x71DC770, 0xD0343400, 0x7000001C, 0x1C01C000, 0xCC70000, 0, 0}},
		{{0x1D01022, 0x1D34710, 0x1C74D00, 0x701C0007, 0x7A40D31C, 0x1C00, 0x70D0034, 0xDC7001C0, 0x1C7771DD, 0, 0, 0xC0000D00, 0x77771DDD, 0x34340D0, 0x340, 0xA174D0D0, 0xC44, 0}},
	},
	{0xD10784, 0xFFFFFF,
		{{0x2B1D1022, 0x74013, 0x4D035C1D, 0x374DDD3, 0xDDC70D, 0xD03401C, 0xD374DD0D, 0x340D, 0, 0x1C001C, 0x700070, 0x1C0007, 0xD00032B0, 0xDDDDC700, 0x1C713774, 0x1DD071C7, 0xDC7, 0}},
		{{0x40135022, 0x74D71D07, 0xD34D0007, 0xD34DDD, 0xDC00DDC0, 1, 0x34D34D0D, 0xD000D, 0x70000000, 0, 0, 0x34000D0, 0x34DD374, 0x1C71DC70, 0x1DC771C7, 0x70707007, 0xC, 0}},
		{{0x741022, 0x1D35C4, 0xD0701C4D, 0x34000, 0xC00D31C7, 0xD3740001, 0x77771374, 0x340D377, 0xD, 0x734, 0x7000000, 0, 0x40D0D000, 0xDC771C73, 0x70000071, 0xD0340D0, 0, 0}},
	},
	{0xC20785, 0xFFFFFF,
		{{0x74740422, 0x1D00740, 0xC01C7740, 0xDC700001, 0x34DC734D, 0xC0000000, 0x7771C1C1, 0x774DD377, 0x71C71C1C, 0x1C71C71C, 0xC71C71C7, 0x701C071, 0xD0007, 0x7070000, 0x71C1C707, 0x70401C, 0xC, 0}},
		{{0x74740422, 0x74004D7, 0x1C777400, 0x371C7070, 0x34C700D, 0xC0000700, 0xC71C71C1, 0xDD3774DD, 0xC0701C1D, 0x1C0701C1, 0x70070070, 0x1C1C0700, 0xD00D070, 0x1C000000, 0x7070700, 0x704007, 0xC, 0}},
		{{0x40135022, 0xDD01DD07, 0x774D0001, 0x34DDDC77, 0x377000, 0x3400, 0xC0701C00, 0x77777771, 0x1C077, 0x7001C007, 0x7001C00, 0xC01C0070, 0xD0340001, 0x340, 0xC07001C0, 0x1C1D001, 0xC, 0}},
	},
	{0xC20784, 0xFFFFFF,
		{{0x1D01022, 0x1D0074, 0x341C1DD0, 0xD34D34D0, 0x70000034, 0x7070701C, 0x377771DC, 0x3400D00D, 0x70001C, 0xDC71C1C7, 0x7771DC71, 0x77771DDC, 0xD034007, 0x71C71C00, 0x1DC771C7, 0x701D077, 0xC, 0}},
		{{0x1D01022, 0x1C740010, 0xB0071D34, 0xD00032, 0x771C0000, 0x77774DD3, 0xD3777777, 0xD0034D, 0x1C0000, 0x701C0070, 0x1C0701C0, 0x1C1C1C1C, 0xDC74001C, 0x1C000DD, 0x7771C707, 0x7171DC, 0, 0}},
		{{0x7401022, 0x74D0774, 0x1C1DDD00, 0xD000D000, 0x71C1C000, 0x71C71DC7, 0x7777771C, 0x3400D037, 0x7000000, 0x70701C0, 0xC7070707, 0xC71C1C71, 0x4DD34071, 0xC0700003, 0xC1C1C1C1, 0x1C75C1, 0xC, 0}},
	},
};

static LCDInfo LCDInfoTable[] = {
	{"n72", 0xB, 0xA4CB80, 0xA3, 320, 0xC, 0xC, 0x10, 480, 6, 6, 8, 0, 0, 0, 0, 24, 3, 0x222},
};

static const PMURegisterData backlightOffData = {0x31, 0x0};

static const PMURegisterData backlightData[] = {
	{0x30, 0x30},
	{0x31, 0x5}
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
static void framebuffer_fill(Framebuffer* framebuffer, int x, int y, int width, int height, int fill);
static void hline_rgb888(Framebuffer* framebuffer, int start, int line_no, int length, int fill);
static void hline_rgb565(Framebuffer* framebuffer, int start, int line_no, int length, int fill);
static void vline_rgb888(Framebuffer* framebuffer, int start, int line_no, int length, int fill);
static void vline_rgb565(Framebuffer* framebuffer, int start, int line_no, int length, int fill);

void displaytime_sleep(uint32_t time) {
	task_sleep(time * TimePerMillionFrames / 1000);
}

void sub_5FF08870(uint8_t arg) {
	uint8_t v1;

	v1 = (1049-50*arg)/50;
	pmu_write_reg(0x6B, v1, 1);
	task_sleep(10);
	pmu_write_reg(0x6C, v1 + 4, 1);
}

void lcd_fill_switch(OnOff on_off, uint32_t color) {
	if (on_off == ON) {
		if (color)
			framebufferLastFill = color;

		framebuffer_fill(&currentWindow->framebuffer, 0, 0, currentWindow->framebuffer.width, currentWindow->framebuffer.height, color);
		SET_REG(CLCD, GET_REG(CLCD) | 1);
		// This line (and the corresponding turn off line) are needed on the 2g touch for some
		// reason. I don't think that any other device requires them.
		// --kleemajo
		SET_REG(CLCD + 0x1b10, GET_REG(CLCD + 0x1b10) | 1);
	} else {
		SET_REG(CLCD, GET_REG(CLCD) & (~1));
		while (!(GET_REG(CLCD) & 2)) ;
		SET_REG(CLCD + 0x1b10, GET_REG(CLCD + 0x1b10) & (~1));
		while (!(GET_REG(CLCD + 0x1b10) & 2));
	}
	SyncFramebufferToDisplayActivated = on_off;
}

void lcd_fill(uint32_t color) {
	lcd_fill_switch(ON, color);
}

void lcd_shutdown() {
	if (LCDTable) {
		framebuffer_fill(&currentWindow->framebuffer, 0, 0, currentWindow->framebuffer.width, currentWindow->framebuffer.height, framebufferLastFill);
		task_sleep(TimePerMillionFrames * 2);
		
		// Removed because it halts for some reason. -Oranav
		//if (LCDTable->unkn17 == 3)
		//	pinot_quiesce();
		
		lcd_fill_switch(OFF, 0);
	}
}

int displaypipe_init() {
	int result = 0;
	uint32_t panelID;

	memset((void*)CLCD_FRAMEBUFFER, 0, 0x300000);

	bufferPrintf("displaypipe_init: framebuffer address: 0x%08x\n", CLCD_FRAMEBUFFER);

	if (!LCDTable)
		LCDTable = &LCDInfoTable[DISPLAYID];
	
	clock_gate_switch(0x2, ON);

	SET_REG(CLCD, 0x100);
	while(GET_REG(CLCD) & 0x100);		// loop until change takes
	
	udelay(1);

	SET_REG(CLCD, 0x20084);
	SET_REG(CLCD + 0x300, 0x80000001);
	if (LCDTable->bitsPerPixel <= 18)
		SET_REG(CLCD + 0x300, GET_REG(CLCD + 0x300) | 0x1110000);
	SET_REG(CLCD + 0x1B10, 0);
	
	SET_REG(CLCD + 0x1B14, (LCDTable->IVClk << VIDCON1_IVCLKSHIFT)
		| (LCDTable->IHSync << VIDCON1_IHSYNCSHIFT) | (LCDTable->IVSync << VIDCON1_IVSYNCSHIFT)
		| (LCDTable->IVDen << VIDCON1_IVDENSHIFT));
	SET_REG(CLCD + 0x1B1C, ((LCDTable->verticalBackPorch - 1) << VIDTCON_BACKPORCHSHIFT)
		| ((LCDTable->verticalFrontPorch - 1) << VIDTCON_FRONTPORCHSHIFT)
		| ((LCDTable->verticalSyncPulseWidth - 1) << VIDTCON_SYNCPULSEWIDTHSHIFT));
	SET_REG(CLCD + 0x1B20, ((LCDTable->horizontalBackPorch - 1) << VIDTCON_BACKPORCHSHIFT)
		| ((LCDTable->horizontalFrontPorch - 1) << VIDTCON_FRONTPORCHSHIFT)
		| ((LCDTable->horizontalSyncPulseWidth - 1) << VIDTCON_SYNCPULSEWIDTHSHIFT));
	SET_REG(CLCD + 0x1B24, ((LCDTable->width - 1) << VIDTCON2_HOZVALSHIFT)
		| ((LCDTable->height - 1) << VIDTCON2_LINEVALSHIFT));
	TimePerMillionFrames = 1000000
		* (LCDTable->verticalBackPorch
			+ LCDTable->verticalFrontPorch
			+ LCDTable->verticalSyncPulseWidth
			+ LCDTable->height)
		* (LCDTable->horizontalBackPorch
			+ LCDTable->horizontalFrontPorch
			+ LCDTable->horizontalSyncPulseWidth
			+ LCDTable->width)
		/ LCDTable->drivingClockFrequency;

	ColorSpace colorSpace;
//XXX:	It normally grabs it from nvram var "display-color-space" as string. -- Bluerise
	colorSpace = RGB888;

	currentWindow = createWindow(0, 0, LCDTable->width, LCDTable->height, colorSpace);
	if (!currentWindow)
		return -1;

//XXX:	It sets the framebuffer address into nvram var "framebuffer". -- Bluerise
//	nvram_setvar("framebuffer", currentWindow->framebuffer.buffer, 0);
	if (LCDTable->unkn17 == 3) {
		result = pinot_init(LCDTable, colorSpace, &panelID, currentWindow);
		if (result) {
			lcd_fill_switch(OFF, 0);
			LCDTable = NULL;
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
	
	buffer1[0] = 0;
	buffer2[0] = 0;
	buffer3[0] = 0;
	
	uint32_t curBuf;
	for (curBuf = 1; curBuf != 256; curBuf++) {
		buffer1[curBuf] = 4*(curBuf-1);
		buffer2[curBuf] = 4*(curBuf-1);
		buffer3[curBuf] = 4*(curBuf-1);
	}

	installGammaTables(panelID, 257, (uint32_t)buffer1, (uint32_t)buffer2, (uint32_t)buffer3);
	setWindowBuffer(4, buffer1);
	setWindowBuffer(5, buffer2);
	setWindowBuffer(6, buffer3);
	SET_REG(CLCD + 0x400, 1);
	SET_REG(CLCD + 0x1B38, 128);
	
	free(buffer1);
	free(buffer2);
	free(buffer3);

	CurFramebuffer = currentWindow->framebuffer.buffer;

	return result;
}

static uint8_t* PanelIDInfo;
static uint8_t dword_5FF3AE0C;

int pinot_init(LCDInfo* LCDTable, ColorSpace colorspace, uint32_t* panelID, Window* currentWindow) {
	uint32_t pinot_default_color = 0;
	uint32_t pinot_backlight_cal = 0;
	uint32_t pinot_panel_id = 0;

	bufferPrintf("pinot_init()\r\n");
	gpio_pin_output(0x107, OFF);
	task_sleep(10);
	mipi_dsim_init(LCDTable);
	gpio_pin_output(0x107, ON);
	task_sleep(6);
	mipi_dsim_write_data(5, 0, 0);
	udelay(10);
	mipi_dsim_framebuffer_on_off(ON);
	task_sleep(25);

//XXX	If there's no PanelID it tries to grab it from nvram entry "pinot-panel-id". We don't have nvram yet. -- Bluerise
	if (!pinot_panel_id) {
		uint32_t read_length;
		read_length = 15;
		PanelIDInfo = malloc(16);
		memset(PanelIDInfo, 0x0, 16);
		PanelIDInfo[0] = 0xB1; // -79
		if (mipi_dsim_read_write(0x14, PanelIDInfo, &read_length) || read_length <= 2) {
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

	uint32_t on_off = 0;
	if ((GET_BITS(pinot_panel_id, 8, 8) - 3) <= 1 || GET_BITS(pinot_panel_id, 8, 8) == 7 || GET_BITS(pinot_panel_id, 8, 8) == 8) {
		mipi_dsim_on_off(ON);
		on_off = 1;
	}

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
	displaytime_sleep(4);

	if(!on_off)
		mipi_dsim_on_off(ON);

	gpio_pulldown_configure(0x106, GPIOPDDisabled);

	*panelID = pinot_panel_id;

	return 0;
}

void pinot_quiesce() {
	bufferPrintf("pinot_quiesce()\r\n");
	mipi_dsim_write_data(5, 0x28, 0);
	mipi_dsim_write_data(5, 0x10, 0);
	displaytime_sleep(6);
	mipi_dsim_framebuffer_on_off(OFF);
	mipi_dsim_quiesce();

	gpio_pin_output(0x107, OFF);

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

	SET_REG(CLCD + 0x408, window_bit | 0x10000);

	for (cur_size = 0; cur_size < size; cur_size++) {
		to_set = *buffer;
		if ((window - 4) <= 2)
			to_set = (*(buffer+1) & 0x3FF) | ((*buffer << 10) & 0xFFC00);
		SET_REG(CLCD + 0x40C, to_set | (1 << 31));
		buffer++;
	}

	SET_REG(CLCD + 0x408, 0);
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

	createFramebuffer(&newWindow->framebuffer, CLCD_FRAMEBUFFER, width, height, width, colorSpace);

	// setup the window parameters
	SET_REG(CLCD + 0x20, (reg_bit << 8) | 0x200000);
	SET_REG(CLCD + 0x24, (uint32_t)newWindow->framebuffer.buffer);
	SET_REG(CLCD + 0x28, width);
	SET_REG(CLCD + 0x2C, 0);
	SET_REG(CLCD + 0x30, (width << 16) | height);
	SET_REG(CLCD + 0x38, 0);
	SET_REG(CLCD + 0x3C, 0);
	SET_REG(CLCD + 0x34, (zero0 << 16) | zero2);
	
	// mark the window as created
	SET_REG(CLCD + 0x4, GET_REG(CLCD + 0x4) | 0x10);
	SET_REG(CLCD + 0x10, GET_REG(CLCD + 0x10) | 0x80);
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

void lcd_set_backlight_level(int level) {
	if(level == 0) {
		pmu_write_regs(&backlightOffData, 1);
	} else { 
		PMURegisterData myBacklightData[sizeof(backlightData)/sizeof(PMURegisterData)];

		memcpy(myBacklightData, backlightData, sizeof(myBacklightData));

		if (level > LCD_MAX_BACKLIGHT) {
			level = LCD_MAX_BACKLIGHT;
		}
		int i;
		for(i = 0; i < (sizeof(myBacklightData)/sizeof(PMURegisterData)); i++) {
			if(myBacklightData[i].reg == LCD_BACKLIGHT_REG) {
				myBacklightData[i].data = level & LCD_BACKLIGHT_REGMASK;
			}
		}
		pmu_write_regs(myBacklightData, sizeof(myBacklightData)/sizeof(PMURegisterData));
	}
}


void cmd_backlight(int argc, char** argv) {
	if(argc < 2) {
		bufferPrintf("Usage: %s <0-%d>\r\n", argv[0], LCD_MAX_BACKLIGHT);
		return;
	}

	uint32_t level = parseNumber(argv[1]);
	lcd_set_backlight_level(level);
	bufferPrintf("backlight set to %d\r\n", level);
}
COMMAND("backlight", "set the backlight level", cmd_backlight);
