#include "openiboot.h"
#include "openiboot-asmhelpers.h"
#include "arm.h"
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
#include "usb.h"

#include "radio.h"
#include "wmcodec.h"
#include "wdt.h"
#include "als.h"
#include "multitouch.h"

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
	//usb_shutdown();
	uart_setup();
	i2c_setup();
	spi_setup();

	LeaveCriticalSection();
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
