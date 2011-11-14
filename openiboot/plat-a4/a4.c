/**
 * a4.c
 *
 * Copyright 2011 iDroid Project
 *
 * This file is part of iDroid. An android distribution for Apple products.
 * For more information, please visit http://www.idroidproject.org/.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "aes.h"
#include "arm/arm.h"
#include "clock.h"
#include "dma.h"
#include "event.h"
#include "framebuffer.h"
#include "gpio.h"
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
}

void platform_shutdown()
{
	//dma_shutdown();
	//wdt_disable();
	arm_disable_caches();
	mmu_disable();
}
