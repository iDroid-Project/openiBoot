#ifndef HW_POWER_H
#define HW_POWER_H

// Device
#define POWER 0x39700000	/* probably a part of the system controller */

// Power
#define POWER_DEFAULT_DEVICES 0xEC
#define POWER_LCD 0x100
#define POWER_USB 0x200
#define POWER_VROM 0x1000

// Registers
#define POWER_CONFIG0 0x0
#define POWER_CONFIG1 0x20
#define POWER_CONFIG2 0x6C
#define POWER_CONFIG3 0x74
#define POWER_ONCTRL 0xC
#define POWER_OFFCTRL 0x10
#define POWER_SETSTATE 0x8
#define POWER_STATE 0x14
#define POWER_ID 0x44
#define POWER_ID_EPOCH(x) ((x) >> 24)

// Values
#define POWER_CONFIG0_RESET 0x1021002
#define POWER_CONFIG2_RESET 0x0
#define POWER_CONFIG3_RESET_DIVIDER 1000000

#endif

