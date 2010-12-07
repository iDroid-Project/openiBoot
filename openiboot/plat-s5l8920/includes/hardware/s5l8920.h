#ifndef A4_H
#define A4_H

// LOL HAX
#ifndef MMU_SECTION_SIZE
#define MMU_SECTION_SIZE 0x100000
#endif

/*
 *	Constants
 */

#define MemoryStart 0x00000000
#define MemoryEnd 0xFFFFFFFF
#define LargeMemoryStart 0x60000000

#define RAMStart 	0x40000000
#define RAMEnd		(RAMStart + (0x100*(MMU_SECTION_SIZE)))
#define HiMemStart 	0xC0000000
#define HiMemEnd	(HiMemStart + (0x400*(MMU_SECTION_SIZE)))

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

#define AMC0Map			0x84000000
#define AMC0Start		0x84800000
#define AMC0End			(AMC0Start + (0x8*(MMU_SECTION_SIZE)))

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

