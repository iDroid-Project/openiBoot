#include "openiboot.h"
#include "arm/arm.h"
#include "gpio.h"
#include "interrupt.h"
#include "hardware/gpio.h"
#include "clock.h"
#include "util.h"
#include "timer.h"
#include "spi.h"
#include "chipid.h"
#include "hardware/chipid.h"
#include "hardware/power.h"
#include "tasks.h"

static GPIORegisters* GPIORegs;

#define GPIO_INTTYPE_MASK 0x1
#define GPIO_INTTYPE_EDGE 0x0
#define GPIO_INTTYPE_LEVEL GPIO_INTTYPE_MASK

#define GPIO_INTLEVEL_MASK 0x2
#define GPIO_INTLEVEL_LOW 0x0
#define GPIO_INTLEVEL_HIGH GPIO_INTLEVEL_MASK

#define GPIO_AUTOFLIP_MASK 0x4
#define GPIO_AUTOFLIP_NO 0x0
#define GPIO_AUTOFLIP_YES GPIO_AUTOFLIP_MASK

typedef struct {
	uint32_t flags[32];
	uint32_t token[32];
	InterruptServiceRoutine handler[32];
} GPIOInterruptGroup;

GPIOInterruptGroup InterruptGroups[GPIO_NUMINTGROUPS];

const uint16_t gpio_reset_table[] = {
	0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 
	0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 
	0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E0D, 0x1E01, 0x3002, 
	0x1002, 0x1202, 0x3002, 0x3002, 0x5002, 0x5002, 0x3002, 0x3002, 0x1E00, 0x1E00, 
	0x1202, 0x1202, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E01, 
	0x1E00, 0x9E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 
	0x1E00, 0x1E00, 0x3002, 0x3002, 0x1E00, 0x1E00, 0x3002, 0x3002, 0x3002, 0x3002, 
	0x3002, 0x3002, 0x1302, 0x3002, 0x3002, 0x3002, 0x1E00, 0x1302, 0x3002, 0x3002, 
	0x1E00, 0x1E00, 0x1202, 0x1202, 0x1002, 0x1E00, 0x1E00, 0x1E00, 0x1202, 0x1E00, 
	0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1202, 0x1202, 0x1202, 0x1202, 
	0x1202, 0x1202, 0x1202, 0x1202, 0x1202, 0x1202, 0x1202, 0x1202, 0x1E00, 0x1302, 
	0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1202, 0x1202, 0x1202, 
	0x1202, 0x1E00, 0x1302, 0x1302, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 
	0x1202, 0x1202, 0x1202, 0x1202, 0x1202, 0x1202, 0x1202, 0x1202, 0x3006, 0xB006, 
	0xB006, 0xB006, 0xB006, 0xB006, 0x1002, 0x3002, 0x3002, 0x3003, 0x1302, 0x3002, 
	0x3002, 0x3002, 0x1202, 0x3002, 0x3002, 0x3002, 0x1302, 0x9002, 0x9002, 0x1002, 
	0x1202, 0x1202, 0x9002, 0x9002, 0x1202, 0x1202, 0x1202, 0x1202, 0x1003, 0x1202, 
	0x1002, 0x1E00, 0x1202, 0x3002, 0x1302, 0x3002, 0x9002, 0x9002, 0x1202, 0x1E00, 
	0x1003, 0x1E00, 0x1002, 0x1002, 0x1E00, 0x3002, 0x3002, 0x1002, 0x3002, 0x3002, 
	0x3002, 0x3002, 0x1002, 0x3002, 0x3002, 0x3002, 0x3002, 0x1002, 0x3002, 0x1E00, 
	0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 
	0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 
	0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1E04, 
	0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1E04, 
	0x1006, 0x1006, 0x1006, 0x1006, 0x1E04, 0x1E04, 0x1E04, 0x1E04, 0x1E04, 0x1E04, 
	0x1E04, 0x1E04, 0x1E04, 0x1E04, 0x1E04, 0x1E04, 0x1E04, 0x1E04, 0x1E04, 0x1E04, 
	0x1006, 0x1E04, 0x1006, 0x1E04, 0x1E04, 0x1E04, 0x1E04, 0x1E04, 0x1E04, 0x1E04, 
	0x1E04, 0x1E04, 0x1E04, 0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1E04, 0x1006, 
	0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1006, 0x1E04, 0x1006, 
	0x1006, 0x1006, 0x1006, 0x1202, 0x1202, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 
	0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 
	0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 
	0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 
	0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 
	0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1002, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 
	0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1E00, 0x1002, 
	0x1002, 0x1002, 0x1002, 0x1002, 0x1E00, 0x1E00, 
};

int gpio_setup() {
	uint8_t v[9];
	if (!(GET_REG(POWERID) & 1)) {
		gpio_pin_use_as_input(0x107);
		gpio_pin_use_as_input(0x106);
		gpio_pin_use_as_input(0x105);
		gpio_pin_use_as_input(0x104);
		gpio_pin_use_as_input(0x103);
		gpio_pin_use_as_input(0x102);
		gpio_pin_use_as_input(0x101);
		gpio_pin_use_as_input(0x100);
		udelay(50);

		v[0] = CHIPID_GET_GPIO(chipid_get_gpio_epoch());
		v[1] = gpio_pin_state(0x104);
		v[2] = gpio_pin_state(0x105);
		v[3] = gpio_pin_state(0x106);
		v[4] = gpio_pin_state(0x107);
		v[5] = gpio_pin_state(0x100);
		v[6] = gpio_pin_state(0x101);
		v[7] = gpio_pin_state(0x102);
		v[8] = gpio_pin_state(0x103);

		gpio_pin_reset(0x107);
		gpio_pin_reset(0x106);
		gpio_pin_reset(0x105);
		gpio_pin_reset(0x104);
		gpio_pin_reset(0x103);
		gpio_pin_reset(0x102);
		gpio_pin_reset(0x101);
		gpio_pin_reset(0x100);

		uint32_t new_status = ((v[0] << 3 | v[1] << 2 | v[2] << 1 | v[3]) << 16) | ((v[4] << 3 | v[5] << 2 | v[6] << 1 | v[7]) << 8) | 1;
		SET_REG(POWERID, (GET_REG(POWERID) & 0xFF000000) | (new_status & 0xFFFFFF));
	}

	return 0;
}

void gpio_register_interrupt(uint32_t interrupt, int type, int level, int autoflip, InterruptServiceRoutine handler, uint32_t token)
{
	int group = interrupt >> 5;
	int index = interrupt & 0x1f;
	int mask = ~(1 << index);

	EnterCriticalSection();

	InterruptGroups[group].flags[index] = (type ? GPIO_INTTYPE_LEVEL : 0) | (level ? GPIO_INTLEVEL_HIGH : 0) | (level ? GPIO_AUTOFLIP_YES : 0);
	InterruptGroups[group].handler[index] = handler;
	InterruptGroups[group].token[index] = token;

	// set whether or not the interrupt is level or edge
	SET_REG(GPIOIC + GPIO_INTTYPE + (0x4 * group),
			(GET_REG(GPIOIC + GPIO_INTTYPE + (0x4 * group)) & mask) | ((type ? 1 : 0) << index));

	// set whether to trigger the interrupt high or low
	SET_REG(GPIOIC + GPIO_INTLEVEL + (0x4 * group),
			(GET_REG(GPIOIC + GPIO_INTLEVEL + (0x4 * group)) & mask) | ((level ? 1 : 0) << index));


	LeaveCriticalSection();
}

void gpio_interrupt_enable(uint32_t interrupt)
{
	int group = interrupt >> 5;
	int index = interrupt & 0x1f;

	EnterCriticalSection();
	SET_REG(GPIOIC + GPIO_INTSTAT + (0x4 * group), 1 << index);
	SET_REG(GPIOIC + GPIO_INTEN + (0x4 * group), GET_REG(GPIOIC + GPIO_INTEN + (0x4 * group)) | (1 << index));
	LeaveCriticalSection();
}

void gpio_interrupt_disable(uint32_t interrupt)
{
	int group = interrupt >> 5;
	int index = interrupt & 0x1f;
	int mask = ~(1 << index);

	EnterCriticalSection();
	SET_REG(GPIOIC + GPIO_INTEN + (0x4 * group), GET_REG(GPIOIC + GPIO_INTEN + (0x4 * group)) & mask);
	LeaveCriticalSection();
}

int gpio_pin_state(int port) {
	uint8_t pin = port & 0x7;
	port = port >> 8;

	if (port == 0x2e) {
		//return spi_status(pin);
		return 0;
	} else {
		return (GET_REG(GPIO + (8 * port + pin) * sizeof(uint32_t)) & 1);
	}
}

void gpio_pin_use_as_input(int port)
{
	gpio_custom_io(port, 0);
}

void gpio_pin_reset(int port)
{
	gpio_custom_io(port, 4); 
}

void gpio_pin_output(int port, int bit) {
	gpio_custom_io(port, (bit&1)+2); // 2 = OFF/ 3 = ON
}

void gpio_pulldown_configure(int port, GPIOPDSetting setting)
{
	uint32_t bit = 1 << GET_BITS(port, 0, 3);

	switch(setting)
	{
		case GPIOPDDisabled:
			GPIORegs[GET_BITS(port, 8, 5)].PUD1 &= ~bit;
			GPIORegs[GET_BITS(port, 8, 5)].PUD2 &= ~bit;
			break;

		case GPIOPDUp:
			GPIORegs[GET_BITS(port, 8, 5)].PUD1 |= bit;
			GPIORegs[GET_BITS(port, 8, 5)].PUD2 &= ~bit;
			break;

		case GPIOPDDown:
			GPIORegs[GET_BITS(port, 8, 5)].PUD1 &= ~bit;
			GPIORegs[GET_BITS(port, 8, 5)].PUD2 |= bit;
			break;
	}
}

void gpio_switch(int pinport, OnOff on_off) {
	uint32_t pin_register = GPIO + ((((pinport >> 5) & 0x7F8) + (pinport & 0x7)) << 2);

	uint32_t val = on_off;
	if(val < 0)
		val = 0x100;
	else if(val != 0)
		val = 0x80;

	val &= 0x180;

	SET_REG(pin_register, ((GET_REG(pin_register) & 0x180) | val));
}

void gpio_custom_io(int pinport, int mode) {
	uint8_t pin = pinport & 0x7;
	uint8_t port = (pinport >> 8) & 0xFF;
	uint16_t bitmask;
	uint16_t value;
	uint32_t pin_register;

	if (port == 0x2E)
	{
		//spi_on_off(pin, mode);
	}
	else
	{
		pin_register = GPIO + ((8 * port) + pin) * sizeof(uint32_t);
		switch(mode)
		{
		case 0: // use_as_input
			value = 0x210;
			bitmask = 0x27E;
			break;

		case 1: // use_as_output
			value = 0x212;
			bitmask = 0x27E;
			break;

		case 2: // clear_output
			value = 0x212;
			bitmask = 0x27F;
			break;

		case 3: // set_output
			value = 0x213;
			bitmask = 0x27F;
			break;

		case 4: // reset
			value = gpio_reset_table[8 * port + pin];
			bitmask = 0x3FF;
			break;

		case 5:
			value = 0x230;
			bitmask = 0x27E;
			break;

		case 6:
			value = 0x250;
			bitmask = 0x27E;
			break;

		case 7:
			value = 0x270;
			bitmask = 0x27E;
			break;

		default:
			return;
		}

		SET_REG(pin_register, (GET_REG(pin_register) & (~bitmask)) | (value & bitmask));
	}
}
