#ifndef S5L8900_H
#define S5L8900_H

#include "hardware/s5l8900.h"

/*
 *	Constants
 */

#define MemoryStart 0x00000000
#define MemoryEnd 0xFFFFFFFF
#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
#define LargeMemoryStart 0x08000000				/* FIXME: This is an ugly hack to get around iBoot's memory rearrangement. Linux boot will only work for installed openiboot! */
#define RAMEnd 0x08000000
#define MemoryHigher 0x80000000
#else
#define LargeMemoryStart 0x60000000
#define RAMStart 0x40000000
#define RAMEnd 0x60000000
#define MemoryHigher 0xC0000000
#endif
#define ExceptionVector MemoryStart
#ifdef SMALL
#define PageTable (OpenIBootLoad + 0x24000)
#define HeapStart (PageTable + 0x4000)
#else
#define OpenIBootLoad 0x00000000
#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
#define GeneralStack ((PageTable - 4) + LargeMemoryStart)
#else
#define GeneralStack (PageTable - 4)
#endif
#define HeapStart (LargeMemoryStart + 0x02000000)
#if !defined(CONFIG_IPHONE_4) && !defined(CONFIG_IPAD)
#define PageTable (RAMEnd - 0x4000)
#else
#define PageTable (RAMEnd - 0x8000)
#endif
#endif

/*
 *	Devices
 */

#define PeripheralPort 0x38000000
#if defined(CONFIG_IPHONE_4) || defined(CONFIG_IPAD)
#define AMC0 0x84000000
#define AMC0End 0x84400000
#define AMC0Higher 0x84C00000
#define AMC0HigherEnd 0x85000000
#else
#ifdef CONFIG_3G
#define AMC0 0x38500000
#define ROM 0x50000000
#else
#define AMC0 0x22000000
#define ROM 0x20000000
#endif
#endif

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

