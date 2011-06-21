#ifndef HW_GPIO_H
#define HW_GPIO_H

// gpioBaseAddress
#define GPIO 0xBFA00000

// Interrupt
#define GPIO_INTERRUPT 0x74

// Registers
#define GPIO_INTLEVEL 0x80
#define GPIO_INTSTAT 0xA0
#define GPIO_INTEN 0xC0
#define GPIO_INTTYPE 0xE0
#define GPIO_FSEL 0x320

// Values
#define GPIO_NUMINTGROUPS 7
#define GPIO_INTSTAT_RESET 0xFFFFFFFF
#define GPIO_INTEN_RESET 0

#define GPIO_FSEL_MAJSHIFT 16
#define GPIO_FSEL_MAJMASK 0x1F
#define GPIO_FSEL_MINSHIFT 8
#define GPIO_FSEL_MINMASK 0x7
#define GPIO_FSEL_USHIFT 0
#define GPIO_FSEL_UMASK 0xF

#define GPIO_CLOCKGATE 0x2C

#endif

