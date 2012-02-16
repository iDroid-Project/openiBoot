#include "aes.h"
#include "arm/arm.h"
#include "clock.h"
#include "dma.h"
#include "event.h"
#include "framebuffer.h"
#include "gpio.h"
#include "interrupt.h"
#include "i2c.h"
#include "lcd.h"
#include "miu.h"
#include "mmu.h"
#include "openiboot.h"
#include "power.h"
#include "spi.h"
#include "tasks.h"
#include "timer.h"
#include "uart.h"
#include "wdt.h"
#include "audiocodec.h"

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
