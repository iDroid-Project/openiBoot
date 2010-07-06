#ifndef SMALL
#ifndef NO_STBIMAGE

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
static uint32_t* imgAndroidOS_unblended;
static int imgAndroidOSWidth;
static int imgAndroidOSHeight;
static int imgAndroidOSX;
static int imgAndroidOSY;

static uint32_t* imgiPhoneOSSelected;
static uint32_t* imgConsoleSelected;
static uint32_t* imgAndroidOSSelected;
static uint32_t* imgAndroidOSSelected_unblended;

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
	imgAndroidOS_unblended = framebuffer_load_image(dataAndroidOSPNG, dataAndroidOSPNG_size, &imgAndroidOSWidth, &imgAndroidOSHeight, TRUE);
	imgAndroidOSSelected_unblended = framebuffer_load_image(dataAndroidOSSelectedPNG, dataAndroidOSSelectedPNG_size, &imgAndroidOSWidth, &imgAndroidOSHeight, TRUE);
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

	framebuffer_draw_rect_hgradient(0, 42, 0, 360, FBWidth, (FBHeight - 12) - 360);
	framebuffer_draw_rect_hgradient(0x22, 0x22, 0, FBHeight - 12, FBWidth, 12);

	framebuffer_setloc(0, 47);
	framebuffer_setcolors(COLOR_WHITE, 0x222222);
	framebuffer_print_force(OPENIBOOT_VERSION_STR);
	framebuffer_setcolors(COLOR_WHITE, COLOR_BLACK);
	framebuffer_setloc(0, 0);

	imgAndroidOS = malloc(imgAndroidOSWidth * imgAndroidOSHeight * sizeof(uint32_t));
	imgAndroidOSSelected = malloc(imgAndroidOSWidth * imgAndroidOSHeight * sizeof(uint32_t));

	framebuffer_capture_image(imgAndroidOS, imgAndroidOSX, imgAndroidOSY, imgAndroidOSWidth, imgAndroidOSHeight);
	framebuffer_capture_image(imgAndroidOSSelected, imgAndroidOSX, imgAndroidOSY, imgAndroidOSWidth, imgAndroidOSHeight);

	framebuffer_blend_image(imgAndroidOS, imgAndroidOSWidth, imgAndroidOSHeight, imgAndroidOS_unblended, imgAndroidOSWidth, imgAndroidOSHeight, 0, 0);
	framebuffer_blend_image(imgAndroidOSSelected, imgAndroidOSWidth, imgAndroidOSHeight, imgAndroidOSSelected_unblended, imgAndroidOSWidth, imgAndroidOSHeight, 0, 0);

	switch(defaultOS){
		case 0:
			Selection = MenuSelectioniPhoneOS;
			break;
		case 1:
			Selection = MenuSelectionAndroidOS;
			break;
		case 2:
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
		if(timeout >= 0){
			if(timeoutLeft == 9){
				sprintf(timeoutstr, "  ");
				framebuffer_setloc(49, 47);
				framebuffer_print_force(timeoutstr);
				framebuffer_setloc(0,0);
			}
			sprintf(timeoutstr, "%d", timeoutLeft);
			if(has_elapsed(startTime, (uint64_t)(timeout - (timeoutLeft * 1000)) * 1000)){
				timeoutLeft -= 1;
			}
			framebuffer_setloc(49, 47);
			framebuffer_print_force(timeoutstr);
			framebuffer_setloc(0,0);
		} else if(timeout == -1) {
			timeoutLeft = -1;
			sprintf(timeoutstr, "  ");
			framebuffer_setloc(49, 47);
			framebuffer_print_force(timeoutstr);
			framebuffer_setloc(0,0);
		}
        	if (isMultitouchLoaded) {
            		if(touch_watcher()) {
                		break;
            		} else {
	        	        startTime = timer_get_system_microtime();
	        	        udelay(100000);
  	        	}
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
		startScripting("linux"); //start script mode if there is a script file
		boot_linux_from_files();
#endif
	}

	return 0;
}

#endif
#endif
