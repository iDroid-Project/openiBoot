#ifndef FTL_YAFTL_H
#define FTL_YAFTL_H

#include "ftl.h"
#include "vfl.h"
#include "nand.h"
#include "openiboot.h"
#include "ftl/yaftl_common.h"
#include "ftl/yaftl_mem.h"
#include "ftl/yaftl_gc.h"

typedef struct _ftl_yaftl_device {
	ftl_device_t ftl;
} ftl_yaftl_device_t;

ftl_yaftl_device_t *ftl_yaftl_device_allocate();

// YAFTL errors
#define ERROR_ARG	0x80000001
#define ERROR_NAND	0x80000002
#define ERROR_EMPTY	0x80000003

// Block status (as defined in the BlockStruct structure)
#define BLOCKSTATUS_ALLOCATED		(0x1)
#define BLOCKSTATUS_FTLCTRL			(0x2)
#define BLOCKSTATUS_GC				(0x4)
#define BLOCKSTATUS_CURRENT			(0x8)
#define BLOCKSTATUS_FTLCTRL_SEL		(0x10)
#define BLOCKSTATUS_I_GC			(0x20)
#define BLOCKSTATUS_I_ALLOCATED		(0x40)
#define BLOCKSTATUS_I_CURRENT		(0x80)
#define BLOCKSTATUS_FREE			(0xFF)

// Cache states
#define CACHESTATE_DIRTY	(0x1)
#define CACHESTATE_CLEAN	(0x2)
#define CACHESTATE_FREE		(0xFFFF)

typedef struct {
	uint32_t eraseCount;
	uint16_t validPagesDNo;			// Data validity counter
	uint16_t validPagesINo;			// Index validity counter
	uint16_t readCount;				// Number of reads
	uint8_t status;					// Block status
	uint8_t unkn5;
} BlockStruct;

typedef struct {
	uint32_t indexPage;
	uint16_t cacheNum;
	uint16_t TOCUnkMember2;
} __attribute__((packed)) TOCStruct;

typedef struct {
	uint32_t* buffer;
	uint32_t page;
	uint16_t useCount;
	uint16_t state;
} __attribute__((packed)) TOCCache;

typedef struct {
	uint32_t numAllocated; // 00
	uint32_t field_4; // 04
	uint32_t numValidDPages; // 08
	uint32_t numIAllocated; // 0C
	uint32_t field_10; // 10
	uint32_t numValidIPages; // 14
	uint32_t numFree; // 18
	uint32_t field_1C; // 1C
} __attribute__((packed)) BlockStats;

typedef struct {
	uint32_t blockNum;
	uint32_t* tocBuffer;
	uint32_t usedPages;
	uint16_t field_A;
	uint32_t usn;
} BlockToUse;

typedef struct {
	bufzone_t zone;
	bufzone_t segment_info_temp;
	uint16_t tocPagesPerBlock; // 38
	uint16_t tocEntriesPerPage; // 3A
	uint32_t unknCalculatedValue0; // 3C
	uint32_t unknCalculatedValue1; // 40
	uint32_t totalPages; // 44
	uint16_t tocArrayLength; // 48
	uint16_t unkn_0x2A; // 4A
	BlockToUse latestUserBlk;
	BlockToUse latestIndexBlk;
	uint32_t selCtrlBlockIndex;
	uint32_t maxIndexUsn; // 6C
	uint32_t field_A4;
	uint8_t totalEraseCount;
	uint8_t field_78; // Heh, wtf?
	uint32_t unk74_4;
	uint32_t unk78_counter;
	uint32_t unk7C_byteMask;
	uint32_t unk80_3;
	uint32_t** unk84_buffer;
	uint32_t* unk88_buffer;
	uint32_t* unk8c_buffer;
	bufzone_t ftl_buffer2;
	uint32_t unkAC_2;
	uint32_t unkB0_1;
	uint32_t* unkB4_buffer;
	uint8_t** unkB8_buffer;
	BlockStats blockStats; // BC
	uint32_t field_DC[4]; // DC
	TOCStruct* tocArray; // EC
	TOCCache* tocCaches; // F0
	BlockStruct* blockArray; // F4
	uint32_t* pageBuffer2;
	SpareData* spareBuffer3;
	SpareData* spareBuffer4;
	SpareData* spareBuffer5;
	SpareData* spareBuffer6;
	SpareData* spareBuffer7;
	SpareData* spareBuffer8;
	SpareData* spareBuffer9;
	SpareData* spareBuffer10;
	SpareData* spareBuffer11;
	SpareData* spareBuffer12;
	SpareData* spareBuffer13;
	SpareData* spareBuffer14;
	SpareData* spareBuffer15;
	SpareData* spareBuffer16;
	SpareData* spareBuffer17;
	uint8_t* pageBuffer;
	uint32_t* tocPageBuffer;
	uint32_t lastTOCPageRead;
	SpareData* spareBuffer19;
	uint32_t* unknBuffer3_ftl; // 148
	SpareData* buffer20;
	SpareData* spareBuffer18;
	uint32_t ctrlBlockPageOffset;
	uint32_t nPagesTocPageIndices;
	uint32_t nPagesBlockStatuses;
	uint32_t nPagesBlockReadCounts;
	uint32_t nPagesBlockEraseCounts;
	uint32_t nPagesBlockValidPagesDNumbers;
	uint32_t nPagesBlockValidPagesINumbers;
	uint32_t ftlCxtPage;
	uint16_t FTLCtrlBlock[3];
	uint16_t unkn_1;
	uint32_t maxBlockEraseCount;
	uint32_t minBlockEraseCount;
	uint32_t unk184_0xA;
	uint8_t unk188_0x63;
	uint32_t pagesAvailable;
	uint32_t bytesPerPage;
	GC gc;
} YAFTLInfo;

typedef struct {
	uint16_t pagesPerSublk;
	uint16_t numBlocks;
	uint16_t bytesPerPage;
	uint16_t spareDataSize;
	uint16_t total_banks_ftl;
	uint32_t total_usable_pages;
} NAND_GEOMETRY_FTL;

typedef struct {
	char version[4]; // 0
	uint32_t unknCalculatedValue0; // 4
	uint32_t totalPages; // 8
	uint32_t latestUserBlk; // C
	uint32_t cxt_unkn0; // 10 // placeholder
	uint32_t latestIndexBlk; // 14
	uint32_t maxIndexUsn; // 18
	uint32_t blockStatsField4; // 1C
	uint32_t blockStatsField10; // 20
	uint32_t numAllocatedBlocks; // 24
	uint32_t numIAllocatedBlocks; // 28
	uint32_t unk184_0xA; // 2C
	uint32_t cxt_unkn1[10]; // placeholder
	uint32_t field_58; // 58
	uint16_t tocArrayLength; // 5C
	uint16_t tocPagesPerBlock; // 5E
	uint16_t tocEntriesPerPage; // 60
	uint16_t unkn_0x2A; // 62
	uint16_t pagesUsedInLatestUserBlk; // 66
	uint16_t pagesUsedInLatestIdxBlk; // 68
	uint32_t cxt_unkn2[11]; // placeholder
	uint8_t unk188_0x63; // 94
} __attribute__((packed)) YAFTL_CXT;

typedef struct _BlockListNode {
	uint32_t usn;
	uint16_t blockNumber;
	uint16_t field_6;
	struct _BlockListNode *next;
	struct _BlockListNode *prev;
} __attribute__((packed)) BlockListNode;

typedef struct {
	uint32_t lpnMin;
	uint32_t lpnMax;
} __attribute__((packed)) BlockLpn;

#endif // FTL_YAFTL_H
