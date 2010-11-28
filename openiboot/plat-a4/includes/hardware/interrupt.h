#ifndef HW_INTERRUPT_H
#define HW_INTERRUPT_H

// This appears to be a PL192

// Constants

#define VIC_MaxInterrupt 0x80
#define VIC_InterruptSeparator 0x20

// Devices

#define VIC0 0xBF200000
#define VIC1 0xBF210000
#define VIC2 0xBF220000
#define VIC3 0xBF230000

// Registers

#define VICIRQSTATUS 0x000
#define VICRAWINTR 0x8
#define VICINTSELECT 0xC
#define VICINTENABLE 0x10
#define VICINTENCLEAR 0x14
#define VICSWPRIORITYMASK 0x24
#define VICVECTADDRS 0x100
#define VICADDRESS 0xF00
#define VICPERIPHID0 0xFE0
#define VICPERIPHID1 0xFE4
#define VICPERIPHID2 0xFE8
#define VICPERIPHID3 0xFEC

#endif

