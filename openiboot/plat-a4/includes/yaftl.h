#ifndef  YAFTL_H
#define  YAFTL_H

#include "openiboot.h"

// YAFTL errors
#define ERROR_ARG	0x80000001
#define ERROR_NAND	0x80000002
#define ERROR_EMPTY	0x80000003

// Page types (as defined in the spare data "type" bitfield)
#define PAGETYPE_INDEX		0x4		// Index block indicator
#define PAGETYPE_LBN		0x10	// User data (also called lbn: maybe logical block number? lBlock 0 is system and lBlock 1 is user?)
#define PAGETYPE_FTL_CLEAN	0x20	// FTL context (unmounted, clean)
// 0x40: VFL context?

// Block status (as defined in the BlockStruct structure)
#define BLOCKSTATUS_ALLOCATED		0x1
#define BLOCKSTATUS_FTLCTRL			0x2
#define BLOCKSTATUS_GC				0x4
#define BLOCKSTATUS_CURRENT			0x8
#define BLOCKSTATUS_FTLCTRL_SEL		0x10
#define BLOCKSTATUS_I_GC			0x20
#define BLOCKSTATUS_I_ALLOCATED		0x40
#define BLOCKSTATUS_I_CURRENT		0x80
#define BLOCKSTATUS_FREE			0xFF

typedef struct
{
  uint32_t buffer;
  uint32_t endOfBuffer;
  uint32_t size;
  uint32_t numAllocs;
  uint32_t numRebases;
  uint32_t paddingsSize;
  uint32_t state;
} WMR_BufZone_t;

typedef struct {
	void* buffer;
	uint32_t field_4;
	uint16_t field_8;
	uint16_t field_A;
} __attribute__((packed)) UnknownStruct;

typedef struct {
	uint32_t lpn;			// Logical page number
	uint32_t usn;			// Update sequence number
	uint8_t  field_8;
	uint8_t  type;			// Page type
	uint16_t field_A;
} __attribute__((packed)) SpareData;

typedef struct {
	uint32_t unkn0;
	uint16_t validPagesDNo;			// Data validity counter
	uint16_t validPagesINo;			// Index validity counter
	uint16_t readCount;				// Number of reads
	uint8_t status;					// Block status
	uint8_t unkn5;
} BlockStruct;

typedef struct {
	uint32_t indexPage;
	uint32_t TOCUnkMember;
} __attribute__((packed)) TOCStruct;

typedef struct {
	WMR_BufZone_t zone;
	WMR_BufZone_t segment_info_temp;
	uint16_t nMetaPages; // (0x38) Number of meta pages. Similar to pagesToRead in the old FTL.
	uint16_t dwordsPerPage; // 3A
	uint32_t unknCalculatedValue0; // 3C
	uint32_t unknCalculatedValue1; // 40
	uint32_t total_pages_ftl; // 44
	uint16_t tocArrayLength; // 48
	uint16_t unkn_0x2A; // 4A
	uint32_t unkn_0x2C; // 4C
	uint8_t* ftl2_buffer_x;
	uint16_t unkn_0x34; // 54
	uint32_t unkn_0x3C; // 5C
	uint8_t* ftl2_buffer_x2000;
	uint32_t unk64; // 64
	uint32_t unk6C; // 6C
	uint32_t unk74_4;
	uint32_t unk78_counter;
	uint32_t unk7C_byteMask;
	uint32_t unk80_3;
	uint32_t** unk84_buffer;
	uint32_t* unk88_buffer;
	uint32_t* unk8c_buffer;
	WMR_BufZone_t ftl_buffer2;
	uint32_t unkAC_2;
	uint32_t unkB0_1;
	uint32_t* unkB4_buffer;
	uint8_t** unkB8_buffer;
	uint32_t unkStruct_ftl[10]; // BC
	TOCStruct* tocArray; // EC
	UnknownStruct* unknStructArray; // F0
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
	uint32_t* pageBuffer1;
	uint32_t unk140_n1;
	uint32_t* buffer19;
	uint32_t* unknBuffer3_ftl; // 148
	uint32_t* buffer20;
	SpareData* spareBuffer18;
	uint32_t ctrlBlockPageOffset;
	uint32_t nPagesTocPageIndices;
	uint32_t nPagesBlockStatuses;
	uint32_t nPagesBlockReadCounts;
	uint32_t nPagesBlockUnkn0s;
	uint32_t nPagesBlockValidPagesDNumbers;
	uint32_t nPagesBlockValidPagesINumbers;
	uint32_t ftlCxtPage;
	uint16_t FTLCtrlBlock[3];
	uint16_t unkn_1;
	uint32_t maxBlockUnkn0;
	uint32_t minBlockUnkn0;
	uint32_t unk184_0xA;
	uint8_t unk188_0x63;
} YAFTL_INFO;
YAFTL_INFO yaftl_info;

typedef struct {
	uint16_t pages_per_block_total_banks;
	uint16_t usable_blocks_per_bank;
	uint16_t bytes_per_page_ftl;
	uint16_t meta_struct_size_0xC;
	uint16_t total_banks_ftl;
	uint32_t total_usable_pages;
} NAND_GEOMETRY_FTL;
NAND_GEOMETRY_FTL nand_geometry_ftl;

typedef struct {
	char version[4]; // 0
	uint32_t unknCalculatedValue0; // 4
	uint32_t total_pages_ftl; // 8
	uint32_t unkn_0x2C; // C
	uint32_t cxt_unkn0; // 10 // placeholder
	uint32_t unkn_0x3C; // 14
	uint32_t unk6C; // 18
	uint32_t unkStruct_ftl_1; // 1C
	uint32_t unkStruct_ftl_4; // 20
	uint32_t unkStruct_ftl_0; // 24
	uint32_t unkStruct_ftl_3; // 28
	uint32_t unk184_0xA; // 2C
	uint32_t cxt_unkn1[10]; // placeholder
	uint16_t tocArrayLength; // 5C
	uint16_t nMetaPages; // 5E
	uint16_t dwordsPerPage; // 60
	uint16_t unkn_0x2A; // 62
	uint16_t unkn_0x34; // 66
	uint16_t unk64; // 68
	uint32_t cxt_unkn2[11]; // placeholder
	uint8_t unk188_0x63; // 94
} __attribute__((packed)) YAFTL_CXT;

#endif //YAFTL_H
