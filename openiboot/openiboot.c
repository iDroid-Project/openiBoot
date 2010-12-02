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
#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	wdt_setup();
#endif

	// Other devices
	usb_shutdown();
	uart_setup();
	i2c_setup();

#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	dma_setup();

	spi_setup();
#endif

	return 0;
}

static int setup_openiboot() {
	arm_setup();
	mmu_setup();
	tasks_setup();
	setup_devices();

	LeaveCriticalSection();

#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	clock_set_sdiv(0);

	aes_setup();

	nor_setup();
	syscfg_setup();
	images_setup();
	nvram_setup();

	lcd_setup();
	framebuffer_setup();

	audiohw_init();
#endif
    isMultitouchLoaded = 0;
	return 0;
}

void OpenIBootStart()
{
	setup_openiboot();

#if defined(CONFIG_IPHONE_4)||defined(CONFIG_IPAD)
	framebuffer_hook();
#endif

	framebuffer_setdisplaytext(TRUE);

	init_boot_modules();

#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	pmu_charge_settings(TRUE, FALSE, FALSE);
#endif

	acm_start();

	init_modules();

	//startScripting("openiboot"); //start script mode if there is a file
	bufferPrintf("  ___                   _ ____              _   \r\n");
	bufferPrintf(" / _ \\ _ __   ___ _ __ (_) __ )  ___   ___ | |_ \r\n");
	bufferPrintf("| | | | '_ \\ / _ \\ '_ \\| |  _ \\ / _ \\ / _ \\| __|\r\n");
	bufferPrintf("| |_| | |_) |  __/ | | | | |_) | (_) | (_) | |_ \r\n");
	bufferPrintf(" \\___/| .__/ \\___|_| |_|_|____/ \\___/ \\___/ \\__|\r\n");
	bufferPrintf("      |_|                                       \r\n");
	bufferPrintf("\r\n");
	bufferPrintf("version: %s\r\n", OPENIBOOT_VERSION_STR);
	DebugPrintf("                    DEBUG MODE\r\n");

#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
	audiohw_postinit();
#endif

	tasks_run(); // Runs forever.
}
