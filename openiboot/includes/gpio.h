/**
 *  @file 
 *
 *  This file defines the GPIO Interface
 *
 *  The GPIO (General Purpose Input Output) interface is designed to represent 
 *  the setup, direction, enabling/disabling, reading and writing of GPIO pins.
 *
 *  @defgroup GPIO
 */

/**
 * gpio.h
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

#ifndef GPIO_H
#define GPIO_H

#include "openiboot.h"
#include "interrupt.h"

typedef enum
{
	GPIOPDDisabled,
	GPIOPDUp,
	GPIOPDDown
} GPIOPDSetting;

typedef struct GPIORegisters {
	volatile uint32_t CON;
	volatile uint32_t DAT;
	volatile uint32_t PUD1;	// (PUD1[x], PUD2[x]): (0, 0) = pull up/down for x disabled, (0, 1) = pull down, (1, 0) = pull up
	volatile uint32_t PUD2;
	volatile uint32_t CONSLP1;
	volatile uint32_t CONSLP2;
	volatile uint32_t PUDSLP1;
	volatile uint32_t PUDSLP2;
} GPIORegisters;

/**
 *  Setup the GPIO interface and register
 *
 *  Returns 0 on success, OiB won't work if this fails.
 *
 *  @ingroup GPIO
 */
int gpio_setup();

/**
 *  Returns the state value of a GPIO Pin.
 *
 *  @param port The GPIO pin of which to return the value for
 *
 *  @ingroup GPIO
 */
int gpio_pin_state(int port);

/**
 *  Modify a GPIO Port
 *  
 *  0 = Use as input\n
 *  1 = Use as output\n
 *  2 = Clear putput\n
 *  3 = Set output\n
 *  4 = Reset\n
 *  5-7 = ?
 * 
 *  @param port The pin to modify
 *
 *  @param mode The mode of which to change the GPIO port to
 *
 *  @ingroup GPIO
 */
void gpio_custom_io(int port, int mode);
/**
 *  void gpio_custom_io(int port, int bits);
 *  Propose using mode as opposed to bits for clarifcation
 */

/**
 *  Use a GPIO Pin as an input
 *
 *  @param port The GPIO pin to use as input
 *
 *  @ingroup GPIO
 */
void gpio_pin_use_as_input(int port);

void gpio_pin_output(int port, int bit);

/**
 *  Reset a GPIO pin
 *
 *  @param port The GPIO pin to reset
 *
 *  @ingroup GPIO
 */
void gpio_pin_reset(int port);

void gpio_register_interrupt(uint32_t interrupt, int type, int level, int autoflip, InterruptServiceRoutine handler, uint32_t token);
void gpio_interrupt_enable(uint32_t interrupt);
void gpio_interrupt_disable(uint32_t interrupt);

void gpio_pulldown_configure(int port, GPIOPDSetting setting);

int pmu_gpio(int gpio, int is_output, int value);

void gpio_switch(int pinport, OnOff on_off);

#endif
