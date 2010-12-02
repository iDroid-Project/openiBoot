#ifndef SMALL
#ifndef NO_STBIMAGE
#ifndef CONFIG_A4 // TODO: Fix for A4

#include "openiboot.h"
#include "lcd.h"
#include "util.h"
#include "framebuffer.h"
#include "buttons.h"
#include "timer.h"
#include "images/ConsolePNG.h"
#include "images/iPhoneOSPNG.h"
#include "images/AndroidOSPNG.h"
#include "images/ConsoleSelectedPNG.h"
#include "images/iPhoneOSSelectedPNG.h"
#include "images/AndroidOSSelectedPNG.h"
#include "images/HeaderPNG.h"
#include "images.h"
#include "actions.h"
#include "stb_image.h"
#include "pmu.h"
#include "nand.h"
#include "radio.h"
#include "hfs/fs.h"
#include "ftl.h"
#include "scripting.h"
#include "multitouch.h"
#include "nvram.h"

int globalFtlHasBeenRestored; /* global variable to tell wether a ftl_restore has been done*/

static uint32_t FBWidth;
static uint32_t FBHeight;


static uint32_t* imgiPhoneOS;
static int imgiPhoneOSWidth;
static int imgiPhoneOSHeight;
static int imgiPhoneOSX;
static int imgiPhoneOSY;

static uint32_t* imgConsole;
static int imgConsoleWidth;
static int imgConsoleHeight;
static int imgConsoleX;
static int imgConsoleY;

static uint32_t* imgAndroidOS;
static int imgAndroidOSWidth;
static int imgAndroidOSHeight;
static int imgAndroidOSX;
static int imgAndroidOSY;

static uint32_t* imgiPhoneOSSelected;
static uint32_t* imgConsoleSelected;
static uint32_t* imgAndroidOSSelected;

static uint32_t* imgHeader;
static int imgHeaderWidth;
static int imgHeaderHeight;
static int imgHeaderX;
static int imgHeaderY;

typedef enum MenuSelection {
	MenuSelectioniPhoneOS,
	MenuSelectionConsole,
	MenuSelectionAndroidOS
} MenuSelection;

static MenuSelection Selection;

volatile uint32_t* OtherFramebuffer;

static int load_multitouch_images()
{
    #ifdef CONFIG_IPHONE_2G
        Image* image = images_get(fourcc("mtza"));
        if (image == NULL) {
            return 0;
        }
        void* aspeedData;
        size_t aspeedLength = images_read(image, &aspeedData);
        
        image = images_get(fourcc("mtzm"));
        if(image == NULL) {
            return 0;
        }
        
        void* mainData;
        size_t mainLength = images_read(image, &mainData);
        
        multitouch_setup(aspeedData, aspeedLength, mainData,mainLength);
        free(aspeedData);
        free(mainData);
    #else
        Image* image = images_get(fourcc("mtz2"));
        if(image == NULL) {
            return 0;
        }
        void* imageData;
        size_t length = images_read(image, &imageData);
        
        multitouch_setup(imageData, length);
        free(imageData);
    #endif
    return 1;
}

static void reset_tempos()
{
	framebuffer_setdisplaytext(FALSE);
	nvram_setvar("opib-temp-os","0");
	nvram_save();
	framebuffer_setdisplaytext(TRUE);
}

static void drawSelectionBox() {
	volatile uint32_t* oldFB = CurFramebuffer;

	CurFramebuffer = OtherFramebuffer;
	currentWindow->framebuffer.buffer = CurFramebuffer;
	OtherFramebuffer = oldFB;

	if(Selection == MenuSelectioniPhoneOS) {
		framebuffer_draw_image(imgiPhoneOSSelected, imgiPhoneOSX, imgiPhoneOSY, imgiPhoneOSWidth, imgiPhoneOSHeight);
		framebuffer_draw_image(imgConsole, imgConsoleX, imgConsoleY, imgConsoleWidth, imgConsoleHeight);
		framebuffer_draw_image(imgAndroidOS, imgAndroidOSX, imgAndroidOSY, imgAndroidOSWidth, imgAndroidOSHeight);
	}

	if(Selection == MenuSelectionConsole) {
		framebuffer_draw_image(imgiPhoneOS, imgiPhoneOSX, imgiPhoneOSY, imgiPhoneOSWidth, imgiPhoneOSHeight);
		framebuffer_draw_image(imgConsoleSelected, imgConsoleX, imgConsoleY, imgConsoleWidth, imgConsoleHeight);
		framebuffer_draw_image(imgAndroidOS, imgAndroidOSX, imgAndroidOSY, imgAndroidOSWidth, imgAndroidOSHeight);
	}

	if(Selection == MenuSelectionAndroidOS) {
		framebuffer_draw_image(imgiPhoneOS, imgiPhoneOSX, imgiPhoneOSY, imgiPhoneOSWidth, imgiPhoneOSHeight);
		framebuffer_draw_image(imgConsole, imgConsoleX, imgConsoleY, imgConsoleWidth, imgConsoleHeight);
		framebuffer_draw_image(imgAndroidOSSelected, imgAndroidOSX, imgAndroidOSY, imgAndroidOSWidth, imgAndroidOSHeight);
	}

	lcd_window_address(2, (uint32_t) CurFramebuffer);
}

static int touch_watcher()
{
    uint8_t isFound = 0;
    multitouch_run();
    if (multitouch_ispoint_inside_region(imgiPhoneOSX, imgiPhoneOSY, imgiPhoneOSWidth, imgiPhoneOSHeight) == TRUE) {
        Selection = MenuSelectioniPhoneOS;
        isFound = 1;
    }
    else if (multitouch_ispoint_inside_region(imgConsoleX, imgConsoleY, imgConsoleWidth, imgConsoleHeight) == TRUE) {
        Selection = MenuSelectionConsole;
        isFound = 1;
    }
    else if (multitouch_ispoint_inside_region(imgAndroidOSX, imgAndroidOSY, imgAndroidOSWidth, imgAndroidOSHeight) == TRUE) {
        Selection = MenuSelectionAndroidOS;
        isFound = 1;
    }
    
    if (isFound ==1) {
        drawSelectionBox();
        return TRUE;
    }
    return FALSE;
}

static void toggle(int forward) {
	if(forward)
	{
		if(Selection == MenuSelectioniPhoneOS)
			Selection = MenuSelectionConsole;
		else if(Selection == MenuSelectionConsole)
			Selection = MenuSelectionAndroidOS;
		else if(Selection == MenuSelectionAndroidOS)
			Selection = MenuSelectioniPhoneOS;
	} else
	{
		if(Selection == MenuSelectioniPhoneOS)
			Selection = MenuSelectionAndroidOS;
		else if(Selection == MenuSelectionAndroidOS)
			Selection = MenuSelectionConsole;
		else if(Selection == MenuSelectionConsole)
			Selection = MenuSelectioniPhoneOS;
	}

	drawSelectionBox();
}

int menu_setup(int timeout, int defaultOS) {
	FBWidth = currentWindow->framebuffer.width;
	FBHeight = currentWindow->framebuffer.height;	

	imgiPhoneOS = framebuffer_load_image(dataiPhoneOSPNG, dataiPhoneOSPNG_size, &imgiPhoneOSWidth, &imgiPhoneOSHeight, TRUE);
	imgiPhoneOSSelected = framebuffer_load_image(dataiPhoneOSSelectedPNG, dataiPhoneOSSelectedPNG_size, &imgiPhoneOSWidth, &imgiPhoneOSHeight, TRUE);
	imgConsole = framebuffer_load_image(dataConsolePNG, dataConsolePNG_size, &imgConsoleWidth, &imgConsoleHeight, TRUE);
	imgConsoleSelected = framebuffer_load_image(dataConsoleSelectedPNG, dataConsoleSelectedPNG_size, &imgConsoleWidth, &imgConsoleHeight, TRUE);
	imgAndroidOS = framebuffer_load_image(dataAndroidOSPNG, dataAndroidOSPNG_size, &imgAndroidOSWidth, &imgAndroidOSHeight, TRUE);
	imgAndroidOSSelected = framebuffer_load_image(dataAndroidOSSelectedPNG, dataAndroidOSSelectedPNG_size, &imgAndroidOSWidth, &imgAndroidOSHeight, TRUE);
	imgHeader = framebuffer_load_image(dataHeaderPNG, dataHeaderPNG_size, &imgHeaderWidth, &imgHeaderHeight, TRUE);

	bufferPrintf("menu: images loaded\r\n");

	imgiPhoneOSX = (FBWidth - imgiPhoneOSWidth) / 2;
	imgiPhoneOSY = 84;

	imgConsoleX = (FBWidth - imgConsoleWidth) / 2;
	imgConsoleY = 207;

	imgAndroidOSX = (FBWidth - imgAndroidOSWidth) / 2;
	imgAndroidOSY = 330;

	imgHeaderX = (FBWidth - imgHeaderWidth) / 2;
	imgHeaderY = 17;

	framebuffer_draw_image(imgHeader, imgHeaderX, imgHeaderY, imgHeaderWidth, imgHeaderHeight);


	framebuffer_setloc(0, 47);
	framebuffer_setcolors(COLOR_WHITE, COLOR_BLACK);
	framebuffer_print_force(OPENIBOOT_VERSION_STR);
	framebuffer_setloc(0, 0);

	switch(defaultOS){
		case 1:
			Selection = MenuSelectioniPhoneOS;
			break;
		case 2:
			Selection = MenuSelectionAndroidOS;
			break;
		case 3:
			Selection = MenuSelectionConsole;
			break;
		default:
			Selection = MenuSelectioniPhoneOS;
	}

	OtherFramebuffer = CurFramebuffer;
	CurFramebuffer = (volatile uint32_t*) NextFramebuffer;

	drawSelectionBox();

	pmu_set_iboot_stage(0);

	memcpy((void*)NextFramebuffer, (void*) CurFramebuffer, NextFramebuffer - (uint32_t)CurFramebuffer);

	uint64_t startTime = timer_get_system_microtime();
	int timeoutLeft = (timeout / 1000);
	while(TRUE) {
		char timeoutstr[4] = "";
		if(timeout > 0){
			sprintf(timeoutstr, "%2d", timeoutLeft);
			uint64_t checkTime = timer_get_system_microtime();
			timeoutLeft = (timeout - ((checkTime - startTime)/1000))/1000;
			framebuffer_setloc(49, 47);
			framebuffer_print_force(timeoutstr);
			framebuffer_setloc(0,0);
			drawSelectionBox();
		} else if(timeout == -1) {
			timeoutLeft = -1;
			sprintf(timeoutstr, "  ");
			framebuffer_setloc(49, 47);
			framebuffer_print_force(timeoutstr);
			framebuffer_setloc(0,0);
			drawSelectionBox();
		}
		if (isMultitouchLoaded && touch_watcher()) {
			break;
        	}
		if(buttons_is_pushed(BUTTONS_HOLD)) {
			toggle(TRUE);
			startTime = timer_get_system_microtime();
			timeout = -1;
			udelay(200000);
		}
#ifndef CONFIG_IPOD
		if(!buttons_is_pushed(BUTTONS_VOLUP)) {
			toggle(FALSE);
			startTime = timer_get_system_microtime();
			timeout = -1;
			udelay(200000);
		}
		if(!buttons_is_pushed(BUTTONS_VOLDOWN)) {
			toggle(TRUE);
			startTime = timer_get_system_microtime();
			timeout = -1;
			udelay(200000);
		}
#endif
		if(buttons_is_pushed(BUTTONS_HOME)) {
			timeout = -1;
			break;
		}
		if(timeout > 0 && has_elapsed(startTime, (uint64_t)timeout * 1000)) {
			bufferPrintf("menu: timed out, selecting current item\r\n");
			break;
		}
		udelay(10000);
	}

	if(Selection == MenuSelectioniPhoneOS) {
		Image* image = images_get(fourcc("ibox"));
		if(image == NULL)
			image = images_get(fourcc("ibot"));
		void* imageData;
		images_read(image, &imageData);
		chainload((uint32_t)imageData);
	}

	if(Selection == MenuSelectionConsole) {
		// Reset framebuffer back to original if necessary
		if((uint32_t) CurFramebuffer == NextFramebuffer)
		{
			CurFramebuffer = OtherFramebuffer;
			currentWindow->framebuffer.buffer = CurFramebuffer;
			lcd_window_address(2, (uint32_t) CurFramebuffer);
		}

		framebuffer_setdisplaytext(TRUE);
		framebuffer_clear();
	}

	if(Selection == MenuSelectionAndroidOS) {
		// Reset framebuffer back to original if necessary
		if((uint32_t) CurFramebuffer == NextFramebuffer)
		{
			CurFramebuffer = OtherFramebuffer;
			currentWindow->framebuffer.buffer = CurFramebuffer;
			lcd_window_address(2, (uint32_t) CurFramebuffer);
		}

		framebuffer_setdisplaytext(TRUE);
		framebuffer_clear();

#ifndef NO_HFS
#ifndef CONFIG_IPOD
		radio_setup();
#endif
		nand_setup();
		fs_setup();
		if(globalFtlHasBeenRestored) /* if ftl has been restored, sync it, so kernel doesn't have to do a ftl_restore again */
		{
			if(ftl_sync())
			{
				bufferPrintf("ftl synced successfully");
			}
			else
			{
				bufferPrintf("error syncing ftl");
			}
		}

		pmu_set_iboot_stage(0);
		boot_linux_from_files();
#endif
	}

	return 0;
}

void menu_init()
{
	framebuffer_setdisplaytext(TRUE);
	framebuffer_clear();
	bufferPrintf("Loading openiBoot...");
#ifndef SMALL
#ifndef NO_STBIMAGE
	int defaultOS = 0;
	int tempOS = 0;
	const char* hideMenu = nvram_getvar("opib-hide-menu");
	const char* sDefaultOS = nvram_getvar("opib-default-os");
	const char* sTempOS = nvram_getvar("opib-temp-os");
	if(sDefaultOS)
		defaultOS = parseNumber(sDefaultOS);
	if(sTempOS)
		tempOS = parseNumber(sTempOS);
	if(tempOS>0) {	
		switch (tempOS) {
			case 1:
				framebuffer_clear();
				bufferPrintf("Loading iOS...\r\n");
				reset_tempos();
				Image* image = images_get(fourcc("ibox"));
				if(image == NULL)
					image = images_get(fourcc("ibot"));
				void* imageData;
				images_read(image, &imageData);
				chainload((uint32_t)imageData);
				break;
			
			case 2:
				framebuffer_clear();
				bufferPrintf("Loading iDroid...\r\n");
				reset_tempos();
#ifndef NO_HFS
#ifndef CONFIG_IPOD
				radio_setup();
#endif
				nand_setup();
				fs_setup();
				if(globalFtlHasBeenRestored) {
					if(ftl_sync()) {
						bufferPrintf("ftl synced successfully\r\n");
					} else {
						bufferPrintf("error syncing ftl\r\n");
					}
				}	
				pmu_set_iboot_stage(0);
				boot_linux_from_files();
#endif
				break;
				
			case 3:
				framebuffer_clear();
				bufferPrintf("Loading Console...\r\n");
				reset_tempos();
				hideMenu = "1";
				break;
		}
	} else if(hideMenu && (strcmp(hideMenu, "1") == 0 || strcmp(hideMenu, "true") == 0)) {
		bufferPrintf("Boot menu hidden. Use 'setenv opib-hide-menu false' and then 'saveenv' to unhide.\r\n");
	} else {
        	framebuffer_setdisplaytext(FALSE);
        	isMultitouchLoaded = load_multitouch_images();
		framebuffer_clear();
		const char* sMenuTimeout = nvram_getvar("opib-menu-timeout");
		int menuTimeout = -1;
		if(sMenuTimeout)
			menuTimeout = parseNumber(sMenuTimeout);
		menu_setup(menuTimeout, defaultOS);
	}
#endif
#endif

	OpenIBootConsole();
}

static void menu_init_boot()
{
	OpenIBootMain = &menu_init;
}
MODULE_INIT_BOOT(menu_init_boot);

#endif
#endif
#endif

