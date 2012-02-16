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
#include "pmu.h"
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
