#ifndef HW_PMU_H
#define HW_PMU_H

// I2C bus and addresses
#define PMU_I2C_BUS 0

#define PMU_SETADDR 0xE9
#define PMU_GETADDR 0xE8

// Registers
#define PMU_ADC_REG           0x2
#define PMU_POWERSUPPLY_REG   0x7
#define PMU_OOCSHDWN_REG      0x12
#define PMU_MUXSEL_REG        0x30
#define PMU_ADCVAL_REG        0x31
#define PMU_VOLTAGE_HIGH_REG  0x8E
#define PMU_VOLTAGE_LOW_REG   0x8D

// Unknown registers
#define PMU_UNK1_REG      0xC
#define PMU_UNK2_REG      0x1
#define PMU_UNKREG_START  0x50
#define PMU_UNKREG_END    0x5A

// Values
#define PMU_POWERSUPPLY_USB       0x8
#define PMU_POWERSUPPLY_FIREWIRE  0x10

#define PMU_CHARGER_IDENTIFY_MAX   2
#define PMU_CHARGER_IDENTIFY_NONE  0
#define PMU_CHARGER_IDENTIFY_DP    1
#define PMU_CHARGER_IDENTIFY_DN    2

#define PMU_ADCMUX_USBCHARGER  6

#endif

