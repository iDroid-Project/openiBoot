#include "openiboot.h"
#include "clock.h"
#include "util.h"
#include "hardware/clock.h"
#include "timer.h"
#include "arm/arm.h"

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
uint32_t UsbPhyFrequency;
uint32_t NANDFrequency;

uint32_t CPU_clock_setting;
uint32_t clock_freq_multiplier;

uint32_t TicksPerSec;

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
		case FrequencyBaseUsbPhy:
			return UsbPhyFrequency;
		case FrequencyNand:
			return NANDFrequency;
		default:
			return 0;
	}
}

uint32_t CalculatedFrequencyTable[55] = {
	0x016e3600, 0x2faf0800, 0x1dcd6500, 0x3d278480, 0x00000000, 0x00000000,
	0x00000000, 0x2faf0800, 0x00000000, 0x2faf0800, 0x0bebc200, 0x061d8d40,
	0x3d278480, 0x09896800, 0x030ec6a0, 0x0bebc200, 0x05f5e100, 0x00000000,
	0x061d8d40, 0x030ec6a0, 0x0bebc200, 0x02e98036, 0x00000000, 0x001e8480,
	0x14628180, 0x00000000, 0x0bebc200, 0x05f5e100, 0x0bebc200, 0x09896800,
	0x0bebc200, 0x0a3140c0, 0x05f5e100, 0x061d8d40, 0x05f5e100, 0x05f5e100,
	0x030ec6a0, 0x0337f980, 0x00000000, 0x0413b380, 0x14628180, 0x030ec6a0,
	0x00000000, 0x030ec6a0, 0x00000000, 0x00000000, 0x00b71b00, 0x016e3600,
	0x016e3600, 0x02e98036, 0x016e3600, 0x016e3600, 0x000f4240, 0x000f4240,
	0x000f4240
};

/*
ClockThirdStruct ClockThirdTable[6] = {
	{0,		0,	}, // GET_BITS(register, 0, 0);
	{0x1F,		0,	}, // GET_BITS(register, 0, 5);
	{0x1F00,	8,	}, // GET_BITS(register, 8, 5);
	{0x3E000,	0xD	}, // GET_BITS(register, 13, 5);
	{0x7C0000,	0x12	}, // GET_BITS(register, 18, 5);
	{0xF800000,	0x17	} // GET_BITS(register, 23, 5);
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
		uint32_t clock_register_bits = GET_BITS(GET_REG(DerivedFrequencySourceTable[round].address), 28, 2);
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
		CalculatedFrequencyTable[round] = (uint32_t)(((uint64_t)((round == 5) ? IHaveNoIdeaWhatsThatFor : 1) * (uint64_t)calculated_frequency) / (uint64_t)divisor);
	}
}

uint32_t calculate_reference_frequency(uint32_t clock_register) {
	return (((uint64_t)GET_BITS(GET_REG(clock_register), 3, 10) * (uint64_t)0x2DC6C00) / ((uint64_t)GET_BITS(GET_REG(clock_register), 14, 6) * (uint64_t)(1 << GET_BITS(GET_REG(clock_register), 0, 3))));
}
*/

int clock_setup() {
/*
	CalculatedFrequencyTable[0] = CLOCK_REFERENCE_0_FREQUENCY;
	if(CLOCK_ACTIVE(CLOCK_REFERENCE_1) == 0) {
		CalculatedFrequencyTable[1] = calculate_reference_frequency(CLOCK_REFERENCE_1);
	}
	if(CLOCK_ACTIVE(CLOCK_REFERENCE_2) == 0) {
		CalculatedFrequencyTable[2] = calculate_reference_frequency(CLOCK_REFERENCE_2);
	}
	if(CLOCK_ACTIVE(CLOCK_REFERENCE_3) == 0) {
		CalculatedFrequencyTable[3] = calculate_reference_frequency(CLOCK_REFERENCE_3);
	}

XXX:	It actually never gets there.

	if(!CLOCK_ACTIVE(CLOCK_REFERENCE_4)) {
		CalculatedFrequencyTable[4] = calculate_reference_frequency(CLOCK_REFERENCE_4);
	}

XXX:	Base Frequencies as they are set by LLB

	derived_frequency_table_setup();
	clock_freq_multiplier = GET_BITS(GET_REG(0xBF100040), 0, 5);
	if (clock_freq_multiplier == 2) {
		CPU_clock_setting = 1;
	} else if (clock_freq_multiplier == 4) {
		CPU_clock_setting = 2;
	} else {
		CPU_clock_setting = 0;
	}
*/

	ClockFrequency = CalculatedFrequencyTable[5];
	MemoryFrequency = CalculatedFrequencyTable[6];
	BusFrequency = CalculatedFrequencyTable[27];
	PeripheralFrequency = CalculatedFrequencyTable[32];
	FixedFrequency = CalculatedFrequencyTable[0];
	TimebaseFrequency = CalculatedFrequencyTable[0];
	UsbPhyFrequency = CalculatedFrequencyTable[0];
	NANDFrequency = CalculatedFrequencyTable[33];

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
	NCOREF_FREQUENCY = get_frequency(0xF); // 15
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

