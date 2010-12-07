#include "openiboot.h"
#include "clock.h"
#include "util.h"
#include "hardware/clock.h"
#include "hardware/power.h"
#include "power.h"
#include "openiboot-asmhelpers.h"

uint32_t PLLFrequencies[] = PLL_FREQUENCIES;
static uint32_t clock_ref_registers[] = CLOCK_REFERENCE;
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

void clock_gate_switch(uint32_t gate, OnOff on_off) {
	power_ctrl(gate, on_off);
}

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
		default:
			return 0;
	}
}

uint32_t clock_get_pll_freq(uint32_t val)
{
	uint32_t divFactor = (val >> 1) & 0x3;
	uint32_t div = (val >> 20) & 0x1F;
	uint32_t mul = (val >> 8) & 0xFf;
	return (PLLFrequencies[3] * mul) / (div << divFactor);
}

void clock_set_reference(int _ref, int _div, int _mul, int _divFactor)
{
	if(_ref > 2 || _ref < 0)
	{
		bufferPrintf("clock: Invalid reference base %d.\n", _ref);
		return;
	}

	uint32_t reg = clock_ref_registers[_ref];
	uint32_t val = (_div << 20) | (_mul << 8) | (_divFactor << 1) | 0x40001;
	uint32_t freq = (PLLFrequencies[3] * _mul) / (_div << _divFactor);
	PLLFrequencies[_ref] = freq;
	bufferPrintf("clock: PLL%d, 0x%x = %d.\n", _ref, val, freq);

	SET_REG(reg, val);
	while((GET_REG(reg) & 0x20000) == 0); // Wait for CPU to accept new reference freq.
}

void clock_init()
{
	// TODO: Actually do this when we can load up all the hardware ourselves! -- Ricky26
	//uint64_t initPwr = 0x2103001000001LL;
	//power_ctrl_many(initPwr, ON);
	//power_ctrl_many(~initPwr, OFF);

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

		//SET_REG(reg, val);
		reg -= 4;
	}

	// Disable reference clocks
	SET_REG(CLOCK_REFERENCE0, GET_REG(CLOCK_REFERENCE0) &~ 1);
	SET_REG(CLOCK_REFERENCE1, GET_REG(CLOCK_REFERENCE1) &~ 1);
	SET_REG(CLOCK_REFERENCE2, GET_REG(CLOCK_REFERENCE2) &~ 1);
}

int clock_setup()
{
	//clock_init();

	clock_set_reference(0, 6, 150, 0);
	clock_set_reference(1, 6, 81, 1);
	clock_set_reference(2, 6, 100, 1);

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
			ocfg = ((cfg & 0xFFFFFF00) | 0xc0000) | (div * 4);
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

		bufferPrintf("clock: Clock %d[%d], freq=%dKhz, cfg=0x%x, div=%d\n", idx, selector, clock_frequencies[idx]/1000, ocfg, div);
		reg += 4;
	}

	clock_frequencies[25] = 0xAAAAAAAB * clock_frequencies[15];
	clock_frequencies[26] = PLLFrequencies[3];
	clock_frequencies[27] = PLLFrequencies[3];

	TicksPerSec = 0x16E3600;

	return 0;
}

