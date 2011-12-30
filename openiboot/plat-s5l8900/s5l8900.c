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
#include "wmcodec.h"

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

	clock_set_sdiv(0);

	aes_setup();

	lcd_setup();
	framebuffer_setup();

	audiohw_init();

	framebuffer_setdisplaytext(TRUE);
}

void platform_shutdown()
{
	dma_shutdown();
	wdt_disable();
	arm_disable_caches();
	mmu_disable();
}
