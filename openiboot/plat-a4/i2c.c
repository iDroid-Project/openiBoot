#include "openiboot.h"
#include "i2c.h"
#include "hardware/i2c.h"
#include "util.h"
#include "clock.h"
#include "gpio.h"
#include "timer.h"
#include "tasks.h"
#include "interrupt.h"

static const I2CInfo I2CInit[] = {
			{0, 0, 0, 0, 0, I2CDone, 0, 0, 0, 0x904, 0x903, 0x13, I2C0 + IICREG0, I2C0 + IICREG8, I2C0 + IICREGC, I2C0 + IICREG10, I2C0 + IICREG14, I2C0 + IICREG18, I2C0 + IICREG20, I2C0 + IICREG24, 0x39},
			{0, 0, 0, 0, 0, I2CDone, 0, 0, 0, 0x905, 0x906, 0x14, I2C1 + IICREG0, I2C1 + IICREG8, I2C1 + IICREGC, I2C1 + IICREG10, I2C1 + IICREG14, I2C1 + IICREG18, I2C1 + IICREG20, I2C1 + IICREG24, 0x3A},
			{0, 0, 0, 0, 0, I2CDone, 0, 0, 0, 0xA00, 0x907, 0x15, I2C2 + IICREG0, I2C2 + IICREG8, I2C2 + IICREGC, I2C2 + IICREG10, I2C2 + IICREG14, I2C2 + IICREG18, I2C2 + IICREG20, I2C2 + IICREG24, 0x3B},
		};
static I2CInfo I2C[3];
static void init_i2c(I2CInfo* i2c);
static void i2cIRQHandler(uint32_t token);

static I2CError i2c_readwrite(I2CInfo* i2c);

int i2c_setup() {
	memcpy(I2C, I2CInit, sizeof(I2CInfo) * (sizeof(I2C) / sizeof(I2CInfo)));

	init_i2c(&I2C[0]);
	init_i2c(&I2C[1]);
	return 0;
}

static void init_i2c(I2CInfo* i2c) {
	clock_gate_switch(i2c->clockgate, ON);

	gpio_set(i2c->iic_sda_gpio, 2); // pull sda low?

	int i;
	for (i = 0; i < 19; i++) {
		gpio_set(i2c->iic_scl_gpio, (i % 2) ? 2 : 0);
		udelay(5);
	}

	gpio_set(i2c->iic_scl_gpio, 5); // generate stop condition?
	gpio_set(i2c->iic_sda_gpio, 5);

	SET_REG(i2c->register_8, 0x30);
	SET_REG(i2c->register_C, 0x37);

	i2c->operation_result = 0;

	interrupt_install(i2c->interrupt, i2cIRQHandler, (uint32_t)i2c);
	interrupt_enable(i2c->interrupt);
}

static void i2cIRQHandler(uint32_t token) {
	uint32_t result;
	I2CInfo* i2c = (I2CInfo*) token;

	udelay(1);
	result = GET_REG(i2c->register_C);
	if (!i2c->is_write) {
		uint32_t count;
		count = result & 0x20;
		if (!(result & 0x20)) {
			while (count < i2c->bufferLen) {
				SET_REG(i2c->buffer + count++, GET_REG(i2c->register_20));
			}
		}
	}
	SET_REG(i2c->register_C, result);
	i2c->operation_result = result;
}

I2CError i2c_tx(int bus, int iicaddr, void* buffer, int len) {
	I2C[bus].address = iicaddr;
	I2C[bus].is_write = TRUE;
	I2C[bus].registers = ((uint8_t*) buffer);
	I2C[bus].buffer = ((uint8_t*) (buffer))+1;
	I2C[bus].bufferLen = len-1;
	return i2c_readwrite(&I2C[bus]);
}

I2CError i2c_rx(int bus, int iicaddr, const uint8_t* registers, int num_regs, void* buffer, int len) {
	if (num_regs != 1)
		return 1;

	I2C[bus].address = iicaddr;
	I2C[bus].is_write = FALSE;
	I2C[bus].registers = registers;
	I2C[bus].bufferLen = len;
	I2C[bus].buffer = (uint8_t*) buffer;
	return i2c_readwrite(&I2C[bus]);
}


static I2CError i2c_readwrite(I2CInfo* i2c) { 
	if (i2c->bufferLen > 0x80)
		return 1;

	while (i2c->state != I2CDone) {
		task_yield();
	}

	i2c->state = I2CStart;

	SET_REG(i2c->register_0, i2c->address >> 1);
	SET_REG(i2c->register_14, 0);
	SET_REG(i2c->register_10, *i2c->registers);
	SET_REG(i2c->register_18, i2c->bufferLen);

	if (i2c->is_write) {
		int i = 0;
		while (i < i2c->bufferLen) {
			SET_REG(i2c->register_20, *(i2c->buffer + i++));
		}
	}

	SET_REG(i2c->register_24, i2c->is_write | 4);

	while (!i2c->operation_result) {
		task_yield();
	}

	i2c->operation_result = 0;
	i2c->state = I2CDone;

	if(i2c->operation_result & 0x20)
		return 1;

	return I2CNoError;
}

