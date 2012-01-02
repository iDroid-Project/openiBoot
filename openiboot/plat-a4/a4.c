#include "openiboot.h"
#include "arm/arm.h"
#include "hardware/a4.h"
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
#include "lcd.h"
#include "tasks.h"
#include "images.h"
#include "syscfg.h"
#include "nvram.h"
#include "aes.h"
#include "accel.h"
#include "util.h"
#include "commands.h"
#include "framebuffer.h"
#include "menu.h"
#include "hfs/bdev.h"
#include "hfs/fs.h"
#include "scripting.h"
#include "actions.h"
#include "pmu.h"

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

	// Other devices
	uart_setup();
	i2c_setup();

	dma_setup();

	spi_setup();

	LeaveCriticalSection();

	aes_setup();

	if(!displaypipe_init()) {
		framebuffer_setup();
		framebuffer_setdisplaytext(TRUE);
		lcd_set_backlight_level(1500);
	}

	pmu_setup_gpio(0, 1, 1);
	pmu_setup_ldo(10, 1800, 0, 1);
}

void platform_shutdown()
{
	//dma_shutdown();
	//wdt_disable();
	arm_disable_caches();
	mmu_disable();
}
