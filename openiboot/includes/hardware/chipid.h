#ifndef HW_CHIPID_H
#define HW_CHIPID_H

#include "hardware/s5l8900.h"

// Device
#ifndef CONFIG_IPHONE_4
#define CHIPID 0x3E500000
#else
#define CHIPID 0xBF500000
#endif

// Registers
#define SPICLOCKTYPE 0x4

// Values
#define GET_SPICLOCKTYPE(x) GET_BITS(x, 24, 4)
#ifdef CONFIG_IPHONE_4
#define CHIPID_GET_POWER_EPOCH(x) GET_BITS((x), 9, 7)
#else
#define CHIPID_GET_POWER_EPOCH(x) (0)
#endif

#endif

