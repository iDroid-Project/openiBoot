#ifndef HW_CLOCKS5L8730_H
#define HW_CLOCKS5L8730_H

// Clock constants for the S5L8730. This device does not have a distinction
// between clock0 and clock1 like the S5L8900 does.

#define NUM_PLL 4

/*
typedef struct {
	uint32_t address;
	uint32_t unkn1;
	uint8_t unkn_bytes[4];
} ClockStruct;

typedef struct {
	uint32_t unkn1;
	uint8_t unkn2;
} ClockThirdStruct;

#define CLOCK_REFERENCE_0_FREQUENCY 0x16E3600
#define CLOCK_REFERENCE_1 0xBF100000
#define CLOCK_REFERENCE_2 0xBF100008
#define CLOCK_REFERENCE_3 0xBF100010
#define CLOCK_REFERENCE_4 0xBF100020

#define CLOCK_ACTIVE(x) GET_BITS(GET_REG(x), 31, 1)
*/

#define CLOCK_CPUFREQ 0xBF100040
#define CLOCK_GATE_BASE 0xBF101010
#define CLOCK_GATE_MAX 0x3F

#endif
