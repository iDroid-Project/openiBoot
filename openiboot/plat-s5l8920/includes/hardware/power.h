#ifndef HW_POWER_H
#define HW_POWER_H

// Device
#define POWER 0xBF100000

// Power
#define POWER_DEFAULT_DEVICES 0xEC
#define POWER_LCD 0x100
#define POWER_USB 0x200
#define POWER_VROM 0x1000

// Registers
#define POWER_CONFIG0 0x0
#define POWER_CONFIG1 0x20
#define POWER_CONFIG2 0x6C
#define POWER_ONCTRL 0xC
#define POWER_OFFCTRL 0x10
#define POWER_SETSTATE 0x8
#define POWER_STATE 0x78
#define POWER_STATE_MAX 0x33
#define POWER_ID 0x158
#define POWER_ID_EPOCH(x) (x)

// Values
#define POWER_CONFIG0_RESET 0x1123009
#define POWER_CONFIG1_RESET 0x20
#define POWER_CONFIG2_RESET 0x0

void power_ctrl_many(uint64_t _gates, OnOff _val);

#endif

