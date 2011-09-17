#include "openiboot.h"
#include "arm/arm.h"
#include "hardware/s5l8720.h"
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
#include "hfs/bdev.h"
#include "hfs/fs.h"
#include "scripting.h"
#include "actions.h"
#include "wdt.h"

#include "buttons.h"

//TODO: remove
#include "actions.h"
#include "mtd.h"
void load_iboot() {
	framebuffer_clear();
	bufferPrintf("Loading iBoot...\r\n");
	Image* image = images_get(fourcc("ibox"));
	if (image == NULL) {
		bufferPrintf("using ibot\r\n");
		image = images_get(fourcc("ibot"));
		if (image == NULL) {
			bufferPrintf("no usable iboot image\r\n");
		}
	} else {
		bufferPrintf("using ibox\r\n");
	}
	void* imageData;
	images_read(image, &imageData);
	chainload((uint32_t)imageData);
}

//TODO: remove
static TaskDescriptor iboot_loader_task;
void iboot_loader_run(void) {
	uint64_t startTime = timer_get_system_microtime();
	// boot iboot when either the up button is pressed or after 10 seconds
	static Boolean buttonPressed = FALSE;
	static Boolean messageShown = FALSE;
	static const int countTime = 10;
	static int count = 0;
	while(1) {
		if (!gpio_pin_state(BUTTONS_VOLUP) || (count == countTime && !buttonPressed)) {
			load_iboot();
			task_stop();
		}
		if (gpio_pin_state(BUTTONS_HOLD) && buttonPressed == FALSE) {
			buttonPressed = TRUE;
			bufferPrintf("Automatic booting cancelled\r\n");
		}
		if (has_elapsed(startTime, 2 * 1000 * 1000) && !messageShown) {
			// show a welcome message after 2 seconds to skip all of the usb spam
			bufferPrintf("===================\r\n");
			bufferPrintf("Welcome to the 2g touch experimental openiBoot!\r\n");
			bufferPrintf("iBoot will be automatically loaded after 10 seconds\r\n");
			bufferPrintf("Press the power button to cancel automatic booting\r\n");
			bufferPrintf("Press the volume up button to load ios\r\n");
			bufferPrintf("===================\r\n");
			bufferPrintf("\r\n\r\n\r\n");
			messageShown = TRUE;
		}
		if (has_elapsed(startTime, count * 1000 * 1000) && !buttonPressed) {
			count++;
			if (count >= 2) {
				bufferPrintf("%d seconds until automatic boot\r\n", countTime - count);
			}
		}
		task_yield();
	}
}

void platform_init()
{
	arm_setup();
	mmu_setup();
	tasks_setup();

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
	uart_setup();
	i2c_setup();

	dma_setup();

	spi_setup();

	LeaveCriticalSection();

	aes_setup();

	displaypipe_init();
	framebuffer_setup();
	framebuffer_setdisplaytext(TRUE);
	lcd_set_backlight_level(186);

//	audiohw_init();
	
	//TODO: remove
	task_init(&iboot_loader_task, "iboot loader", TASK_DEFAULT_STACK_SIZE);
	task_start(&iboot_loader_task, &iboot_loader_run, NULL);
}

void platform_shutdown()
{
	dma_shutdown();
	wdt_disable();
	arm_disable_caches();
	mmu_disable();
}
