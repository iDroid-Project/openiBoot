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
#include "hfs/bdev.h"
#include "hfs/fs.h"
#include "scripting.h"
#include "actions.h"
#include "wdt.h"

//TODO: remove
#include "buttons.h"
static void gpio_test_handler(uint32_t token) {
	switch(token) {
		case 0:
			bufferPrintf("Hold button changed state!\r\n");
			break;
		case 1:
			bufferPrintf("Home button changed state!\r\n");
			break;
		case 2:
			bufferPrintf("Volume up button changed state!\r\n");
			break;
		case 3:
			bufferPrintf("Volume down button changed state!\r\n");
			break;
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
//	uart_setup();
	i2c_setup();

//	dma_setup();

	spi_setup();

	LeaveCriticalSection();

	aes_setup();

	nor_setup();
	syscfg_setup();
	images_setup();
	nvram_setup();

	displaypipe_init();
	framebuffer_setup();
	framebuffer_setdisplaytext(TRUE);
//	lcd_set_backlight_level(1500);

//	audiohw_init();


	gpio_register_interrupt(BUTTONS_HOLD_IRQ, BUTTONS_HOLD_IRQTYPE, BUTTONS_HOLD_IRQLEVEL, BUTTONS_HOLD_IRQAUTOFLIP, gpio_test_handler, 0);
	gpio_interrupt_enable(BUTTONS_HOLD_IRQ);
	gpio_register_interrupt(BUTTONS_HOME_IRQ, BUTTONS_HOME_IRQTYPE, BUTTONS_HOME_IRQLEVEL, BUTTONS_HOME_IRQAUTOFLIP, gpio_test_handler, 1);
	gpio_interrupt_enable(BUTTONS_HOME_IRQ);
	gpio_register_interrupt(BUTTONS_VOLUP_IRQ, BUTTONS_VOLUP_IRQTYPE, BUTTONS_VOLUP_IRQLEVEL, BUTTONS_VOLUP_IRQAUTOFLIP, gpio_test_handler, 2);
	gpio_interrupt_enable(BUTTONS_VOLUP_IRQ);
	gpio_register_interrupt(BUTTONS_VOLDOWN_IRQ, BUTTONS_VOLDOWN_IRQTYPE, BUTTONS_VOLDOWN_IRQLEVEL, BUTTONS_VOLDOWN_IRQAUTOFLIP, gpio_test_handler, 3);
	gpio_interrupt_enable(BUTTONS_VOLDOWN_IRQ);
}

void platform_shutdown()
{
	//dma_shutdown();
	wdt_disable();
	arm_disable_caches();
	mmu_disable();
}
