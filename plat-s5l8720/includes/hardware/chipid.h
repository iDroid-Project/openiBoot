#ifndef HW_CHIPID_H
#define HW_CHIPID_H

// Device
#define CHIPID 0x3D100000

// Registers
#define SPICLOCKTYPE 0x4
#define CHIPINFO 0x8

// Values
#define GET_SPICLOCKTYPE(x) GET_BITS(x, 24, 4)
#define CHIPID_GET_SECURITY_EPOCH(x) (GET_BITS((x), 0, 1))
#define CHIPID_GET_POWER_EPOCH(x) (GET_BITS((x), 4, 7) > 3 ? 3 : 4)

#endif

