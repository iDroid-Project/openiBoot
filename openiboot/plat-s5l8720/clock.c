#include "openiboot.h"
#include "commands.h"
#include "clock.h"
#include "util.h"
#include "chipid.h"
#include "hardware/clock.h"
#include "power.h"
#include "arm/arm.h"

uint32_t ClockPLL;
uint32_t PLLFrequencies[NUM_PLL];

uint32_t ClockFrequency;
uint32_t MemoryFrequency;
uint32_t BusFrequency;
uint32_t PeripheralFrequency;
uint32_t UnknownFrequency;   
uint32_t DisplayFrequency;  
uint32_t FixedFrequency;
uint32_t TimebaseFrequency;
uint32_t UsbPhyFrequency;

uint32_t TicksPerSec;

error_t clock_setup() {
	uint32_t config;

	config = GET_REG(CLOCK + CLOCK_CONFIG0);
	uint32_t clockPLL = CLOCK_CLOCKPLL(config);

	// Config1 divisors
	config = GET_REG(CLOCK + CLOCK_CONFIG1);

	// Clock divisor
	uint32_t clockDivisor;
	if(CLOCK_CLOCKHASDIVIDER(config)) {
		clockDivisor = CLOCK_CLOCKDIVIDER(config);
	} else {
		clockDivisor = 1;
		bufferPrintf("PLL %d: off.\n", clockPLL);
	}
	
	// Bus divisor
	uint32_t busPLL = clockPLL;
	uint32_t busDivisor;
	if(CLOCK_BUSHASDIVIDER(config)) {
		busDivisor = CLOCK_BUSDIVIDER(config);
	} else {
		busDivisor = 1;
	}
	
	// Peripheral divisor
	uint32_t peripheralPLL = clockPLL;
	uint32_t peripheralDivisor;
	if(CLOCK_PERIPHERALHASDIVIDER(config)) {
		peripheralDivisor = CLOCK_PERIPHERALDIVIDER(config);
	} else {
		peripheralDivisor = 1;
	}
	
	// Memory divisor
	uint32_t memoryPLL = clockPLL;
	uint32_t memoryDivisor;
	if(CLOCK_MEMORYHASDIVIDER(config)) {
		memoryDivisor = CLOCK_MEMORYDIVIDER(config);
	} else {
		memoryDivisor = 1;
	}
	
	// Config2 divisors
	config = GET_REG(CLOCK + CLOCK_CONFIG2);
	
	// Unknown divisor
	uint32_t unknownPLL = CLOCK_UNKNOWNPLL(config);
	uint32_t unknownDivisor = CLOCK_UNKNOWNDIVIDER(config);

	// Display divisor
	uint32_t displayPLL = CLOCK_DISPLAYPLL(config);
	uint32_t displayDivisor = CLOCK_DISPLAYDIVIDER(config);
	
	// Populate the PLLFrequencies table
	int pll = 0;
	for (pll = 0; pll < NUM_PLL; pll++) {
		uint32_t pllStates = GET_REG(CLOCK + CLOCK_PLLMODE);
		if(CLOCK_PLLMODE_ONOFF(pllStates, pll)) {
			uint32_t inputFrequency;
			uint32_t pllConReg;
			uint32_t pllCon;
			
			if (CLOCK_PLLMODE_USE_SECURITY_EPOCH_INFREQ(pllStates, pll)) {
				inputFrequency = clock_get_base_frequency();
			} else {
				inputFrequency = CLOCK_STATIC_INFREQ;
			}
			
			if (pll == 0) {
				pllConReg = CLOCK + CLOCK_PLL0CON;
			} else if (pll == 1) {
				pllConReg = CLOCK + CLOCK_PLL1CON;
			} else if (pll == 2) {
				pllConReg = CLOCK + CLOCK_PLL2CON;
			} else {
				PLLFrequencies[pll] = 0;
				continue;
			}

			pllCon = GET_REG(pllConReg);

			uint64_t afterMDiv = inputFrequency * CLOCK_MDIV(pllCon);
			uint64_t afterPDiv = afterMDiv / CLOCK_PDIV(pllCon);
			uint64_t afterSDiv;
			if(pll != clockPLL) {
				afterSDiv = afterPDiv / (((uint64_t)0x1) << (CLOCK_SDIV(pllCon) + 1));
			} else {
				afterSDiv = afterPDiv;
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
	PeripheralFrequency = PLLFrequencies[peripheralPLL] / peripheralDivisor;
	UnknownFrequency = PLLFrequencies[unknownPLL] / unknownDivisor;
	DisplayFrequency = PLLFrequencies[displayPLL] / displayDivisor;
	if (chipid_get_security_epoch() == 1) {
		FixedFrequency = clock_get_base_frequency();
	} else {
		FixedFrequency = 0;
	}
	TimebaseFrequency = FixedFrequency / 4;
	UsbPhyFrequency = clock_get_base_frequency();

	TicksPerSec = FixedFrequency / 2;
	
	// Set the base divisor
	clock_set_base_divisor(ClockDivisorCode0);
	return SUCCESS;
}

ClockGate ClockGateTable[29] = {
	// id	reg_0_bits	reg_1_bits	reg_2_bits	reg_3_bits	reg_4_bits
	{0x0,	0x00000080,	0x0,		0x0,		0x0,		0x0		},
	{0x1,	0x0,		0x00004000,	0x0,		0x0,		0x0		},
	{0x2,	0x00004000,	0x0,		0x0,		0x00004E00,	0x0		},
	{0x3,	0x00000800,	0x0,		0x0,		0x0,		0x0		},
	{0x4,	0x00001000,	0x0,		0x0,		0x0,		0x0		},
	{0x5,	0x00000220,	0x0,		0x0,		0x0,		0x0		},
	{0x6,	0x0,		0x00001000,	0x0,		0x0,		0x0		},
	{0x7,	0x0,		0x00000010,	0x0,		0x0,		0x00000800	},
	{0x8,	0x0,		0x00000040,	0x0,		0x0,		0x00001000	},
	{0x9,	0x00000002,	0x0,		0x0,		0x0,		0x00010000	},
	{0xA,	0x0,		0x00080000,	0x0,		0x0,		0x0		},
	{0xB,	0x0,		0x00002000,	0x0,		0x0,		0x0		},
	{0xC,	0x00000400,	0x0,		0x0,		0x0,		0x0		},
	{0xD,	0x00000001,	0x0,		0x0,		0x0,		0x0		},
	{0xE,	0x0,		0x00000004,	0x0,		0x0,		0x00002000	},
	{0xF,	0x0,		0x00000800,	0x0,		0x0,		0x00004000	},
	{0x10,	0x0,		0x00008000,	0x0,		0x0,		0x00008000	},
	{0x11,	0x0,		0x0,		0x00000002,	0x0,		0x00080000	},
	{0x12,	0x0,		0x0,		0x00000010,	0x0,		0x00100000	},
	{0x13,	0x0,		0x1F800020,	0x00000060,	0x0,		0x00C0007F	},
	{0x14,	0x0,		0x00000200,	0x0,		0x0,		0x00000080	},
	{0x15,	0x0,		0x20000000,	0x0,		0x0,		0x00000100	},
	{0x16,	0x0,		0x40000000,	0x0,		0x0,		0x00000200	},
	{0x17,	0x0,		0x80000000,	0x0,		0x0,		0x00000400	},
	{0x18,	0x00000004,	0x0,		0x0,		0x0,		0x0		},
	{0x19,	0x0,		0x00000008,	0x0,		0x0,		0x0		},
	{0x1A,	0x0,		0x0,		0x00000001,	0x0,		0x0		},
	{0x1B,	0x00000220,	0x0,		0x0,		0x0,		0x0		},
	{0x1C,	0x00000220,	0x0,		0x0,		0x0,		0x0		}
};

void clock_gate_switch(uint32_t gate, OnOff on_off) {
	uint32_t gate_register;
	uint32_t gate_register_bits;
	uint32_t reg_num;
	for (reg_num=0; reg_num<NUM_CLOCK_GATE_REGISTERS; reg_num++) {
		switch (reg_num) {
			case 0:
				gate_register = CLOCK + CLOCK_GATES_0;
				gate_register_bits = ClockGateTable[gate].reg_0_bits;
				break;
			case 1:
				gate_register = CLOCK + CLOCK_GATES_1;
				gate_register_bits = ClockGateTable[gate].reg_1_bits;
				break;
			case 2:
				gate_register = CLOCK + CLOCK_GATES_2;
				gate_register_bits = ClockGateTable[gate].reg_2_bits;
				break;
			case 3:
				gate_register = CLOCK + CLOCK_GATES_3;
				gate_register_bits = ClockGateTable[gate].reg_3_bits;
				break;
			case 4:
				gate_register = CLOCK + CLOCK_GATES_4;
				gate_register_bits = ClockGateTable[gate].reg_4_bits;
				break;
			default:
				continue;
		}
		
		uint32_t gate_register_data = GET_REG(gate_register);
		if (on_off == ON) {
			gate_register_data &= ~gate_register_bits;
		} else {
			gate_register_data |= gate_register_bits;
		}
		SET_REG(gate_register, gate_register_data);
	}
}

int clock_set_base_divisor(ClockDivisorCode code) {
	int baseDivisor;
	
	switch(code) {
		case ClockDivisorCode0:
			baseDivisor = CLOCK_BASE_DIVISOR_0;
			break;
		case ClockDivisorCode1:
			baseDivisor = CLOCK_BASE_DIVISOR_1;
			break;
		case ClockDivisorCode2:
			baseDivisor = CLOCK_BASE_DIVISOR_2;
			break;
		case ClockDivisorCode3:
			baseDivisor = CLOCK_BASE_DIVISOR_3;
			break;
		default:
			return -1;
	}
	
	SET_REG(CLOCK + CLOCK_CONFIG0, (GET_REG(CLOCK + CLOCK_CONFIG0) & (~CLOCK_BASE_DIVISOR_MASK)) | baseDivisor);

	return 0;
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
		default:
			return 0;
	}
}

unsigned int clock_get_base_frequency() {
	if (chipid_get_security_epoch() == 1) {
		return CLOCK_SECURITY_EPOCH_1_FREQUENCY;
	} else {
		return CLOCK_SECURITY_EPOCH_0_FREQUENCY;
	}
}

static error_t cmd_frequency(int argc, char** argv)
{
	bufferPrintf("Clock frequency: %d Hz\r\n", clock_get_frequency(FrequencyBaseClock));
	bufferPrintf("Memory frequency: %d Hz\r\n", clock_get_frequency(FrequencyBaseMemory));
	bufferPrintf("Bus frequency: %d Hz\r\n", clock_get_frequency(FrequencyBaseBus));
	bufferPrintf("Unknown frequency: %d Hz\r\n", clock_get_frequency(FrequencyBaseUnknown));
	bufferPrintf("Peripheral frequency: %d Hz\r\n", clock_get_frequency(FrequencyBasePeripheral));
	bufferPrintf("Display frequency: %d Hz\r\n", clock_get_frequency(FrequencyBaseDisplay));
	bufferPrintf("Fixed frequency: %d Hz\r\n", clock_get_frequency(FrequencyBaseFixed));
	bufferPrintf("Timebase frequency: %d Hz\r\n", clock_get_frequency(FrequencyBaseTimebase));
	bufferPrintf("Usbphy frequency: %d Hz\r\n", clock_get_frequency(FrequencyBaseUsbPhy));

	return SUCCESS;
}
COMMAND("frequency", "display clock frequencies", cmd_frequency);
