#include "openiboot.h"
#include "clock.h"
#include "util.h"
#ifndef CONFIG_IPHONE_4G
#include "hardware/clock0.h"
#include "hardware/clock1.h"
#else
#include "hardware/clockS5L8730.h"
#endif
#include "power.h"
#include "openiboot-asmhelpers.h"

uint32_t ClockPLL;
uint32_t PLLFrequencies[4];

uint32_t ClockFrequency;
uint32_t MemoryFrequency;
uint32_t BusFrequency;
uint32_t PeripheralFrequency;
uint32_t UnknownFrequency;   
uint32_t DisplayFrequency;  
uint32_t FixedFrequency;
uint32_t TimebaseFrequency;
#ifdef CONFIG_IPHONE_4G
uint32_t UsbPhyFrequency;

uint32_t IHaveNoIdeaWhatsThatFor = 0; // 0x5FF3D120 - not populated before clock_setup. I assume there's nothing but 0x0
uint32_t IHaveNoIdeaWhatsThatFor2; // 0x5FF3D11C
#endif

uint32_t TicksPerSec;

#ifndef CONFIG_IPHONE_4G
uint32_t ClockSDiv;

int clock_setup() {
	uint32_t config;

	config = GET_REG(CLOCK1 + CLOCK1_CONFIG0);
	uint32_t clockPLL = CLOCK1_CLOCKPLL(config);
	uint32_t clockDivisor;

	if(CLOCK1_CLOCKHASDIVIDER(config)) {
		clockDivisor = CLOCK1_CLOCKDIVIDER(config);
	} else {
		clockDivisor = 1;
	}

	bufferPrintf("PLL %d: off.\n", clockPLL);

	uint32_t memoryPLL = CLOCK1_MEMORYPLL(config);
	uint32_t memoryDivisor;

	if(CLOCK1_MEMORYHASDIVIDER(config)) {
		memoryDivisor = CLOCK1_MEMORYDIVIDER(config);
	} else {
		memoryDivisor = 1;
	}


	config = GET_REG(CLOCK1 + CLOCK1_CONFIG1);
	uint32_t busPLL = CLOCK1_BUSPLL(config);
	uint32_t busDivisor;

	if(CLOCK1_BUSHASDIVIDER(config)) {
		busDivisor = CLOCK1_BUSDIVIDER(config);
	} else {
		busDivisor = 1;
	}


	uint32_t unknownPLL = CLOCK1_UNKNOWNPLL(config);
	uint32_t unknownDivisor;

	if(CLOCK1_UNKNOWNHASDIVIDER(config)) {
		unknownDivisor = CLOCK1_UNKNOWNDIVIDER(config);
	} else {
		unknownDivisor = 1;
	}

	uint32_t peripheralFactor = CLOCK1_PERIPHERALDIVIDER(config);

	config = GET_REG(CLOCK1 + CLOCK1_CONFIG2);
	uint32_t displayPLL = CLOCK1_DISPLAYPLL(config);
	uint32_t displayDivisor;

	if(CLOCK1_DISPLAYDIVIDER(config)) {
		displayDivisor = CLOCK1_DISPLAYHASDIVIDER(config) + 1;
	} else {
		displayDivisor = 1;
	}

	ClockPLL = clockPLL;

	/* Now we get to populate the PLLFrequencies table so we can calculate the various clock frequencies */
	int pll = 0;
	for(pll = 0; pll < NUM_PLL; pll++) {
		uint32_t pllStates = GET_REG(CLOCK1 + CLOCK1_PLLMODE);
		if(CLOCK1_PLLMODE_ONOFF(pllStates, pll)) {
			int dividerMode = CLOCK1_PLLMODE_DIVIDERMODE(pllStates, pll);
			int dividerModeInputFrequency = dividerMode;
			uint32_t inputFrequency;
			uint32_t inputFrequencyMult;
			uint32_t pllConReg;
			uint32_t pllCon;

			if(pll == 0) {
				inputFrequency = PLL0_INFREQ_DIV * 2;
				pllConReg = CLOCK1 + CLOCK1_PLL0CON;
				inputFrequencyMult = PLL0_INFREQ_MULT * 2;
			} else if(pll == 1) {
				inputFrequency = PLL1_INFREQ_DIV * 2;
				pllConReg = CLOCK1 + CLOCK1_PLL1CON;
				inputFrequencyMult = PLL1_INFREQ_MULT * 2;
			} else if(pll == 2) {
				inputFrequency = PLL2_INFREQ_DIV * 2;
				pllConReg = CLOCK1 + CLOCK1_PLL2CON;
				inputFrequencyMult = PLL2_INFREQ_MULT * 2;
			} else if(pll == 3) {
				inputFrequency = PLL3_INFREQ_DIV * 2;
				pllConReg = CLOCK1 + CLOCK1_PLL3CON;
				inputFrequencyMult = PLL3_INFREQ_MULT * 2;
				dividerMode = CLOCK1_PLLMODE_DIVIDE;
			} else {
				PLLFrequencies[pll] = 0;
				continue;
			}

			if(dividerModeInputFrequency == CLOCK1_PLLMODE_MULTIPLY) {
				inputFrequency = inputFrequencyMult;
			}

			pllCon = GET_REG(pllConReg);

			uint64_t afterMDiv = inputFrequency * CLOCK1_MDIV(pllCon);
			uint64_t afterPDiv;
			if(dividerMode == CLOCK1_PLLMODE_DIVIDE) {
				afterPDiv = afterMDiv / CLOCK1_PDIV(pllCon);
			} else {
				afterPDiv = afterMDiv * CLOCK1_PDIV(pllCon);
			}

			uint64_t afterSDiv;
			if(pll != clockPLL) {
				afterSDiv = afterPDiv / (((uint64_t)0x1) << CLOCK1_SDIV(pllCon));
			} else {
				afterSDiv = afterPDiv;
				ClockSDiv = CLOCK1_SDIV(pllCon);
			}

			PLLFrequencies[pll] = (uint32_t)afterSDiv;

			bufferPrintf("PLL %d: %d\n", pll, PLLFrequencies[pll]);
		} else {
			PLLFrequencies[pll] = 0;
			bufferPrintf("PLL %d: off.\n", pll);
			continue;
		}
	}

	ClockFrequency = PLLFrequencies[clockPLL] / clockDivisor;
	MemoryFrequency = PLLFrequencies[memoryPLL] / memoryDivisor;
	BusFrequency = PLLFrequencies[busPLL] / busDivisor;
	UnknownFrequency = PLLFrequencies[unknownPLL] / unknownDivisor;
	PeripheralFrequency = BusFrequency / (1 << peripheralFactor);
	DisplayFrequency = PLLFrequencies[displayPLL] / displayDivisor;
	FixedFrequency = FREQUENCY_BASE * 2;
	TimebaseFrequency = FREQUENCY_BASE / 2;

	TicksPerSec = FREQUENCY_BASE;

	return 0;
}
#endif

void clock_gate_switch(uint32_t gate, OnOff on_off) {
#ifndef CONFIG_IPHONE_4G
	uint32_t gate_register;
	uint32_t gate_flag;

	if(gate < CLOCK1_Separator) {
		gate_register = CLOCK1 + CLOCK1_CL2_GATES;
		gate_flag = gate;
	} else {
		gate_register = CLOCK1 + CLOCK1_CL3_GATES;
		gate_flag = gate - CLOCK1_Separator;
	}

	uint32_t gates = GET_REG(gate_register);

	if(on_off == ON) {
		gates &= ~(1 << gate_flag);
	} else {
		gates |= 1 << gate_flag;
	}

	SET_REG(gate_register, gates);
#else
	power_ctrl(gate, on_off);
#endif
}

#ifndef CONFIG_IPHONE_4G
int clock_set_bottom_bits_38100000(Clock0ConfigCode code) {
	int bottomValue;

	switch(code) {
		case Clock0ConfigCode0:
			bottomValue = CLOCK0_CONFIG_C0VALUE;
			break;
		case Clock0ConfigCode1:
			bottomValue = CLOCK0_CONFIG_C1VALUE;
			break;
		case Clock0ConfigCode2:
			bottomValue = CLOCK0_CONFIG_C2VALUE;
			break;
		case Clock0ConfigCode3:
			bottomValue = CLOCK0_CONFIG_C3VALUE;
			break;
		default:
			return -1;
	}

	SET_REG(CLOCK0 + CLOCK0_CONFIG, (GET_REG(CLOCK0 + CLOCK0_CONFIG) & (~CLOCK0_CONFIG_BOTTOMMASK)) | bottomValue);

	return 0;
}
#endif

uint32_t clock_get_frequency(FrequencyBase freqBase) {
	switch(freqBase) {
		case FrequencyBaseClock:
			return ClockFrequency;
		case FrequencyBaseMemory:
			return MemoryFrequency;
		case FrequencyBaseBus:
			return BusFrequency;
		case FrequencyBasePeripheral:
			return PeripheralFrequency;
		case FrequencyBaseUnknown:
			return UnknownFrequency;
		case FrequencyBaseDisplay:
			return DisplayFrequency;
		case FrequencyBaseFixed:
			return FixedFrequency;
		case FrequencyBaseTimebase:
			return TimebaseFrequency;
		default:
			return 0;
	}
}
#ifndef CONFIG_IPHONE_4G
uint32_t clock_calculate_frequency(uint32_t pdiv, uint32_t mdiv, FrequencyBase freqBase) {
	unsigned int y = clock_get_frequency(freqBase) / (0x1 << ClockSDiv);
	uint64_t z = (((uint64_t) pdiv) * ((uint64_t) 1000000000)) / ((uint64_t) y);
	uint64_t divResult = ((uint64_t)(1000000 * mdiv)) / z;
	return divResult - 1;
}

static void clock0_reset_frequency() {
	SET_REG(CLOCK0 + CLOCK0_ADJ1, (GET_REG(CLOCK0 + CLOCK0_ADJ1) & CLOCK0_ADJ_MASK) | clock_calculate_frequency(0x200C, 0x40, FrequencyBaseMemory));
	SET_REG(CLOCK0 + CLOCK0_ADJ2, (GET_REG(CLOCK0 + CLOCK0_ADJ2) & CLOCK0_ADJ_MASK) | clock_calculate_frequency(0x1018, 0x4, FrequencyBaseBus));
}

void clock_set_sdiv(int sdiv) {
	int oldClockSDiv = ClockSDiv;

	if(sdiv < 1) {
		sdiv = 0;
	}

	if(sdiv <= 2) {
		ClockSDiv = sdiv;
	}

	if(oldClockSDiv < ClockSDiv) {
		clock0_reset_frequency();
	}

	uint32_t clockCon = 0;
	switch(ClockPLL) {
		case 0:
			clockCon = CLOCK1 + CLOCK1_PLL0CON;
			break;
		case 1:
			clockCon = CLOCK1 + CLOCK1_PLL1CON;
			break;
		case 2:
			clockCon = CLOCK1 + CLOCK1_PLL2CON;
			break;
		case 3:
			clockCon = CLOCK1 + CLOCK1_PLL3CON;
			break;
	}

	if(clockCon != 0) {
		SET_REG(clockCon, (GET_REG(clockCon) & ~0x7) | ClockSDiv);
	}

	if(oldClockSDiv >= ClockSDiv) {
		clock0_reset_frequency();
	}
}
#endif

#ifdef CONFIG_IPHONE_4G
uint32_t CalculatedFrequencyTable[55] = {
};

ClockThirdStruct ClockThirdTable[6] = {
	{0,		0,	},
	{0x1F,		0,	},
	{0x1F00,	8,	},
	{0x3E000,	0xD	},
	{0x7C0000,	0x12	},
	{0xF800000,	0x17	}
};

ClockStruct DerivedFrequencySourceTable[55] = {
	{0,		0,	{0,	0,	0,	0	}},
	{0,		0,	{0,	0,	0,	0	}},
	{0,		0,	{0,	0,	0,	0	}},
	{0,		0,	{0,	0,	0,	0	}},
	{0,		0,	{0,	0,	0,	0	}},
	{0xBF100040,	1,	{1,	2,	3,	0	}},
	{0xBF100040,	2,	{5,	5,	5,	5	}},
	{0xBF100040,	3,	{1,	2,	3,	0	}},
	{0xBF100040,	4,	{1,	2,	3,	0	}},
	{0xBF100040,	5,	{1,	2,	3,	0	}},
	{0xBF100044,	1,	{7,	8,	3,	0	}},
	{0xBF100048,	1,	{7,	8,	3,	0	}},
	{0xBF10004C,	1,	{7,	8,	3,	0	}},
	{0xBF100050,	1,	{7,	8,	3,	0	}},
	{0xBF100054,	1,	{7,	8,	3,	0	}},
	{0xBF100070,	0,	{0xA,	0xB,	0xC,	0xD	}},
	{0xBF100070,	1,	{0xF,	0xF,	0xF,	0xF	}},
	{0xBF100070,	2,	{0xF,	0xF,	0xF,	0xF	}},
	{0xBF100074,	0,	{0xA,	0xB,	0xC,	0xD	}},
	{0xBF100074,	1,	{0x12,	0x12,	0x12,	0x12	}},
	{0xBF100078,	1,	{9,	9,	9,	9	}},
	{0xBF100068,	1,	{7,	8,	3,	0	}},
	{0xBF1000CC,	1,	{4,	4,	4,	4	}},
	{0xBF1000C4,	1,	{0,	0,	0,	0	}},
	{0xBF100060,	1,	{7,	8,	3,	0	}},
	{0xBF100064,	1,	{7,	8,	3,	0	}},
	{0xBF1000A0,	1,	{0xF,	0x10,	0x14,	0x12	}},
	{0xBF1000B4,	0,	{0x13,	0x10,	0x11,	0x12	}},
	{0xBF100098,	0,	{0xF,	0x10,	0x14,	0x12	}},
	{0xBF100090,	1,	{0xA,	0xB,	0xC,	0xD	}},
	{0xBF10009C,	0,	{0xF,	0x10,	0x14,	0x12	}},
	{0xBF10008C,	1,	{0xA,	0xB,	0xC,	0xD	}},
	{0xBF1000A4,	0,	{0x13,	0x10,	0x11,	0x12	}},
	{0xBF1000A8,	0,	{0xF,	0x10,	0x14,	0x12	}},
	{0xBF1000AC,	0,	{0x13,	0x10,	0x11,	0x12	}},
	{0xBF1000B0,	0,	{0x13,	0x10,	0x11,	0x12	}},
	{0xBF100094,	1,	{0xA,	0xB,	0xC,	0xE	}},
	{0xBF10005C,	1,	{7,	8,	3,	0	}},
	{0xBF100080,	1,	{0xA,	0xB,	0xC,	0xD	}},
	{0xBF100084,	1,	{0xA,	0xB,	0xC,	0xD	}},
	{0xBF100088,	1,	{0xA,	0xB,	0xC,	0xD	}},
	{0xBF1000B8,	0,	{0x13,	0x10,	0x11,	0x12	}},
	{0xBF1000FC,	1,	{0x17,	0x17,	0x17,	0x17	}},
	{0xBF1000BC,	0,	{0x13,	0x10,	0x11,	0x12	}},
	{0xBF1000C8,	1,	{4,	4,	4,	4	}},
	{0xBF1000D0,	0,	{0x15,	0x16,	0,	0	}},
	{0xBF1000D4,	1,	{0,	0,	0,	0	}},
	{0xBF1000DC,	0,	{0x15,	0x16,	0,	0	}},
	{0xBF1000E0,	0,	{0x15,	0x16,	0,	0	}},
	{0xBF1000E4,	0,	{0x15,	0x16,	0,	0	}},
	{0xBF1000E8,	0,	{0x15,	0x16,	0,	0	}},
	{0xBF1000EC,	0,	{0x15,	0x16,	0,	0	}},
	{0xBF1000F0,	1,	{0x17,	0x17,	0x17,	0x17	}},
	{0xBF1000F4,	1,	{0x17,	0x17,	0x17,	0x17	}},
	{0xBF1000F8,	1,	{0x17,	0x17,	0x17,	0x17	}}
};

void derived_frequency_table_setup() {
	int round;

	for (round = 0; round != 55; round++) {
		if (DerivedFrequencySourceTable[round].address == 0) {
			continue;
		}
		uint32_t clock_register_bits = GET_BITS(DerivedFrequencySourceTable[round].address, 28, 2);
		uint8_t unkn_byte = DerivedFrequencySourceTable[round].unkn_bytes[clock_register_bits];
		uint32_t calculated_frequency = CalculatedFrequencyTable[unkn_byte];
		uint32_t unknown = DerivedFrequencySourceTable[round].unkn1;
		if (DerivedFrequencySourceTable[round].unkn1 == 0) {
			CalculatedFrequencyTable[round] = calculated_frequency;
			continue;
		}

		uint32_t divisor = (GET_REG(DerivedFrequencySourceTable[round].address) & ClockThirdTable[unknown].unkn1) >> ClockThirdTable[unknown].unkn2;
		if (divisor == ClockThirdTable[unknown].unkn2)  {
			continue;
		}
		CalculatedFrequencyTable[round] = (((round == 5) ? IHaveNoIdeaWhatsThatFor : 1) * calculated_frequency) / divisor;
	}
}

uint32_t calculate_reference_frequency(uint32_t clock_register) {
	 return (GET_BITS(clock_register, 3, 10) * 0x2DC6C00) / (GET_BITS(clock_register, 14, 6) * (1 << GET_BITS(clock_register, 0, 3)));
}

int clock_setup() {
	CalculatedFrequencyTable[0] = CLOCK_REFERENCE_0_FREQUENCY;
	if(!CLOCK_ACTIVE(CLOCK_REFERENCE_1)) {
		CalculatedFrequencyTable[1] = calculate_reference_frequency(CLOCK_REFERENCE_1);
	}
	if(!CLOCK_ACTIVE(CLOCK_REFERENCE_2)) {
		CalculatedFrequencyTable[2] = calculate_reference_frequency(CLOCK_REFERENCE_2);
	}
	if(!CLOCK_ACTIVE(CLOCK_REFERENCE_3)) {
		CalculatedFrequencyTable[3] = calculate_reference_frequency(CLOCK_REFERENCE_3);
	}
	if(!CLOCK_ACTIVE(CLOCK_REFERENCE_4)) {
		CalculatedFrequencyTable[4] = calculate_reference_frequency(CLOCK_REFERENCE_4);
	}
	derived_frequency_table_setup();
	IHaveNoIdeaWhatsThatFor = GET_BITS(0xBF100040, 0, 5);
	if (GET_BITS(0xBF100040, 0, 5) == 2) {
		IHaveNoIdeaWhatsThatFor2 = 1;
	} else if (GET_BITS(0xBF100040, 0, 5) == 4) {
		IHaveNoIdeaWhatsThatFor2 = 2;
	} else {
		IHaveNoIdeaWhatsThatFor2 = 0;
	}


	ClockFrequency = CalculatedFrequencyTable[5];
	MemoryFrequency = CalculatedFrequencyTable[6];
	BusFrequency = CalculatedFrequencyTable[27];
	PeripheralFrequency = CalculatedFrequencyTable[32];
	FixedFrequency = CalculatedFrequencyTable[0];
	TimebaseFrequency = CalculatedFrequencyTable[0];
	UsbPhyFrequency = CalculatedFrequencyTable[0];

	TicksPerSec = CalculatedFrequencyTable[0];

	return 0;
}
/*
	CLOCK_FREQUENCY = get_frequency(0);
	MEMORY_FREQUENCY = get_frequency(2);
	BUS_FREQUENCY = get_frequency(3);
	PERIPHERAL_FREQUENCY = get_frequency(1);
	FIXED_FREQUENCY = get_frequency(4);
	TIMEBASE_FREQUENCY = get_frequency(5);
	CLOCK_FREQUENCIES = get_derived_frequencies.. omg;
	USBPHY_FREQUENCY = get_frequency(0xE);
	NCOREF_FREQUENCY = get_frequency(0xF); // 16
BASE = 0x5FF3D040;
default, 11, 12 => Frequency = 0;
0, 6 => BASE + 0x14; 5
2, 7 => BASE + 0x18; 6
3, 8 => BASE + 0x6C; 27
1, 9 => BASE + 0x80; 32
16 => BASE + 0x84; 33
4, 5, 13, 14 => BASE;
15 => BASE + 0x60; 24
10 => BASE + 0x90; 36
	*/

#endif
