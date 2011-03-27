#ifndef HW_PMU_H
#define HW_PMU_H

// I2C bus and addresses
#define PMU_I2C_BUS 0

#define PMU_SETADDR 0xE9
#define PMU_GETADDR 0xE8

// Registers
#define PMU_ADC_REG         2
#define PMU_POWERSUPPLY_REG 7
#define PMU_MUXSEL_REG      0x30
#define PMU_ADCVAL_REG      0x31

// Values
#define PMU_POWERSUPPLY_USB       0b01000
#define PMU_POWERSUPPLY_FIREWIRE  0b10000

#endif

