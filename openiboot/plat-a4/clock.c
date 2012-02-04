#include "openiboot.h"
#include "clock.h"
#include "util.h"
#include "hardware/clock.h"
#include "timer.h"
#include "arm/arm.h"
#include "commands.h"

typedef struct _pll_regs
{
	reg32_t con0;
	reg32_t con1;
} pll_regs_t;

static const pll_regs_t pll_regs[] = {
	{ CLOCK_APLL_CON0, CLOCK_APLL_CON1, },
	{ CLOCK_MPLL_CON0, CLOCK_MPLL_CON1, },
	{ CLOCK_EPLL_CON0, CLOCK_EPLL_CON1, },
	{ CLOCK_VPLL_CON0, CLOCK_VPLL_CON1, },
};

typedef struct _clock_divider
{
	uint32_t mask;
	uint32_t shift;
} clock_divider_t;

static const clock_divider_t clock_dividers[] = {
	{0,			0, },
	{0x1F,		0, },
	{0x1F00,	8, },
	{0x3E000,	0xD },
	{0x7C0000,	0x12 },
	{0xF800000,	0x17 }
};

typedef struct _clock_t
{
	reg32_t reg;
	uint32_t divider; // index into clock_dividers
	uint8_t sources[4];
} clock_t;

static const clock_t clocks[] = {
	{ 0,					0,	{ 0,	0,	0,	0 }},
	{ 0,					0,	{ 0,	0,	0,	0 }},
	{ 0,					0,	{ 0,	0,	0,	0 }},
	{ 0,					0,	{ 0,	0,	0,	0 }},
	{ 0,					0,	{ 0,	0,	0,	0 }},
	{ PMGR0_BASE + 0x40,	1,	{ 1,	2,	3,	0 }}, // ARMCLK
	{ PMGR0_BASE + 0x40,	2,	{ 5,	5,	5,	5 }},
	{ PMGR0_BASE + 0x40,	3,	{ 1,	2,	3,	0 }},
	{ PMGR0_BASE + 0x40,	4,	{ 1,	2,	3,	0 }},
	{ PMGR0_BASE + 0x40,	5,	{ 1,	2,	3,	0 }},
	{ PMGR0_BASE + 0x44,	1,	{ 7,	8,	3,	0 }}, // 10	// PREDIV0
	{ PMGR0_BASE + 0x48,	1,	{ 7,	8,	3,	0 }},		// PREDIV1
	{ PMGR0_BASE + 0x4C,	1,	{ 7,	8,	3,	0 }},		// PREDIV2
	{ PMGR0_BASE + 0x50,	1,	{ 7,	8,	3,	0 }},		// PREDIV3
	{ PMGR0_BASE + 0x54,	1,	{ 7,	8,	3,	0 }},		// PREDIV4
	{ PMGR0_BASE + 0x70,	0,	{ 0xA,	0xB,	0xC,	0xD }},	// BASE0
	{ PMGR0_BASE + 0x70,	1,	{ 0xF,	0xF,	0xF,	0xF }},	// BASE0_DIV0
	{ PMGR0_BASE + 0x70,	2,	{ 0xF,	0xF,	0xF,	0xF }},	// BASE0_DIV1
	{ PMGR0_BASE + 0x74,	0,	{ 0xA,	0xB,	0xC,	0xD }},	// BASE1
	{ PMGR0_BASE + 0x74,	1,	{ 0x12,	0x12,	0x12,	0x12 }},// BASE1_DIV0
	{ PMGR0_BASE + 0x78,	1,	{ 9,	9,	9,	9 }}, // 20		// BASE2
	{ PMGR0_BASE + 0x68,	1,	{ 7,	8,	3,	0 }},	// MEDIUM0
	{ PMGR0_BASE + 0xCC,	1,	{ 4,	4,	4,	4 }},	// MEDIUM1
	{ PMGR0_BASE + 0xC4,	1,	{ 0,	0,	0,	0 }},	
	{ PMGR0_BASE + 0x60,	1,	{ 7,	8,	3,	0 }},	// NCO_REF0
	{ PMGR0_BASE + 0x64,	1,	{ 7,	8,	3,	0 }},	// NCO_REF1
	{ PMGR0_BASE + 0xA0,	1,	{ 0xF,	0x10,	0x14,	0x12 }}, // CDMA
	{ PMGR0_BASE + 0xB4,	0,	{ 0x13,	0x10,	0x11,	0x12 }},
	{ PMGR0_BASE + 0x98,	0,	{ 0xF,	0x10,	0x14,	0x12 }}, // HPERF0
	{ PMGR0_BASE + 0x90,	1,	{ 0xA,	0xB,	0xC,	0xD }},	// HPERF1
	{ PMGR0_BASE + 0x9C,	0,	{ 0xF,	0x10,	0x14,	0x12 }}, // 30 // HPERF2-CLK
	{ PMGR0_BASE + 0x8C,	1,	{ 0xA,	0xB,	0xC,	0xD }},		// MPERF
	{ PMGR0_BASE + 0xA4,	0,	{ 0x13,	0x10,	0x11,	0x12 }},	// LPERF0
	{ PMGR0_BASE + 0xA8,	0,	{ 0xF,	0x10,	0x14,	0x12 }},	// LPERF1
	{ PMGR0_BASE + 0xAC,	0,	{ 0x13,	0x10,	0x11,	0x12 }},	// LPERF2
	{ PMGR0_BASE + 0xB0,	0,	{ 0x13,	0x10,	0x11,	0x12 }},	// LPERF3
	{ PMGR0_BASE + 0x94,	1,	{ 0xA,	0xB,	0xC,	0xE }},	// VID0-CLK
	{ PMGR0_BASE + 0x5C,	1,	{ 7,	8,	3,	0 }},			// VID1-CLK
	{ PMGR0_BASE + 0x80,	1,	{ 0xA,	0xB,	0xC,	0xD }},
	{ PMGR0_BASE + 0x84,	1,	{ 0xA,	0xB,	0xC,	0xD }}, // AUDIO-CLK
	{ PMGR0_BASE + 0x88,	1,	{ 0xA,	0xB,	0xC,	0xD }}, // 40 // MIPI-CLK
	{ PMGR0_BASE + 0xB8,	0,	{ 0x13,	0x10,	0x11,	0x12 }}, // SDIO-CLKs
	{ PMGR0_BASE + 0xFC,	1,	{ 0x17,	0x17,	0x17,	0x17 }},
	{ PMGR0_BASE + 0xBC,	0,	{ 0x13,	0x10,	0x11,	0x12 }}, // CLK50-CLK
	{ PMGR0_BASE + 0xC8,	1,	{ 4,	4,	4,	4 }},
	{ PMGR0_BASE + 0xD0,	0,	{ 0x15,	0x16,	0,	0 }},
	{ PMGR0_BASE + 0xD4,	1,	{ 0,	0,	0,	0 }},
	{ PMGR0_BASE + 0xDC,	0,	{ 0x15,	0x16,	0,	0 }},		// SPI0
	{ PMGR0_BASE + 0xE0,	0,	{ 0x15,	0x16,	0,	0 }},		// SPI1
	{ PMGR0_BASE + 0xE4,	0,	{ 0x15,	0x16,	0,	0 }},		// SPI2	
	{ PMGR0_BASE + 0xE8,	0,	{ 0x15,	0x16,	0,	0 }}, // 50 // SPI3
	{ PMGR0_BASE + 0xEC,	0,	{ 0x15,	0x16,	0,	0 }},		// SPI4
	{ PMGR0_BASE + 0xF0,	1,	{ 0x17,	0x17,	0x17,	0x17 }}, // I2C0
	{ PMGR0_BASE + 0xF4,	1,	{ 0x17,	0x17,	0x17,	0x17 }}, // I2C1
	{ PMGR0_BASE + 0xF8,	1,	{ 0x17,	0x17,	0x17,	0x17 }}  // I2C2
};

static const uint32_t clock_reset_values[] = {
	0xB0842101, 0xB0000001, 0xB0000001, 0xB0000001,
	0xB0000001, 0xB0000001, 0, 0xB0000001,
	0xB0000001, 0xB0000001, 0xB0000001, 0,
	0xB0000101, 0xB0000001, 0x80000001, 0,
	0xB0000001, 0xB0000001, 0xB0000001, 0xB0000001,
	0xB0000001, 0xB0000001, 0xB0000000, 0xB0000000,
	0xB0000000, 0xB0000000, 0xB0000000, 0xB0000000,
	0xB0000000, 0xB0000000, 0xB0000000, 0xB0000000,
	0, 0x80000001, 0x80000000, 0x8000001F,
	0xB0000000,	0xB0000001, 0, 0xB0000000,
	0xB0000000, 0xB0000000,	0xB0000000, 0xB0000000,
   	0x80000001, 0x80000001,	0x80000001, 0x80000001
};

static const uint32_t clock_init_values[] = {
	0x80802401, 0x80000004, 0xA000000A, 0xA0000001,
	0x80000005, 0xA000001F, 0, 0xA0000013,
	0xA0000003, 0x30000000, 0xA0000015, 0,
	0x80000002, 0x90000002, 0x80000004, 0,
	0, 0xA000000F, 0xA0000003, 0xA0000006,
	0xB0000001, 0xB000001F, 0x80000000, 0xA0000000,
	0x80000000, 0x90000000, 0xB0000000, 0x90000000,
	0x90000000, 0x90000000, 0x80000000, 0,
	0, 0x8000000C, 0x80000000, 0x80000001,
	0x90000000, 0x80000002, 0, 0xB0000000,
#ifdef CONFIG_IPOD_TOUCH_4G
	0xB0000000, 0xB0000000, 0xB0000000, 0xB0000000,
#else
	0xB0000000, 0x80000000, 0xB0000000, 0xB0000000,
#endif
	0x80000002, 0x80000002, 0x80000002, 0
};

uint32_t CPU_clock_setting;
uint32_t clock_freq_multiplier;
uint32_t TicksPerSec;
uint32_t CalculatedFrequencyTable[55];

void clock_gate_switch(uint32_t gate, OnOff on_off) {
	if (gate > CLOCK_GATE_MAX)
		return;

	uint32_t reg = CLOCK_GATE_BASE + (gate << 2);

	if (on_off == ON) {
		SET_REG(reg, GET_REG(reg) | 0xF);
	} else {
		SET_REG(reg, GET_REG(reg) & ~0xF);
	}
	
	/* wait for the new state to take effect */
	while ((GET_REG(reg) & 0xF) != ((GET_REG(reg) >> 4) & 0xF));
}

void clock_gate_many(uint64_t _gates, OnOff _val)
{
	uint32_t reg = CLOCK_GATE_BASE;
	int num;
	for(num = 0; num < CLOCK_GATE_MAX; num++)
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

static error_t cmd_enable_all_clocks(int argc, char **argv)
{
	int i;
	for(i = 0; i <= CLOCK_GATE_MAX; i++)
		clock_gate_switch(i, ON);

	return SUCCESS;
}
COMMAND("enable_all_clocks", "Enable all clock-gates, for debugging.",
		cmd_enable_all_clocks);

void clock_gate_reset(uint32_t gate) {
	uint32_t pin = gate-22;
	if(pin > 19)
		return;

	if ((1 << pin) & 0xA4001) {
		uint32_t reg = CLOCK_GATE_BASE + (sizeof(uint32_t) * gate);
		SET_REG(reg, GET_REG(reg) | 0x80000000);
		udelay(1);
		SET_REG(reg, GET_REG(reg) &~ 0x80000000);
	}
}

uint32_t clock_get_frequency(FrequencyBase freqBase) {
	switch(freqBase) {
		case FrequencyBaseClock:
			return CalculatedFrequencyTable[5];
		case FrequencyBaseMemory:
			return CalculatedFrequencyTable[6];
		case FrequencyBaseBus:
			return CalculatedFrequencyTable[27];
		case FrequencyBasePeripheral:
			return CalculatedFrequencyTable[32];
		case FrequencyBaseFixed:
			return CalculatedFrequencyTable[0];
		case FrequencyBaseTimebase:
			return CalculatedFrequencyTable[0];
		case FrequencyBaseUsbPhy:
			return CalculatedFrequencyTable[0];
		case FrequencyNand:
			return CalculatedFrequencyTable[33];
		default:
			return 0;
	}
}

static void clock_reset()
{
	static const uint64_t defaultClocks = 0x80000080300002ULL;
	clock_gate_many(defaultClocks, ON);
	clock_gate_many(~defaultClocks, OFF);

	int i;
	for(i = 1; i < ARRAY_SIZE(clock_reset_values); i++)
		SET_REG(&CLOCK_CON_BASE[i], clock_reset_values[i]);

	SET_REG(CLOCK_CPUFREQ, clock_reset_values[0]);

	// Disable all PLLs so we can mess with them. :)
	SET_REG(CLOCK_APLL_CON0, GET_REG(CLOCK_APLL_CON0) &~ PLLCON0_ENABLE);
	SET_REG(CLOCK_MPLL_CON0, GET_REG(CLOCK_MPLL_CON0) &~ PLLCON0_ENABLE);
	SET_REG(CLOCK_EPLL_CON0, GET_REG(CLOCK_EPLL_CON0) &~ PLLCON0_ENABLE);
	SET_REG(CLOCK_VPLL_CON0, GET_REG(CLOCK_VPLL_CON0) &~ PLLCON0_ENABLE);

	SET_REG(CLOCK_CON0, GET_REG(CLOCK_CON0) &~ 0x3);
}

static error_t clock_setup_pll(uint32_t _pll, uint32_t _p, uint32_t _m, uint32_t _s)
{
	if(_pll >= NUM_PLL)
		return EINVAL;

	uint32_t freq = (_m * (uint64_t)CLOCK_BASE_HZ) / _p;
	uint32_t freq_sel;
	if(freq > 1500000000)
		freq_sel = PLLCON1_FSEL_2;
	else if(freq > 1099999999)
		freq_sel = PLLCON1_FSEL_1;
	else
		freq_sel = PLLCON1_FSEL_0;

	SET_REG(pll_regs[_pll].con1, (freq_sel << PLLCON1_FSEL_SHIFT) | PLLCON1_ENABLE);
	SET_REG(pll_regs[_pll].con0, (_m << PLLCON0_M_SHIFT) | (_p << PLLCON0_P_SHIFT)
			| (_s << PLLCON0_S_SHIFT) | PLLCON0_ENABLE | PLLCON0_UPDATE);

	while(!(GET_REG(pll_regs[_pll].con0) & PLLCON0_LOCKED));
	return SUCCESS;
}

uint32_t calculate_pll_frequency(uint32_t _pll)
{
	if(_pll >= NUM_PLL)
		return 0;

	uint32_t conf = GET_REG(pll_regs[_pll].con0);
	if(!(conf & PLLCON0_ENABLE))
		return 0;

	uint64_t ret = (PLLCON0_M(conf) * (uint64_t)CLOCK_BASE_HZ)
		/ (((uint64_t)PLLCON0_P(conf)) << PLLCON0_S(conf));
	return ret;
}

static error_t clock_init_perf()
{
	int i;
	for(i = 0; i < CLOCK_NUM_PERF; i++)
	{
		SET_REG(&CLOCK_PERF_BASE[1+i*2], 0x9009); 	// Bus divider?
		SET_REG(&CLOCK_PERF_BASE[i*2], 0x8241);		// CPU divider?
	}

	SET_REG(CLOCK_CON1, 7);
	while(GET_REG(CLOCK_CON1) & CLOCK_CON1_UNSTABLE);

	return SUCCESS;
}

static error_t clock_init_clocks()
{
	int i;
	for(i = 48; i > 0; i--)
	{
		if(i == 48)
		{
			SET_REG(&CLOCK_CON_BASE[0], clock_init_values[0]);
			continue;
		}

		SET_REG(&CLOCK_CON_BASE[i], clock_init_values[i]);
	}

	return SUCCESS;
}

static uint32_t clock_frequency(uint32_t _clock)
{
	if(_clock >= ARRAY_SIZE(clocks))
		return 0;

	if(clocks[_clock].reg)
	{
		uint32_t conf = GET_REG(clocks[_clock].reg);
		uint8_t src = CLOCK_CON_SRC(conf);
		uint32_t freq = CalculatedFrequencyTable[clocks[_clock].sources[src]];

		if(clocks[_clock].divider)
		{
			uint32_t div = CLOCK_CON_DIV(conf, clocks[_clock].divider);
			if(div)
			{
				uint32_t mul = (_clock == 5)? 4 : 1;
				freq = (mul * freq) / div;
			}
			else
				freq = 0;
		}

		return freq;
	}
	else
		return 0;
}

uint32_t clock_get_divisor(uint32_t _clock)
{
	if(_clock >= ARRAY_SIZE(clocks))
		return 0;

	const clock_t *clk = &clocks[_clock];
	if(!clk->reg)
		return 0;

	if(!clk->divider)
		return 1;

	return CLOCK_CON_DIV(GET_REG(clk->reg), clk->divider);
}

error_t clock_set_divisor(uint32_t _clock, uint32_t _div)
{
	if(_clock >= ARRAY_SIZE(clocks))
		return EINVAL;

	const clock_t *clk = &clocks[_clock];
	if(!clk->reg)
		return EINVAL;

	const clock_divider_t *div = &clock_dividers[clk->divider];

	uint32_t conf = (GET_REG(clk->reg) &~ div->mask);
	conf |= (_div << div->shift) & div->mask;
	SET_REG(clk->reg, conf);

	uint32_t freq = clock_frequency(_clock);
	CalculatedFrequencyTable[_clock] = freq;

	bufferPrintf("clocks: Reconfigured clock %d to %u. (0x%08x)\n",
			_clock, freq, conf);

	return SUCCESS;
}

static void clock_calculate_frequencies()
{
	CalculatedFrequencyTable[0] = 24000000;
	CalculatedFrequencyTable[1] = calculate_pll_frequency(0);
	CalculatedFrequencyTable[2] = calculate_pll_frequency(1);
	CalculatedFrequencyTable[3] = calculate_pll_frequency(2);
	CalculatedFrequencyTable[4] = calculate_pll_frequency(3);

	int i;
	for(i = 5; i < 55; i++)
		CalculatedFrequencyTable[i] = clock_frequency(i);
}

error_t clock_setup()
{
	if(0)
	{
		clock_reset();
		clock_init_perf();

		CHAIN_FAIL(clock_setup_pll(0, 6, 200, 1));
		CHAIN_FAIL(clock_setup_pll(1, 6, 125, 1));
		CHAIN_FAIL(clock_setup_pll(2, 4, 171, 1));
		CHAIN_FAIL(clock_setup_pll(3, 2, 64, 5));
	}

	clock_init_clocks();
	clock_calculate_frequencies();

	clock_freq_multiplier = GET_BITS(GET_REG(CLOCK_CPUFREQ), 0, 5);
	switch(clock_freq_multiplier) {
		case 2:
			CPU_clock_setting = 1;
			break;
		case 4:
			CPU_clock_setting = 2;
			break;
		default:
			CPU_clock_setting = 0;
			break;
	}

	TicksPerSec = CalculatedFrequencyTable[0];
	return SUCCESS;
}

void set_CPU_clockconfig(uint32_t _mode)
{
	uint32_t setting;

	switch(_mode) {
		case 1: // 1/2
			setting = 4;
			clock_freq_multiplier = 2;
			break;
		case 2: // 1/4
			setting = 4;
			clock_freq_multiplier = 4;
			break;
		case 3: // 6/10
			if (!CPU_clock_setting)
				return;
			setting = 1;
			clock_freq_multiplier = 4;
			break;
		default: // 1
			setting = 4;
			clock_freq_multiplier = 1;
			_mode = 0;
			break;
	}
	CPU_clock_setting = _mode;
	SET_REG(CLOCK_CPUFREQ, (GET_REG(CLOCK_CPUFREQ) & 0xFFFFE0E0) | clock_freq_multiplier | (setting << 8));
}

static error_t cmd_frequencies(int argc, char **argv)
{
	int i;
	for(i = 0; i < NUM_PLL; i++)
		bufferPrintf("clocks: PLL%d %u.\n", i, calculate_pll_frequency(i));

	for(i = 0; i < 55; i++)
		bufferPrintf("clocks: clock %d %u.\n", i, CalculatedFrequencyTable[i]);

	return SUCCESS;
}
COMMAND("frequencies", "Print clock frequencies.", cmd_frequencies);
