#ifndef HW_CHIPID_H
#define HW_CHIPID_H

// Device
#define CHIPID 0xBF500000

// Registers
#define SPICLOCKTYPE 0x4

// Values
#define GET_SPICLOCKTYPE(x) GET_BITS(x, 24, 4)
#define CHIPID_GET_GPIO(x) GET_BITS((x), 4, 2)
#define CHIPID_GET_POWER_EPOCH(x) GET_BITS((x), 9, 7)

#endif

