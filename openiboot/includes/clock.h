#ifndef CLOCK_H
#define CLOCK_H

#include "openiboot.h"
#include "hardware/clock.h"

typedef enum ClockDivisorCode {
	ClockDivisorCode0 = 0,
	ClockDivisorCode1 = 1,
	ClockDivisorCode2 = 2,
	ClockDivisorCode3 = 3
} ClockDivisorCode;

extern uint32_t ClockPLL;
extern uint32_t PLLFrequencies[NUM_PLL];
extern uint32_t CalculatedFrequencyTable[55];
extern uint32_t ClockSDiv;
extern uint32_t TicksPerSec;

typedef enum FrequencyBase {
	FrequencyBaseClock,
	FrequencyBaseMemory,
	FrequencyBaseBus,
	FrequencyBasePeripheral,
	FrequencyBaseUnknown,
	FrequencyBaseDisplay,
	FrequencyBaseFixed,
	FrequencyBaseTimebase,
	FrequencyBaseUsbPhy,
	FrequencyNand,
} FrequencyBase;

int clock_set_base_divisor(ClockDivisorCode code);
error_t clock_setup();

error_t clock_set_divisor(uint32_t _clock, uint32_t _div);
uint32_t clock_get_divisor(uint32_t _clock);

void clock_gate_switch(uint32_t gate, OnOff on_off);
void clock_gate_reset(uint32_t gate);

uint32_t clock_get_frequency(FrequencyBase freqBase);
void clock_set_sdiv(int sdiv);
unsigned int clock_get_base_frequency();
void set_CPU_clockconfig(uint32_t _mode);

#endif
