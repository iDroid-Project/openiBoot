#include "openiboot.h"
#include "arm/arm.h"
#include "hardware/s5l8920.h"
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
#include "util.h"
#include "commands.h"
#include "framebuffer.h"
#include "hfs/bdev.h"
#include "hfs/fs.h"
#include "scripting.h"
#include "actions.h"
#include "usb.h"
#include "cdma.h"

void platform_init()
{
	arm_setup();
	mmu_setup();

	tasks_setup();

	// Basic prerequisites for everything else
	miu_setup();
	power_setup();

	//framebuffer_hook(); // TODO: Remove once LCD implemented -- Ricky26
	//framebuffer_setdisplaytext(TRUE);

	clock_setup();

	// Need interrupts for everything afterwards
	interrupt_setup();

	gpio_setup();

	// For scheduling/sleeping niceties
	timer_setup();
	event_setup();

	// Other devices
	dma_setup();
	//usb_shutdown();
	uart_setup();
	i2c_setup();
	spi_setup();

	LeaveCriticalSection();

	aes_setup();
}

void platform_shutdown()
{
	//dma_shutdown();
	arm_disable_caches();
	mmu_disable();
}

// NYI -- Ricky26
void pmu_set_iboot_stage(uint8_t stage)
{}
