#include "openiboot.h"
#include "arm/arm.h"
#include "commands.h"
#include "clock.h"
#include "timer.h"
#include "hardware/clock.h"
#include "util.h"

uint32_t PLLFrequencies[] = PLL_FREQUENCIES;
static uint32_t clock_pll_registers[] = CLOCK_REFERENCE;
static uint32_t clock_frequencies[NUM_PERIPHERAL_CLOCK];
uint32_t TicksPerSec;

static uint32_t clock_init_table[] = {
	0x300, 0x701, 0x300, 0x300,
	0x300, 0x300, 0x701, 0x300,
	0x300, 0x300, 0x300, 0x300,
	0x300, 0x300, 0x300, 0x70101,
	0x300, 0x300, 0x300, 0x300,
	0x300, 0x300, 0x300, 0x300,
	0x300,
};

static uint32_t clock_setup_table[] = {
	0x004, 0x202, 0x202, 0x102,
	0x202, 0x202, 0x103, 0x201,
	0x001, 0x202, 0xA0330, 0x300,
	0x001, 0x300, 0x104, 0x301,
	0x318, 0x318, 0x318, 0x301,
	0x104, 0x300, 0x10F, 0x103,
	0x101,
};

uint32_t clock_get_frequency(FrequencyBase freqBase) {
	switch(freqBase) {
		case FrequencyBaseClock:
			return clock_frequencies[CLOCK_CPU];
		case FrequencyBaseMemory:
			return clock_frequencies[CLOCK_MEMORY];
		case FrequencyBaseBus:
			return clock_frequencies[CLOCK_BUS];
		case FrequencyBasePeripheral:
			return clock_frequencies[CLOCK_PERIPHERAL];
		case FrequencyBaseFixed:
			return clock_frequencies[CLOCK_FIXED];
		case FrequencyBaseTimebase:
			return clock_frequencies[CLOCK_TIMEBASE];
		case FrequencyBaseUsbPhy:
			return clock_frequencies[CLOCK_USB_PHY];
		case FrequencyNand:
			return clock_frequencies[CLOCK_NAND];
		default:
			return 0;
	}
}

void clock_gate_switch(uint32_t gate, OnOff on_off)
{
	if (gate > NUM_CLOCK_GATES)
		return;

	uint32_t reg = CLOCK_GATE_BASE + (sizeof(uint32_t) * gate);

	if (on_off == ON)
		SET_REG(reg, GET_REG(reg) | 0xF);
	else
		SET_REG(reg, GET_REG(reg) & ~0xF);
}

void clock_gate_reset(uint32_t gate)
{
	uint32_t pin = gate-35;
	if(pin >= 11)
		return;

	if((1 << pin) & 0x819)
	{
		uint32_t reg = CLOCK_GATE_BASE + (sizeof(uint32_t) * gate);
		SET_REG(reg, GET_REG(reg) | 0x80000000);
		udelay(1);
		SET_REG(reg, GET_REG(reg) &~ 0x80000000);
	}
}

void clock_gate_many(uint64_t _gates, OnOff _val)
{
	uint32_t reg = CLOCK_GATE_BASE;
	int num;
	for(num = 0; num < NUM_CLOCK_GATES; num++)
	{
		if(_gates & 1)
		{
			if(_val == ON)
				SET_REG(reg, GET_REG(reg) | 0xF);
			else
				SET_REG(reg, GET_REG(reg) &~ 0xF);
		}

		reg += sizeof(uint32_t);
		_gates >>= 1;
	}
}

uint32_t clock_get_pll_freq(uint32_t _pll)
{
	uint32_t val = GET_REG(clock_pll_registers[_pll]);
	uint32_t divFactor = (val >> 1) & 0x3;
	uint32_t div = (val >> 20) & 0x1F;
	uint32_t mul = (val >> 8) & 0xFf;
	return (PLLFrequencies[3] * mul) / (div << divFactor);
}

void clock_setup_pll(int _pll, int _div, int _mul, int _divFactor)
{
	uint32_t reg = clock_pll_registers[_pll];
	uint32_t val = (_div << 20) | (_mul << 8) | (_divFactor << 1) | 0x40000001;
	uint32_t freq = (PLLFrequencies[3] * _mul) / (_div << _divFactor);
	PLLFrequencies[_pll] = freq;
	bufferPrintf("clock: PLL%d, 0x%x = %d.\n", _pll, val, freq);

	SET_REG(reg, val);
	while((GET_REG(reg) & 0x20000) == 0); // Wait for CPU to accept new reference freq.
}

void clock_init()
{
	uint64_t initPwr = 0x2103001000001LL;
	clock_gate_many(initPwr, ON);
	clock_gate_many(~initPwr, OFF);

	int idx;
	uint32_t reg = DYNAMIC_CLOCK_BASE;
	for(idx = 0; idx < NUM_DYNAMIC_CLOCKS; idx++)
	{
		uint32_t val = clock_init_table[idx];
		if(idx == 15)
		{
			if(val & 0xFF)
				val |= 0x40000;
			val |= 0x80000;
		}
		else
		{
			if(val & 0xFF)
				val |= 0x400;
			val |= 0x800;
		}

		SET_REG(reg, val);
		reg += 4;
	}

	// Disable reference clocks
	SET_REG(CLOCK_REFERENCE0, GET_REG(CLOCK_REFERENCE0) &~ 1);
	SET_REG(CLOCK_REFERENCE1, GET_REG(CLOCK_REFERENCE1) &~ 1);
	SET_REG(CLOCK_REFERENCE2, GET_REG(CLOCK_REFERENCE2) &~ 1);
}

error_t clock_setup()
{
	clock_init();

	clock_setup_pll(0, 6, 150, 0);
	clock_setup_pll(1, 6, 81, 1);
	clock_setup_pll(2, 6, 100, 1);

	uint32_t reg = DYNAMIC_CLOCK_BASE;
	int idx;
	for(idx = 0; idx < NUM_DYNAMIC_CLOCKS; idx++)
	{
		uint32_t cfg = clock_setup_table[idx];
		uint32_t ocfg, selector, div;

		if(idx == 10)
		{
			selector = (cfg >> 8) & 3;
			div = (cfg & 0xFF) * ((cfg >> 16) & 0xFF);
			if(div != 0)
				ocfg = cfg | 0xc00;
			else
				ocfg = cfg | 0x800;
		}
		else if(idx == 15)
		{
			div = cfg & 0xff;
			selector = (cfg >> 16) & 0x3;
			ocfg = ((cfg & 0xFFFF) | 0xc0000);
		}
		else
		{
			div = cfg & 0xff;
			selector = (cfg >> 8) & 0x3;

			if(div != 0)
				ocfg = cfg | 0x400;
			else
				ocfg = cfg | 0x800;
		}

		if(div > 0)
			clock_frequencies[idx] = PLLFrequencies[selector]/div;
		else
			clock_frequencies[idx] = 0;

		SET_REG(reg, ocfg);
		reg += 4;
	}

	clock_frequencies[25] = 0xAAAAAAAB * clock_frequencies[15];
	clock_frequencies[26] = PLLFrequencies[3];
	clock_frequencies[27] = PLLFrequencies[3];
	TicksPerSec = PLLFrequencies[3];

	return SUCCESS;
}

static int cmd_frequencies(int argc, char **argv)
{
	if(argc > 1)
	{
		bufferPrintf("Usage: %s\n", argv[0]);
		return -1;
	}

	int i;
	for(i = 0; i < NUM_PERIPHERAL_CLOCK; i++)
	{
		bufferPrintf("clock: Clock %d, frequency=%d.\n", i, clock_frequencies[i]);
	}

	bufferPrintf("clock: CPU clocked at %d.\n", clock_frequencies[CLOCK_CPU]);
	bufferPrintf("clock: Bus clocked at %d.\n", clock_frequencies[CLOCK_BUS]);
	bufferPrintf("clock: Memory clocked at %d.\n", clock_frequencies[CLOCK_MEMORY]);

	return 0;
}
COMMAND("frequencies", "Display the clock frequencies of the device.", cmd_frequencies);
