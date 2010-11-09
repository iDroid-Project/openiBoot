#include "openiboot.h"
#include "openiboot-asmhelpers.h"

#include "arm.h"
#include "uart.h"
#include "usb.h"
#include "mmu.h"
#include "clock.h"
#include "timer.h"
#include "event.h"
#include "miu.h"
#include "power.h"
#include "interrupt.h"
#include "gpio.h"
#include "dma.h"
#include "spi.h"
#include "i2c.h"
#include "nor.h"
#include "aes.h"
#include "lcd.h"
#include "tasks.h"
#include "images.h"
#include "syscfg.h"
#include "nvram.h"
#include "accel.h"
#include "sdio.h"
#include "wlan.h"
#include "camera.h"
#include "util.h"
#include "commands.h"
#include "framebuffer.h"
#include "menu.h"
#include "pmu.h"
#include "nand.h"
#include "ftl.h"
#include "hfs/bdev.h"
#include "hfs/fs.h"
#include "scripting.h"
#include "actions.h"
#include "acm.h"

#include "radio.h"
#include "wmcodec.h"
#include "wdt.h"
#include "als.h"
#include "multitouch.h"

#ifdef OPENIBOOT_INSTALLER
#include "images/installerLogoPNG.h"
#endif

int globalFtlHasBeenRestored = 0; 

int received_file_size;

static int setup_devices();
static int setup_openiboot();

#ifndef OPENIBOOT_INSTALLER
static int load_multitouch_images()
{
    #ifdef CONFIG_IPHONE
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

static void reset_tempos(const char* sDefaultOS)
{
	framebuffer_setdisplaytext(FALSE);
	nvram_setvar("opib-temp-os",sDefaultOS);
	nvram_save();
	framebuffer_setdisplaytext(TRUE);
}
#endif //OPENIBOOT_INSTALLER

extern uint8_t _binary_payload_bin_start;
extern uint8_t _binary_payload_bin_end;
extern uint8_t _binary_payload_bin_size;

void OpenIBootStart()
{
	setup_openiboot();
	pmu_charge_settings(TRUE, FALSE, FALSE);

#ifdef OPENIBOOT_INSTALLER
	framebuffer_setdisplaytext(FALSE);
	framebuffer_clear();

	{
		int w, h;
		uint32_t *bgImg = framebuffer_load_image(datainstallerLogoPNG, sizeof(datainstallerLogoPNG), &w, &h, TRUE);
		if(bgImg)
		{
			int x = (framebuffer_width() - w)/2;
			int y = (framebuffer_height() - h)/3;

			framebuffer_draw_image(bgImg, x, y, w, h);
		}
		else
		{
			framebuffer_setdisplaytext(TRUE);
			bufferPrintf("Failed to load image...\n");
		}
	}

#else
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
	if(tempOS!=defaultOS) {
				
		switch (tempOS) {
			case 0:
				framebuffer_clear();
				bufferPrintf("Loading iOS...");
				reset_tempos(sDefaultOS);
				Image* image = images_get(fourcc("ibox"));
				if(image == NULL)
					image = images_get(fourcc("ibot"));
				void* imageData;
				images_read(image, &imageData);
				chainload((uint32_t)imageData);
				break;
			
			case 1:
				framebuffer_clear();
				bufferPrintf("Loading iDroid...");
				reset_tempos(sDefaultOS);
#ifndef NO_HFS
#ifndef CONFIG_IPOD
				radio_setup();
#endif
				nand_setup();
				fs_setup();
				if(globalFtlHasBeenRestored) {
					if(ftl_sync()) {
						bufferPrintf("ftl synced successfully");
					} else {
						bufferPrintf("error syncing ftl");
					}
				}	
				pmu_set_iboot_stage(0);
				startScripting("linux"); //start script mode if there is a script file
				boot_linux_from_files();
#endif
				break;
				
			case 2:
				framebuffer_clear();
				bufferPrintf("Loading Console...");
				reset_tempos(sDefaultOS);
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
#endif //OPENIBOOT_INSTALLER

	acm_start();

#ifndef CONFIG_IPOD
	camera_setup();
	radio_setup();
#endif
	sdio_setup();
	wlan_setup();
	accel_setup();
#ifndef CONFIG_IPOD
	als_setup();
#endif

	nand_setup();
#ifndef NO_HFS
	fs_setup();
#endif

	pmu_set_iboot_stage(0);
	startScripting("openiboot"); //start script mode if there is a file
	bufferPrintf("version: %s\r\n", OPENIBOOT_VERSION_STR);
	bufferPrintf("-----------------------------------------------\r\n");
	bufferPrintf("              WELCOME TO OPENIBOOT\r\n");
	bufferPrintf("-----------------------------------------------\r\n");
	DebugPrintf("                    DEBUG MODE\r\n");

	audiohw_postinit();

	tasks_run(); // Runs forever.
}

static int setup_devices() {
	// Basic prerequisites for everything else
	miu_setup();
	power_setup();
	clock_setup();

	// Need interrupts for everything afterwards
	interrupt_setup();

	gpio_setup();

	// For scheduling/sleeping niceties
	timer_setup();
	event_setup();
	wdt_setup();

	// Other devices
	usb_shutdown();
	uart_setup();
	i2c_setup();

	dma_setup();

	spi_setup();

	return 0;
}

static int setup_openiboot() {
	arm_setup();
	mmu_setup();
	tasks_setup();
	setup_devices();

	LeaveCriticalSection();

	clock_set_sdiv(0);

	aes_setup();

	nor_setup();
	syscfg_setup();
	images_setup();
	nvram_setup();

	lcd_setup();
	framebuffer_setup();

	audiohw_init();
    isMultitouchLoaded = 0;
	return 0;
}
