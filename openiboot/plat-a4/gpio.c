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
#include "hardware/power.h"
#include "commands.h"

#define NUM_GPIO 276

typedef struct _gpio_interrupt
{
	uint32_t token;
	InterruptServiceRoutine func;
} gpio_interrupt_t;

gpio_interrupt_t gpio_interrupts[NUM_GPIO];

static void gpio_handle_interrupt(uint32_t token);
void gpio_custom_io(int pinport, int mode);

#if !defined(CONFIG_IPAD_1G)
const uint16_t gpio_reset_table[] = {
	0x210, 0x210, 0x390, 0x390, 0x210, 0x290, 0x213, 0x212,
	0x213, 0x212, 0x213, 0x290, 0x290, 0x390, 0x212, 0x1E,
	0x212, 0x212, 0x390, 0x212, 0x212, 0x290, 0x212, 0x390,
	0x210, 0x1E, 0x290, 0x212, 0x1E, 0x1E, 0x213, 0x212,
	0x390, 0x290, 0x212, 0x1E, 0x390, 0x390, 0x390, 0x1E,
	0x1E, 0x1E, 0x630, 0x630, 0x630, 0x213, 0x630, 0x630,
	0x630, 0x212, 0x630, 0x630, 0x630, 0x212, 0x630, 0x630,
	0x630, 0x630, 0x630, 0x630, 0x1E, 0x1E, 0x630, 0x630,
	0x613, 0x630, 0x630, 0x630, 0x630, 0x630, 0x230, 0x230,
	0x230, 0x1E, 0x1E, 0x230, 0x230, 0x250, 0x250, 0x230,
	0x230, 0xA30, 0xA30, 0xA30, 0xA30, 0xAB0, 0xAB0, 0xBB0,
	0xBB0, 0xAB0, 0xAB0, 0xAB0, 0xAB0, 0xAB0, 0xAB0, 0xAB0,
	0xAB0, 0x81E, 0x81E, 0x81E, 0x81E, 0xA30, 0xA30, 0xA30,
	0xA30, 0xAB0, 0xAB0, 0xBB0, 0xBB0, 0xAB0, 0xAB0, 0xAB0,
	0xAB0, 0xAB0, 0xAB0, 0xAB0, 0xAB0, 0x1E, 0xA30, 0x230,
	0x212, 0x230, 0x213, 0x212, 0x630, 0x630, 0x630, 0x630,
	0x630, 0x630, 0x630, 0x630, 0x630, 0x630, 0x630, 0x630,
	0x630, 0x630, 0x630, 0x1E, 0x212, 0x1E, 0x230, 0x630,
	0x630, 0xE30, 0xFB0, 0xFB0, 0xFB0, 0xFB0, 0xFB0, 0x81E,
	0x81E, 0x81E, 0x81E, 0x1E, 0x1E, 0x1E, 0x210, 0x210,
	0x210, 0x212, 0x210, 0x212, 0x1E, 0x210, 0x210, 0x212,
	0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
};
#else
const uint16_t gpio_reset_table[] = {
	0x210, 0x210, 0x390, 0x390, 0x210, 0x212, 0x1E, 0x612,
	0x1E, 0x612, 0x612, 0x210, 0x210, 0x390, 0x612, 0x613,
	0x1E, 0x210, 0x390, 0x612, 0x612, 0x290, 0x212, 0x613,
	0x612, 0x1E, 0x210, 0x212, 0x1E, 0x1E, 0x210, 0x1E,
	0x1E, 0x390, 0x212, 0x390, 0x390, 0x390, 0x390, 0x1E,
	0x1E, 0x1E, 0xA30, 0xA30, 0xA30, 0x613, 0xA30, 0xA30,
	0xA30, 0x612, 0xA30, 0xA30, 0xA30, 0x612, 0xA30, 0xA30,
	0xA30, 0xA30, 0xA30, 0xA30, 0x613, 0x613, 0xA30, 0xA30,
	0xA13, 0xA30, 0x213, 0x9E, 0x613, 0x41E, 0xA30, 0xA30,
	0xA30, 0xA30, 0xA30, 0xA30, 0xA30, 0xA50, 0xA50, 0xA30,
	0xA30, 0xA30, 0xA30, 0xA30, 0xA30, 0xA30, 0xA30, 0xA30,
	0xA30, 0xA30, 0xA30, 0xA30, 0xA30, 0xA30, 0xA30, 0xA30,
	0xA30, 0x212, 0x212, 0x212, 0x212, 0xA30, 0xA30, 0xA30,
	0xA30, 0xA30, 0xA30, 0xA30, 0xA30, 0xA30, 0xA30, 0xA30,
	0xA30, 0xA30, 0xA30, 0xA30, 0xA30, 0x212, 0xA30, 0x613,
	0x390, 0xA30, 0x612, 0xA12, 0x1E, 0xA30, 0xA30, 0xA30,
	0xA30, 0xA30, 0xA30, 0xA30, 0xA30, 0xA30, 0x1E, 0xA30,
	0xA30, 0xA30, 0xA30, 0x213, 0x212, 0x1E, 0x230, 0xA30,
	0xA30, 0xA30, 0xBB0, 0xBB0, 0xBB0, 0xBB0, 0xBB0, 0x212,
	0x212, 0x612, 0x212, 0x212, 0x212, 0x612, 0x612, 0x212,
	0x212, 0x612, 0x610, 0x612, 0x612, 0x210, 0x210, 0x212,
	0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E, 0x1E,
};
#endif


int gpio_setup() {
	// Initialise it
	uint8_t v[8];
	if (!(GET_REG(POWER + POWER_ID) & 1)) {
		gpio_custom_io(0x502, 0);
		gpio_custom_io(0x503, 0);
		gpio_custom_io(0x504, 0);
		gpio_pulldown_configure(0x502, GPIOPDDown);
		gpio_pulldown_configure(0x503, GPIOPDDown);
		gpio_pulldown_configure(0x504, GPIOPDDown);
		gpio_custom_io(0x202, 0);
		gpio_custom_io(0x301, 0);
		gpio_custom_io(0x304, 0);
		gpio_custom_io(0x305, 0);
		gpio_pulldown_configure(0x202, GPIOPDDown);
		gpio_pulldown_configure(0x301, GPIOPDDown);
		gpio_pulldown_configure(0x304, GPIOPDDown);
		gpio_pulldown_configure(0x305, GPIOPDDown);
		udelay(100);
		v[0] = chipid_get_gpio_epoch();
		v[1] = gpio_pin_state(0x504);
		v[2] = gpio_pin_state(0x503);
		v[3] = gpio_pin_state(0x502);
		v[4] = gpio_pin_state(0x305);
		v[5] = gpio_pin_state(0x304);
		v[6] = gpio_pin_state(0x301);
		v[7] = gpio_pin_state(0x202);
		gpio_custom_io(0x502, 4);
		gpio_custom_io(0x503, 4);
		gpio_custom_io(0x504, 4);
		gpio_custom_io(0x202, 4);
		gpio_custom_io(0x301, 4);
		gpio_custom_io(0x304, 4);
		gpio_custom_io(0x305, 4);
		uint32_t new_status = ((v[0] << 3 | v[1] << 2 | v[2] << 1 | v[3]) << 16) | ((v[4] << 3 | v[5] << 2 | v[6] << 1 | v[7]) << 8) | 1;
		SET_REG(POWER + POWER_ID, (GET_REG(POWER + POWER_ID) & 0xFF000000) | (new_status & 0xFFFFFF));
	}

	interrupt_install(GPIO_INTERRUPT, gpio_handle_interrupt, 0);
	interrupt_enable(GPIO_INTERRUPT);

	return 0;
}

static void gpio_handle_interrupt(uint32_t token)
{
	while(TRUE)
	{
		uint32_t gsts = GET_REG(GPIO+GPIOIC_INT);
		if(!gsts)
			break;

		uint32_t block = (31 - __builtin_clz(GET_REG(GPIO + GPIOIC_INT)));
		reg32_t sts_reg = GPIO + GPIOIC_INTSTS + sizeof(uint32_t)*block;
		uint32_t sts = GET_REG(sts_reg);

		if(sts)
		{
			uint32_t sub = 31 - __builtin_clz(sts);
			uint32_t bit = (1 << sub);
			uint32_t gpio = sub + (32*block);
			reg32_t reg = GPIO + (gpio << 2);

			uint32_t done;
			if((GET_REG(reg) & 0xC) == 4)
			{
				done = 1;
				SET_REG(sts_reg, bit); // Clear this interrupt
			}
			else
				done  = 0;

			if(gpio_interrupts[gpio].func)
				gpio_interrupts[gpio].func(gpio_interrupts[gpio].token);

			if(!done)
				SET_REG(sts_reg, bit);
		}
	}
}

void gpio_register_interrupt(uint32_t interrupt, int type, int level, int autoflip, InterruptServiceRoutine handler, uint32_t token)
{
	uint8_t mode;
	if(autoflip)
		mode = 0xC;
	else if(type)
	{
		if(level)
			mode = 0x4;
		else
			mode = 0x6;
	}
	else
	{
		if(level)
			mode = 0x8;
		else
			mode = 0xa;
	}

	gpio_interrupts[interrupt].token = token;
	gpio_interrupts[interrupt].func = handler;

	EnterCriticalSection();

	reg32_t reg = GPIO + sizeof(uint32_t)*interrupt;
	SET_REG(reg, (GET_REG(reg) &~ 0xE) | mode | 0x200);

	LeaveCriticalSection();
}

void gpio_interrupt_enable(uint32_t interrupt)
{
	uint32_t block = interrupt >> 5;
	uint32_t gpio = interrupt & 0x1f;
	uint32_t bit = (1 << gpio);

	reg32_t reg = GPIO + sizeof(uint32_t)*interrupt;
	if((GET_REG(reg) & 0xC) == 4)
	{
		reg32_t sts_reg = GPIO + GPIOIC_INTSTS + sizeof(uint32_t)*block;
		SET_REG(sts_reg, bit);
	}

	reg32_t en_reg = GPIO + GPIOIC_ENABLE + sizeof(uint32_t)*block;
	SET_REG(en_reg, bit);
}

void gpio_interrupt_disable(uint32_t interrupt)
{
	uint32_t block = interrupt >> 5;
	uint32_t gpio = interrupt & 0x1f;
	uint32_t bit = (1 << gpio);

	reg32_t ic_reg = GPIO + GPIOIC_DISABLE + sizeof(uint32_t)*block;
	SET_REG(ic_reg, bit);
}

static error_t cmd_test_gpioic(int _argc, char **_argv)
{
	uint32_t num = parseNumber(_argv[1]);
	uint32_t flags = parseNumber(_argv[2]);
	gpio_register_interrupt(num, flags & 1, (flags >> 1) & 1, (flags >> 2) & 1, NULL, 0);
	gpio_interrupt_enable(((num >> 8) * 8) + (num & 0x7));

	return 0;
};
COMMAND("test_gpioic", "Test GPIOIC", cmd_test_gpioic);

int gpio_pin_state(int port) {
	uint8_t pin = port & 0x7;
	port = port >> 8;

	if (port == 0x16) {
		//return spi_status(pin);
		return 0;
	} else {
		return (GET_REG(GPIO + (8 * port + pin) * sizeof(uint32_t)) & 1);
	}
}

void gpio_pin_use_as_input(int port) {
	gpio_custom_io(port, 0);
}

void gpio_pin_output(int pinport, int bit) {
	uint8_t pin = pinport & 0x7;
	uint8_t port = (pinport >> 8) & 0xFF;

	if (port == 0x16) {
//		sub_5FF0EF38(pinport, bit); // SPI related, not yet, sorry.
	} else {
		reg32_t reg = GPIO + (8 * port + pin) * sizeof(uint32_t);
		SET_REG(reg, (GET_REG(reg) &~1) | (bit & 1));
	}
}

void gpio_pulldown_configure(int port, GPIOPDSetting setting)
{
	uint32_t pin_register = GPIO + (((port >> 5) & 0x7F8) + ((port & 0x7)<<2));

	switch(setting) {
		case GPIOPDDisabled:
			SET_REG(pin_register, (GET_REG(pin_register) & (~(0x3<<7))));
			break;

		case GPIOPDUp:
			SET_REG(pin_register, ((GET_REG(pin_register) & (~(0x3<<7))) | (0x3<<7)));
			break;

		case GPIOPDDown:
			SET_REG(pin_register, ((GET_REG(pin_register) & (~(0x3<<7))) | (0x1<<7)));
			break;
	}
}

void gpio_custom_io(int pinport, int mode) {
	uint8_t pin = pinport & 0x7;
	uint8_t port = (pinport >> 8) & 0xFF;
	uint16_t bitmask;
	uint16_t value;
	uint32_t pin_register;
	if (port == 0x16) {
		//spi_on_off(pin, mode);
	} else {
		pin_register = GPIO + (8 * port + pin) * sizeof(uint32_t);
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
			value &= ~0x10;
			break;

		case 6:
			value = 0x250;
			bitmask = 0x27E;
			value &= ~0x10;
			break;

		case 7:
			value = 0x270;
			bitmask = 0x27E;
			value &= ~0x10;
			break;

		default:
			return;
		}
	SET_REG(pin_register, (GET_REG(pin_register) & (~bitmask)) | (value & bitmask));
	}
}
