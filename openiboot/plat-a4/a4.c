#include "openiboot.h"
#include "openiboot-asmhelpers.h"
#include "arm.h"
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
#include "accel.h"
#include "util.h"
#include "commands.h"
#include "framebuffer.h"
#include "menu.h"
#include "hfs/bdev.h"
#include "hfs/fs.h"
#include "scripting.h"
#include "actions.h"

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

	LeaveCriticalSection();

	displaypipe_init();
	framebuffer_setup();
	framebuffer_setdisplaytext(TRUE);
}

void platform_shutdown()
{
	//dma_shutdown();
	//wdt_disable();
	arm_disable_caches();
	mmu_disable();
}
