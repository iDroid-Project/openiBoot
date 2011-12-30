#include "openiboot.h"
#include "commands.h"
#include "arm/arm.h"
#include "gpio.h"
#include "interrupt.h"
#include "hardware/gpio.h"
#include "clock.h"
#include "util.h"

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

static void gpio_handle_interrupt(uint32_t token);

int gpio_setup() {
	// iboot writes to these registers on the S5L8720
	// this probably is some weird replacement for a clock gate
	SET_REG(GPIOIC + 0x70, 0x32);
	SET_REG(GPIOIC + 0x6C, 3);
	while (!(GET_REG(GPIOIC + 0x7C) & 0x1));
	SET_REG(GPIOIC + 0x6C, 2);

	int i;

	GPIORegs = (GPIORegisters*) GPIO;

	for(i = 0; i < GPIO_NUMINTGROUPS; i++) {
		// writes to all the interrupt status register to acknowledge and discard any pending
		SET_REG(GPIOIC + GPIO_INTSTAT + (i * 0x4), GPIO_INTSTAT_RESET);

		// disable all interrupts
		SET_REG(GPIOIC + GPIO_INTEN + (i * 0x4), GPIO_INTEN_RESET);
	}

	memset(InterruptGroups, 0, sizeof(InterruptGroups));

	interrupt_install(0x21, gpio_handle_interrupt, 0);
	interrupt_install(0x20, gpio_handle_interrupt, 1);
	interrupt_install(0x1f, gpio_handle_interrupt, 2);
	interrupt_install(0x03, gpio_handle_interrupt, 3);
	interrupt_install(0x02, gpio_handle_interrupt, 4);
	interrupt_install(0x01, gpio_handle_interrupt, 5);
	interrupt_install(0x00, gpio_handle_interrupt, 6);

	interrupt_enable(0x21);
	interrupt_enable(0x20);
	interrupt_enable(0x1f);
	interrupt_enable(0x03);
	interrupt_enable(0x02);
	interrupt_enable(0x01);
	interrupt_enable(0x00);

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

static void gpio_handle_interrupt(uint32_t token)
{
	uint32_t statReg = GPIOIC + GPIO_INTSTAT + (0x4 * token);

	int stat;
	while((stat = GET_REG(statReg)) != 0)
	{
		int i;
		for(i = 0; i < 32; ++i)
		{
			if(stat & 1)
			{
				uint32_t flags = InterruptGroups[token].flags[i];

				if((flags & GPIO_INTTYPE_MASK) == GPIO_INTTYPE_EDGE)
					SET_REG(statReg, 1 << i);

				if((flags & GPIO_AUTOFLIP_MASK) == GPIO_AUTOFLIP_YES)
				{
					// flip the corresponding bit in GPIO_INTLEVEL
					SET_REG(GPIOIC + GPIO_INTLEVEL + (0x4 * token),
							GET_REG(GPIOIC + GPIO_INTLEVEL + (0x4 * token)) ^ (1 << i));

					// flip the bit in the flags too
					InterruptGroups[token].flags[i] = flags ^ GPIO_INTLEVEL_MASK;
				}

				if(InterruptGroups[token].handler[i])
				{
					InterruptGroups[token].handler[i](InterruptGroups[token].token[i]);
				}

				SET_REG(statReg, 1 << i);
			}

			stat >>= 1;
		}
	}
}

int gpio_pin_state(int port) {
	return ((GPIORegs[GET_BITS(port, 8, 5)].DAT & (1 << GET_BITS(port, 0, 3))) != 0);
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

void cmd_gpio_pinstate(int argc, char** argv) {
	if(argc < 2) {
		bufferPrintf("Usage: %s <port>\r\n", argv[0]);
		return;
	}

	uint32_t port = parseNumber(argv[1]);
	bufferPrintf("Pin 0x%x state: 0x%x\r\n", port, gpio_pin_state(port));
}
COMMAND("gpio_pinstate", "get the state of a GPIO pin", cmd_gpio_pinstate);

void cmd_gpio_out(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <port> [0|1]\r\n", argv[0]);
		return;
	}

	uint32_t port = parseNumber(argv[1]);
	uint32_t value = parseNumber(argv[2]);
	bufferPrintf("Pin 0x%x value: %d\r\n", port, value);
    gpio_pin_output(port,value);
}
COMMAND("gpio_out", "set the state of a GPIO pin", cmd_gpio_out);
