#include "openiboot.h"
#include "openiboot-asmhelpers.h"
#include "gpio.h"
#include "interrupt.h"
#include "hardware/gpio.h"
#include "clock.h"
#include "util.h"
#include "timer.h"
#include "spi.h"
#include "chipid.h"
#include "hardware/power.h"

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

void gpio_switch(OnOff on_off, uint32_t pinport);
void gpio_set(uint32_t pinport, int mode);

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

int gpio_setup() {
/*	When resetting the GPIO Interrupts we'll also fuck the framebuffer hook.
	Once LCD is ported we can enable this again.

	// Reset everything
	int i;
	for (i = 0; i < 0xB0; i++) {
		SET_REG(GPIO + i * sizeof(uint32_t), gpio_reset_table[i]);
	}
*/

	// Initialise it
	uint8_t v[8];
	if (!(GET_REG(POWER + POWER_ID) & 1)) {
		gpio_set(0x502, 0);
		gpio_set(0x503, 0);
		gpio_set(0x504, 0);
		gpio_switch(0x502, ON);
		gpio_switch(0x503, ON);
		gpio_switch(0x504, ON);
		gpio_set(0x202, 0);
		gpio_set(0x301, 0);
		gpio_set(0x304, 0);
		gpio_set(0x305, 0);
		gpio_switch(0x202, ON);
		gpio_switch(0x301, ON);
		gpio_switch(0x304, ON);
		gpio_switch(0x305, ON);
		udelay(100);
		v[0] = chipid_get_gpio();
		v[1] = gpio_pin_state(0x504);
		v[2] = gpio_pin_state(0x503);
		v[3] = gpio_pin_state(0x502);
		v[4] = gpio_pin_state(0x305);
		v[5] = gpio_pin_state(0x304);
		v[6] = gpio_pin_state(0x301);
		v[7] = gpio_pin_state(0x202);
		gpio_set(0x502, 4);
		gpio_set(0x503, 4);
		gpio_set(0x504, 4);
		gpio_set(0x202, 4);
		gpio_set(0x301, 4);
		gpio_set(0x304, 4);
		gpio_set(0x305, 4);
		uint32_t new_status = ((v[0] << 3 | v[1] << 2 | v[2] << 1 | v[3]) << 16) | ((v[4] << 3 | v[5] << 2 | v[6] << 1 | v[7]) << 8) | 1;
		SET_REG(POWER + POWER_ID, (GET_REG(POWER + POWER_ID) & 0xFF000000) | (new_status & 0xFFFFFF));
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

	if (port == 0x16) {
		return spi_status(pin);
	} else {
		return (GET_REG(GPIO + (8 * port + pin) * sizeof(uint32_t)) & 1);
	}
}

void gpio_custom_io(int port, int bits) {
	SET_REG(GPIO + GPIO_FSEL, ((GET_BITS(port, 8, 5) & GPIO_FSEL_MAJMASK) << GPIO_FSEL_MAJSHIFT)
				| ((GET_BITS(port, 0, 3) & GPIO_FSEL_MINMASK) << GPIO_FSEL_MINSHIFT)
				| ((bits & GPIO_FSEL_UMASK) << GPIO_FSEL_USHIFT));
}

void gpio_pin_use_as_input(int port) {
	gpio_custom_io(port, 0);
}

void gpio_pin_output(int port, int bit) {
	gpio_custom_io(port, 0xE | bit); // 0b111U, where U is the argument
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

void gpio_switch(OnOff on_off, uint32_t pinport) {
	uint32_t pin_register = GPIO + (((pinport >> 5) & 0x7F8) + ((pinport & 0x7)<<2));

	if (on_off == ON) {
		SET_REG(pin_register, ((GET_REG(pin_register) & (~(0x3<<7))) | (0x1<<7)));
	} else {
		SET_REG(pin_register, (GET_REG(pin_register) & (~(0x3<<7))));
	}
	// There would be a third state when (state > 0), setting 0x3, but iBoot didn't ever do that.
}

void gpio_set(uint32_t pinport, int mode) {
	uint8_t pin = pinport & 0x7;
	uint8_t port = (pinport >> 8) & 0xFF;
	uint16_t bitmask;
	uint16_t value;
	uint32_t pin_register;
	if (port == 0x16) {
		spi_on_off(pin, mode);
	} else {
		pin_register = GPIO + (8 * port + pin) * sizeof(uint32_t);
		switch(mode) {
			default:
				return;
		case 0:
			value = 0x210;
			bitmask = 0x27E;
			break;
		case 1:
			value = 0x212;
			bitmask = 0x27E;
			break;
		case 2: // set pin state
			value = 0x212;
			bitmask = 0x27F;
			break;
		case 3: // set pin state
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
		}
	SET_REG(pin_register, (GET_REG(pin_register) & (~bitmask)) | (value & bitmask));
	}
}
