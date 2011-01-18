#ifndef A4_H
#define A4_H

/*
 *	Constants
 */

#define MemoryStart 0x00000000
#define MemoryEnd 0xFFFFFFFF
#define RAMStart 0x40000000
#if defined(CONFIG_IPHONE_4)
#define LargeMemoryStart 0x60000000
#define RAMEnd 0x60000000
#else
#define LargeMemoryStart 0x50000000
#define RAMEnd 0x50000000
#endif
#define MemoryHigher 0xC0000000
#define ExceptionVector MemoryStart
#ifdef SMALL
#define PageTable (OpenIBootLoad + 0x24000)
#define HeapStart (PageTable + 0x4000)
#else
#define OpenIBootLoad 0x00000000
#define GeneralStack (PageTable - 4)
#define HeapStart (LargeMemoryStart + 0x02000000)
#define PageTable (RAMEnd - 0x8000)
#endif

/*
 *	Devices
 */

#define PeripheralPort 0x38000000
#define AMC0 0x84000000
#define AMC0End 0x84400000
#define AMC0Higher 0x84C00000
#define AMC0HigherEnd 0x85000000

#define WDT_CTRL 0x3E300000
#define WDT_CNT 0x3E300004

#define WDT_INT 0x33
/*
 *	Values
 */

#define EDRAM_CLOCKGATE 0x1B
#define WDT_ENABLE 0x100000
#define WDT_PRE_SHIFT 16
#define WDT_PRE_MASK 0xF
#define WDT_CS_SHIFT 12
#define WDT_CS_MASK 0x7
#define WDT_CLR 0xA00
#define WDT_DIS 0xA5
#define WDT_INT_EN 0x8000

#define DMA_ALIGN 0x40

#endif

