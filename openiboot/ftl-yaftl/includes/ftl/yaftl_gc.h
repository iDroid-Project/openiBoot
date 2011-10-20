#ifndef FTL_YAFTL_GC_H
#define FTL_YAFTL_GC_H

#include "openiboot.h"
#include "util.h"
#include "ftl/l2v.h"
#include "ftl/yaftl_common.h"

/* Types */

#define GCLIST_MAX (16)

typedef struct {
	uint32_t block[GCLIST_MAX + 1];
	uint32_t head;
	uint32_t tail;
} GCList;

typedef struct {
	uint32_t pageIndex;
	uint32_t field_4;
	uint32_t vpn;
	uint32_t* span;
	L2VNode* node;
	uint32_t field_14;
	uint32_t next_nOfs;
	uint32_t nodeSize;
	uint32_t field_20;
	uint32_t field_24;
} GCReadC;

typedef struct {
	uint32_t victim;
	GCList list;
	uint32_t chosenBlock;
	uint32_t totalValidPages;
	uint32_t eraseCount;
	uint32_t* tocPagesBuffer;
	uint8_t* pageBuffer1;
	uint32_t* pageArray;
	uint8_t* pageBuffer2;
	SpareData* spareArray;
	uint32_t pageCount;
	uint32_t field_84;
	uint32_t field_88;
	uint32_t field_8C;
	uint32_t xPagesPerSublk;
	uint32_t field_94;
	GCReadC read_c;
} GCData;

typedef struct {
	GCData data;
	GCData index;
} GC;

/* Functions */

void gcListPushBack(GCList* _gcList, uint32_t _block);

#endif // FTL_YAFTL_GC_H
