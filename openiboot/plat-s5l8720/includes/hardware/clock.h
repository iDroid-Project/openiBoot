#ifndef HW_CLOCK_H
#define HW_CLOCK_H

// Clock constants for the S5L8720. This device does not have a distinction
// between clock0 and CLOCK like the S5L8900 does.

// Constants
#define NUM_PLL 3
#define CLOCK_SECURITY_EPOCH_1_FREQUENCY 24000000
#define CLOCK_SECURITY_EPOCH_0_FREQUENCY 12000000
#define CLOCK_STATIC_INFREQ 27000000

// Devices
#define CLOCK 0x3C500000

// Registers
#define CLOCK_CONFIG0 0x0
#define CLOCK_CONFIG1 0x4
#define CLOCK_CONFIG2 0x8
#define CLOCK_PLL0CON 0x20
#define CLOCK_PLL1CON 0x24
#define CLOCK_PLL2CON 0x28
#define CLOCK_PLL0LCNT 0x30
#define CLOCK_PLL1LCNT 0x34
#define CLOCK_PLL2LCNT 0x38
#define CLOCK_PLLLOCK 0x40
#define CLOCK_PLLMODE 0x44
#define CLOCK_GATES_0 0x48
#define CLOCK_GATES_1 0x4C
#define CLOCK_GATES_2 0x58
#define CLOCK_GATES_3 0x68
#define CLOCK_GATES_4 0x6C

// Values
// Config 1
#define CLOCK_CLOCKPLL(x) (GET_BITS((x), 12, 2) - 1)
#define CLOCK_CLOCKHASDIVIDER(x) GET_BITS((x), 30, 1)
#define CLOCK_CLOCKDIVIDER(x) ((GET_BITS((x), 25, 5) + 1) << 1)

#define CLOCK_BUSPLL(x) (GET_BITS((x), 12, 2) - 1)
#define CLOCK_BUSHASDIVIDER(x) GET_BITS((x), 22, 1)
#define CLOCK_BUSDIVIDER(x) ((GET_BITS((x), 17, 5) + 1) << 1)

#define CLOCK_PERIPHERALPLL(x) (GET_BITS((x), 12, 2) - 1)
#define CLOCK_PERIPHERALHASDIVIDER(x) GET_BITS((x), 14, 1)
#define CLOCK_PERIPHERALDIVIDER(x) ((GET_BITS((x), 9, 5) + 1) << 1)

#define CLOCK_MEMORYPLL(x) (GET_BITS((x), 12, 2) - 1)
#define CLOCK_MEMORYHASDIVIDER(x) GET_BITS((x), 6, 1)
#define CLOCK_MEMORYDIVIDER(x) ((GET_BITS((x), 1, 5) + 1) << 1)

// Config 2
#define CLOCK_UNKNOWNPLL(x) (GET_BITS((x), 28, 2) - 1) 
#define CLOCK_UNKNOWNDIVIDER(x) (GET_BITS((x), 16, 4) + 1)

#define CLOCK_DISPLAYPLL(x) (GET_BITS((x), 12, 2) - 1)
#define CLOCK_DISPLAYDIVIDER(x) ((GET_BITS((x), 4, 4) + 1) * (GET_BITS((x), 0, 4) + 1))

// PLL mode
#define CLOCK_PLLMODE_ONOFF(x, y) (((x) >> (y)) & 0x1)
#define CLOCK_PLLMODE_USE_SECURITY_EPOCH_INFREQ(x, y) (!(((x) >> (y + 4)) & 0x1))

#define CLOCK_MDIV(x) (((x) >> 8) & 0xFF)
#define CLOCK_PDIV(x) (((x) >> 24) & 0x3F)
#define CLOCK_SDIV(x) ((x) & 0x7)

// Clock gates
#define NUM_CLOCK_GATE_REGISTERS 5
typedef struct {
	uint32_t id;
	uint32_t reg_0_bits;
	uint32_t reg_1_bits;
	uint32_t reg_2_bits;
	uint32_t reg_3_bits;
	uint32_t reg_4_bits;
} ClockGate;

// Base clock divisor
#define CLOCK_BASE_DIVISOR_MASK 0xf
#define CLOCK_BASE_DIVISOR_0 0x1
#define CLOCK_BASE_DIVISOR_1 0x5
#define CLOCK_BASE_DIVISOR_2 0xB
#define CLOCK_BASE_DIVISOR_3 0x4

#endif

