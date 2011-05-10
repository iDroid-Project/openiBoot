#include "ftl/yaftl.h"
#include "openiboot.h"
#include "commands.h"
#include "nand.h"
#include "vfl.h"
#include "ftl.h"
#include "mtd.h"
#include "util.h"

static uint32_t yaftl_inited = 0;
static vfl_device_t* vfl = 0;

static void* wmr_malloc(uint32_t size) {
	void* buffer = memalign(0x40, size);

	if(!buffer)
		system_panic("wmr_malloc: failed\r\n");

	memset(buffer, 0, size);

	return buffer;
}

static void WMR_BufZone_Init(WMR_BufZone_t *_zone)
{
	_zone->buffer = 0;
	_zone->endOfBuffer = 0;
	_zone->size = 0;
	_zone->numAllocs = 0;
	_zone->numRebases = 0;
	_zone->paddingsSize = 0;
	_zone->state = 1;
}

// returns the sub-buffer offset
static void* WMR_Buf_Alloc_ForDMA(WMR_BufZone_t *_zone, uint32_t size)
{
	uint32_t oldSizeRounded;

	if (_zone->state != 1)
		system_panic("WMR_Buf_Alloc_ForDMA: bad state\r\n");

	oldSizeRounded = ROUND_UP(_zone->size, 64);
	_zone->paddingsSize = _zone->paddingsSize + (oldSizeRounded - _zone->size);
	_zone->size = oldSizeRounded + size;
	_zone->numAllocs++;

	return (void*)oldSizeRounded;
}

static int WMR_BufZone_FinishedAllocs(WMR_BufZone_t *_zone)
{
	uint8_t* buff;

	if (_zone->state != 1) {
		bufferPrintf("WMR_BufZone_FinishedAllocs: bad state\r\n");
		return -1;
	}

	_zone->size = ROUND_UP(_zone->size, 64);

	buff = wmr_malloc(_zone->size);

	if(!buff) {
		bufferPrintf("WMR_BufZone_FinishedAllocs: No buffer.\r\n");
		return -1;
	}

	_zone->buffer = (uint32_t)buff;
	_zone->endOfBuffer = (uint32_t)(buff + _zone->size);
	_zone->state = 2;

	return 0;
}

static void* WMR_BufZone_Rebase(WMR_BufZone_t *_zone, void *buf)
{
	_zone->numRebases++;
	return buf + _zone->buffer;
}

static int WMR_BufZone_FinishedRebases(WMR_BufZone_t *_zone)
{
	if (_zone->numAllocs != _zone->numRebases) {
		bufferPrintf("WMR_BufZone_FinishedRebases: _zone->numAllocs != _zone->numRebases\r\n");
		return -1;
	}

	return 0;
}

static uint32_t BTOC_Init() {
	yaftl_info.unk74_4 = 4;
	yaftl_info.unk78_counter = 0;
	yaftl_info.unkAC_2 = 2;
	yaftl_info.unkB0_1 = 1;

	yaftl_info.unk8c_buffer = wmr_malloc(0x10);
	yaftl_info.unk88_buffer = wmr_malloc(yaftl_info.unk74_4 << 2);
	yaftl_info.unk84_buffer = wmr_malloc(yaftl_info.unk74_4 << 2);
	yaftl_info.unkB8_buffer = wmr_malloc(yaftl_info.unkAC_2 << 2);
	yaftl_info.unkB4_buffer = wmr_malloc(yaftl_info.unkAC_2 << 2);

	if(!yaftl_info.unk8c_buffer || !yaftl_info.unk8c_buffer || !yaftl_info.unk84_buffer || !yaftl_info.unkB8_buffer || !yaftl_info.unkB4_buffer)
		return -1;

	uint32_t i;
	for(i = 0; i < yaftl_info.unk74_4; i++) {
		yaftl_info.unk88_buffer[i] = -1;
	}

	for(i = 0; i < yaftl_info.unkAC_2; i++) {
		yaftl_info.unkB4_buffer[i] = -1;
		yaftl_info.unkB8_buffer[i] = wmr_malloc(nand_geometry_ftl.pages_per_block_total_banks<<2);
		memset(yaftl_info.unkB8_buffer[i], 0xFF, nand_geometry_ftl.pages_per_block_total_banks<<2);
	}

	yaftl_info.unk7C_byteMask = (1 << yaftl_info.unk74_4) - 1;
	yaftl_info.unk80_3 = 3;

	WMR_BufZone_Init(&yaftl_info.ftl_buffer2);

	for(i = 0; i < yaftl_info.unk74_4; i++) {
		yaftl_info.unk84_buffer[i] = WMR_Buf_Alloc_ForDMA(&yaftl_info.ftl_buffer2, nand_geometry_ftl.bytes_per_page_ftl * yaftl_info.nMetaPages);
	}

	if(WMR_BufZone_FinishedAllocs(&yaftl_info.ftl_buffer2) != 0)
		return -1;

	for(i = 0; i < yaftl_info.unk74_4; i++) {
		yaftl_info.unk84_buffer[i] = WMR_BufZone_Rebase(&yaftl_info.ftl_buffer2, yaftl_info.unk84_buffer[i]);
	}

	if (WMR_BufZone_FinishedRebases(&yaftl_info.ftl_buffer2) != 0)
		return -1;

	return 0;
}

static uint32_t* BTOC_Alloc(uint32_t _arg0, uint32_t _unused_arg1) {
	int32_t inited = 0x7FFFFFFF;
	yaftl_info.unkB0_1 = (yaftl_info.unkB0_1 + 1) % yaftl_info.unkAC_2;
	yaftl_info.unkB4_buffer[yaftl_info.unkB0_1] = _arg0;

	memset(yaftl_info.unkB8_buffer[yaftl_info.unkB0_1], 0xFF, nand_geometry_ftl.pages_per_block_total_banks * sizeof(uint32_t));

	yaftl_info.unk78_counter++;

	uint32_t found = -1;
	uint32_t i;
	for (i = 0; i < yaftl_info.unk74_4; i++) {
		if ((yaftl_info.unk80_3 & (1<<i)) && (yaftl_info.unk7C_byteMask & (1<<i)) && (inited > (int32_t)yaftl_info.unk8c_buffer[i])) {
			found = i;
			inited = yaftl_info.unk8c_buffer[i];
		}
	}

	if(found == -1)
		system_panic("BTOC_Alloc: Couldn't allocate a BTOC.\r\n");

	yaftl_info.unk7C_byteMask &= ~(1<<found);
	yaftl_info.unk88_buffer[found] = _arg0;
	yaftl_info.unk8c_buffer[found] = yaftl_info.unk78_counter;
	return yaftl_info.unk84_buffer[found];
}

static void YAFTL_LoopThingy() {
	uint32_t i;
	for (i = 0; i < yaftl_info.unk74_4; i++) {
		if(yaftl_info.unk88_buffer[i] == ~3)
			yaftl_info.unk88_buffer[i] = yaftl_info.unkn_0x2C;
		if(yaftl_info.unk88_buffer[i] == ~2)
			yaftl_info.unk88_buffer[i] = yaftl_info.unkn_0x3C;
	}
}

static uint32_t YAFTL_readPage(uint32_t _page, uint8_t* _data_ptr, SpareData* _spare_ptr, uint32_t _disable_aes, uint32_t _empty_ok, uint32_t _scrub) {
	int refreshPage = 0, result;
	uint32_t block = _page / nand_geometry_ftl.pages_per_block_total_banks;

	uint8_t* meta_ptr = (uint8_t*)(_spare_ptr ? _spare_ptr : yaftl_info.spareBuffer18);

	result = vfl_read_single_page(vfl, _page, _data_ptr, meta_ptr, _empty_ok, &refreshPage, _disable_aes);

	if (FAILED(result)) {
		bufferPrintf("YAFTL_readPage: We got read failure: page %d, block %d, block status %d, scrub %d.\r\n",
			_page,
			block,
			yaftl_info.blockArray[block].status,
			_scrub);

		return ERROR_ARG;
	}

	yaftl_info.blockArray[block].readCount++;
	return result;
}

static uint32_t YAFTL_readCxtInfo(uint32_t _page, uint8_t* _ptr, uint8_t _extended, uint32_t *pSupported) {
	uint32_t pageToRead;
	uint32_t i = 0, j = 0;

	uint16_t* _ptr16 = (uint16_t*)_ptr;
	uint32_t* _ptr32 = (uint32_t*)_ptr;

	uint32_t leftToRead = 0, perPage = 0;

	if (YAFTL_readPage(_page, _ptr, yaftl_info.spareBuffer5, 0, 1, 0))
		return ERROR_ARG;

	if (!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
		return ERROR_ARG;

	YAFTL_CXT *yaftl_cxt = (YAFTL_CXT*)_ptr;

	if (memcmp(yaftl_cxt->version, "CX01", 4)) {
		if (pSupported)
			*pSupported = 0;

		bufferPrintf("yaftl: Wrong context version.\r\n");
		return ERROR_ARG;
	}

	if (_extended) {
		yaftl_info.nMetaPages = yaftl_cxt->nMetaPages;
		yaftl_info.dwordsPerPage = yaftl_cxt->dwordsPerPage;
		yaftl_info.unknCalculatedValue0 = yaftl_cxt->unknCalculatedValue0;
		yaftl_info.tocArrayLength = yaftl_cxt->tocArrayLength;
		yaftl_info.total_pages_ftl = yaftl_cxt->total_pages_ftl;
		yaftl_info.unkn_0x2A = yaftl_cxt->unkn_0x2A;
		yaftl_info.unkn_0x2C = yaftl_cxt->unkn_0x2C;
		yaftl_info.unkn_0x34 = yaftl_cxt->unkn_0x34;
		yaftl_info.unkn_0x3C = yaftl_cxt->unkn_0x3C;
		yaftl_info.blockStats.field_4 = yaftl_cxt->blockStatsField4;
		yaftl_info.blockStats.field_10 = yaftl_cxt->blockStatsField10;
		yaftl_info.blockStats.numAllocated = yaftl_cxt->numAllocatedBlocks;
		yaftl_info.blockStats.numIAllocated = yaftl_cxt->numIAllocatedBlocks;
		yaftl_info.unk64 = yaftl_cxt->unk64;
		yaftl_info.unk6C = yaftl_cxt->unk6C;
		yaftl_info.unk184_0xA = yaftl_cxt->unk184_0xA;
		yaftl_info.unk188_0x63 = yaftl_cxt->unk188_0x63;

		DebugPrintf("yaftl: reading ftl2_buffer_x\r\n");

		// Read the meta pages into ftl2_buffer_x.
		pageToRead = _page + 1;
		for (i = 0; i < yaftl_info.nMetaPages; i++) {
			if (YAFTL_readPage(pageToRead + i, &yaftl_info.ftl2_buffer_x[nand_geometry_ftl.bytes_per_page_ftl * i], yaftl_info.spareBuffer5, 0, 1, 0) ||
					!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
				return ERROR_ARG;
		}

		DebugPrintf("yaftl: reading ftl2_buffer_x2000\r\n");

		// Read the meta pages into ftl2_buffer_x2000.
		pageToRead += yaftl_info.nMetaPages;
		for (i = 0; i < yaftl_info.nMetaPages; i++) {
			if (YAFTL_readPage(pageToRead + i, &yaftl_info.ftl2_buffer_x2000[nand_geometry_ftl.bytes_per_page_ftl * i], yaftl_info.spareBuffer5, 0, 1, 0) ||
					!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
				return ERROR_ARG;
		}

		DebugPrintf("yaftl: reading tocArray's index pages\r\n");

		// Read tocArray's index pages.
		pageToRead += yaftl_info.nMetaPages + 1;
		leftToRead = yaftl_info.tocArrayLength;
		perPage = nand_geometry_ftl.bytes_per_page_ftl / sizeof(yaftl_info.tocArray->indexPage);

		for (i = 0; i < yaftl_info.nPagesTocPageIndices; i++) {
			if (YAFTL_readPage(pageToRead + i, _ptr, yaftl_info.spareBuffer5, 0, 1, 0) ||
					!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
				return ERROR_ARG;

			for (j = 0; j < leftToRead && j < perPage; j++)
				yaftl_info.tocArray[i * perPage + j].indexPage = _ptr32[j];

			leftToRead -= perPage;
		}

		DebugPrintf("yaftl: reading blockArrays's statuses\r\n");

		// Read blockArray's statuses.
		pageToRead += yaftl_info.nPagesTocPageIndices;
		leftToRead = nand_geometry_ftl.usable_blocks_per_bank;
		perPage = nand_geometry_ftl.bytes_per_page_ftl / sizeof(yaftl_info.blockArray->status);

		for (i = 0; i < yaftl_info.nPagesBlockStatuses; i++) {
			if (YAFTL_readPage(pageToRead + i, _ptr, yaftl_info.spareBuffer5, 0, 1, 0) ||
					!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
				return ERROR_ARG;

			for (j = 0; j < leftToRead && j < perPage; j++)
				yaftl_info.blockArray[i * perPage + j].status = _ptr[j];

			leftToRead -= perPage;
		}
	} else {
		// Skip.
		pageToRead = _page + yaftl_info.nPagesTocPageIndices + 2 + (yaftl_info.nMetaPages * 2);
	}

	DebugPrintf("yaftl: reading blockArrays's read counts\r\n");

	// Read blockArray's readCounts.
	pageToRead += yaftl_info.nPagesBlockStatuses;
	leftToRead = nand_geometry_ftl.usable_blocks_per_bank;
	perPage = nand_geometry_ftl.bytes_per_page_ftl / sizeof(yaftl_info.blockArray->readCount);

	for (i = 0; i < yaftl_info.nPagesBlockReadCounts; i++) {
		if (YAFTL_readPage(pageToRead + i, _ptr, yaftl_info.spareBuffer5, 0, 1, 0) ||
				!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			return ERROR_ARG;

		for (j = 0; j < leftToRead && j < perPage; j++)
			yaftl_info.blockArray[i * perPage + j].readCount = _ptr16[j];

		leftToRead -= perPage;
	}

	DebugPrintf("yaftl: reading blockArrays's unkn0s\r\n");

	// Read blockArray's unkn0s.
	pageToRead += yaftl_info.nPagesBlockReadCounts;
	leftToRead = nand_geometry_ftl.usable_blocks_per_bank;
	perPage = nand_geometry_ftl.bytes_per_page_ftl / sizeof(yaftl_info.blockArray->unkn0);

	for (i = 0; i < yaftl_info.nPagesBlockUnkn0s; i++) {
		if (YAFTL_readPage(pageToRead + i, _ptr, yaftl_info.spareBuffer5, 0, 1, 0) ||
				!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			return ERROR_ARG;

		for (j = 0; j < leftToRead && j < perPage; j++)
			yaftl_info.blockArray[i * perPage + j].unkn0 = _ptr32[j];

		leftToRead -= perPage;
	}

	if (_extended) {

		DebugPrintf("yaftl: reading blockArrays's validPagesINo\r\n");

		// Read blockArray's validPagesINo.
		pageToRead += yaftl_info.nPagesBlockUnkn0s;
		leftToRead = nand_geometry_ftl.usable_blocks_per_bank;
		perPage = nand_geometry_ftl.bytes_per_page_ftl / sizeof(yaftl_info.blockArray->validPagesINo);

		for (i = 0; i < yaftl_info.nPagesBlockValidPagesINumbers; i++) {
			if (YAFTL_readPage(pageToRead+i, _ptr, yaftl_info.spareBuffer5, 0, 1, 0) ||
					!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
				return ERROR_ARG;

			for (j = 0; j < leftToRead && j < perPage; j++) {
				yaftl_info.blockArray[i * perPage + j].validPagesINo = _ptr16[j];
				yaftl_info.blockStats.numValidIPages += yaftl_info.blockArray[i * perPage + j].validPagesINo;
			}

			leftToRead -= perPage;
		}

		DebugPrintf("yaftl: reading blockArrays's validPagesDNo\r\n");

		// Read blockArray's validPagesDNo.
		pageToRead += yaftl_info.nPagesBlockValidPagesINumbers;
		leftToRead = nand_geometry_ftl.usable_blocks_per_bank;
		perPage = nand_geometry_ftl.bytes_per_page_ftl / sizeof(yaftl_info.blockArray->validPagesDNo);

		for (i = 0; i < yaftl_info.nPagesBlockValidPagesDNumbers; i++) {
			if (YAFTL_readPage(pageToRead+i, _ptr, yaftl_info.spareBuffer5, 0, 1, 0) ||
					!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
				return ERROR_ARG;

			for (j = 0; j < leftToRead && j < perPage; j++) {
				yaftl_info.blockArray[i * perPage + j].validPagesDNo = _ptr16[j];
				yaftl_info.blockStats.numValidDPages += yaftl_info.blockArray[i * perPage + j].validPagesDNo;
			}

			leftToRead -= perPage;
		}
	}
	return 0;
}

static uint32_t YAFTL_Init() {
	if(yaftl_inited)
		system_panic("Oh shit, yaftl already initialized!\r\n");

	memset(&yaftl_info, 0, sizeof(yaftl_info)); // 0x358

	yaftl_info.lastTOCPageRead = 0xFFFFFFFF;
	yaftl_info.ftlCxtPage = 0xFFFFFFFF;
	yaftl_info.unk184_0xA = 0xA;
	yaftl_info.unk188_0x63 = 0x63;

	uint16_t metaBytes, unkn20_1;
	error_t error = 0;

	error |= vfl_get_info(vfl, diPagesPerBlockTotalBanks, &nand_geometry_ftl.pages_per_block_total_banks, sizeof(nand_geometry_ftl.pages_per_block_total_banks));
	error |= vfl_get_info(vfl, diSomeThingFromVFLCXT, &nand_geometry_ftl.usable_blocks_per_bank, sizeof(nand_geometry_ftl.usable_blocks_per_bank));
	error |= vfl_get_info(vfl, diBytesPerPageFTL, &nand_geometry_ftl.bytes_per_page_ftl, sizeof(nand_geometry_ftl.bytes_per_page_ftl));
	error |= vfl_get_info(vfl, diTotalBanks, &nand_geometry_ftl.total_banks_ftl, sizeof(nand_geometry_ftl.total_banks_ftl));

	error |= vfl_get_info(vfl, diMetaBytes0xC, &metaBytes, sizeof(metaBytes));
	error |= vfl_get_info(vfl, diUnkn20_1, &unkn20_1, sizeof(unkn20_1));
	nand_geometry_ftl.meta_struct_size_0xC = metaBytes * unkn20_1;

	nand_geometry_ftl.total_usable_pages = nand_geometry_ftl.pages_per_block_total_banks * nand_geometry_ftl.usable_blocks_per_bank;

	if (FAILED(error))
		system_panic("yaftl: vfl get info failed!\r\n");

	if (nand_geometry_ftl.meta_struct_size_0xC != 0xC)
		system_panic("yaftl: MetaStructSize not equal 0xC!\r\n");

	bufferPrintf("yaftl: got information from VFL.\r\n");
	bufferPrintf("pages_per_block_total_banks: %d\r\n", nand_geometry_ftl.pages_per_block_total_banks);
	bufferPrintf("usable_blocks_per_bank: %d\r\n", nand_geometry_ftl.usable_blocks_per_bank);
	bufferPrintf("bytes_per_page_ftl: %d\r\n", nand_geometry_ftl.bytes_per_page_ftl);
	bufferPrintf("total_banks_ftl: %d\r\n", nand_geometry_ftl.total_banks_ftl);
	bufferPrintf("metaBytes: %d\r\n", metaBytes);
	bufferPrintf("unkn20_1: %d\r\n", unkn20_1);
	bufferPrintf("total_usable_pages: %d\r\n", nand_geometry_ftl.total_usable_pages);

	// nMetaPages is the minimum number of pages required to store (number of pages per superblock) dwords.
	yaftl_info.nMetaPages = (nand_geometry_ftl.pages_per_block_total_banks * sizeof(uint32_t)) / nand_geometry_ftl.bytes_per_page_ftl;
	if (((nand_geometry_ftl.pages_per_block_total_banks * sizeof(uint32_t)) % nand_geometry_ftl.bytes_per_page_ftl) != 0)
		yaftl_info.nMetaPages++;

	yaftl_info.dwordsPerPage = nand_geometry_ftl.bytes_per_page_ftl >> 2;
	WMR_BufZone_Init(&yaftl_info.zone);
	yaftl_info.pageBuffer = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, nand_geometry_ftl.bytes_per_page_ftl);
	yaftl_info.tocPageBuffer = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, nand_geometry_ftl.bytes_per_page_ftl);
	yaftl_info.pageBuffer2 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, nand_geometry_ftl.bytes_per_page_ftl * yaftl_info.nMetaPages);
	yaftl_info.spareBuffer3 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer4 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer5 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer6 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer7 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer8 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer9 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer10 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer11 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer12 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer13 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer14 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer15 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer16 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer17 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.spareBuffer18 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, sizeof(SpareData));
	yaftl_info.buffer19 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, nand_geometry_ftl.total_banks_ftl * 0x180);
	yaftl_info.buffer20 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, nand_geometry_ftl.pages_per_block_total_banks * sizeof(SpareData));

	if (WMR_BufZone_FinishedAllocs(&yaftl_info.zone) != 0)
		system_panic("YAFTL_Init: WMR_BufZone_FinishedAllocs failed!");

	yaftl_info.pageBuffer = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.pageBuffer);
	yaftl_info.tocPageBuffer = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.tocPageBuffer);
	yaftl_info.pageBuffer2 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.pageBuffer2);
	yaftl_info.spareBuffer3 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer3);
	yaftl_info.spareBuffer4 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer4);
	yaftl_info.spareBuffer5 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer5);
	yaftl_info.spareBuffer6 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer6);
	yaftl_info.spareBuffer7 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer7);
	yaftl_info.spareBuffer8 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer8);
	yaftl_info.spareBuffer9 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer9);
	yaftl_info.spareBuffer10 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer10);
	yaftl_info.spareBuffer11 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer11);
	yaftl_info.spareBuffer12 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer12);
	yaftl_info.spareBuffer13 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer13);
	yaftl_info.spareBuffer14 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer14);
	yaftl_info.spareBuffer15 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer15);
	yaftl_info.spareBuffer16 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer16);
	yaftl_info.spareBuffer17 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer17);
	yaftl_info.spareBuffer18 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.spareBuffer18);
	yaftl_info.buffer19 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer19);
	yaftl_info.buffer20 = WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer20);

	if (WMR_BufZone_FinishedRebases(&yaftl_info.zone) != 0)
		return -1;

	yaftl_info.unknCalculatedValue0 = CEIL_DIVIDE((nand_geometry_ftl.pages_per_block_total_banks - yaftl_info.nMetaPages) - (nand_geometry_ftl.usable_blocks_per_bank - 8), (nand_geometry_ftl.pages_per_block_total_banks - yaftl_info.nMetaPages) * yaftl_info.dwordsPerPage) * 3;

	yaftl_info.unknCalculatedValue1 = nand_geometry_ftl.usable_blocks_per_bank - yaftl_info.unknCalculatedValue0 - 3;

	yaftl_info.total_pages_ftl = ((nand_geometry_ftl.pages_per_block_total_banks - yaftl_info.nMetaPages) - (nand_geometry_ftl.usable_blocks_per_bank - 8)) - (nand_geometry_ftl.usable_blocks_per_bank * nand_geometry_ftl.pages_per_block_total_banks);

	yaftl_info.tocArrayLength = CEIL_DIVIDE(nand_geometry_ftl.pages_per_block_total_banks * nand_geometry_ftl.usable_blocks_per_bank * 4, nand_geometry_ftl.bytes_per_page_ftl);

	yaftl_info.tocArray = wmr_malloc(yaftl_info.tocArrayLength * sizeof(TOCStruct));
	if (!yaftl_info.tocArray)
		system_panic("No buffer.\r\n");

	yaftl_info.blockArray = wmr_malloc(nand_geometry_ftl.usable_blocks_per_bank * sizeof(BlockStruct));
	if (!yaftl_info.blockArray)
		system_panic("No buffer.\r\n");

	yaftl_info.unknBuffer3_ftl = wmr_malloc(nand_geometry_ftl.pages_per_block_total_banks << 2);
	if (!yaftl_info.unknBuffer3_ftl)
		system_panic("No buffer.\r\n");

	// Initialize number of pages in a context slot.
	yaftl_info.nPagesTocPageIndices = CEIL_DIVIDE(yaftl_info.tocArrayLength * sizeof(yaftl_info.tocArray->indexPage), nand_geometry_ftl.bytes_per_page_ftl);
	yaftl_info.nPagesBlockStatuses = CEIL_DIVIDE(nand_geometry_ftl.usable_blocks_per_bank * sizeof(yaftl_info.blockArray->status), nand_geometry_ftl.bytes_per_page_ftl);
	yaftl_info.nPagesBlockReadCounts = CEIL_DIVIDE(nand_geometry_ftl.usable_blocks_per_bank * sizeof(yaftl_info.blockArray->readCount), nand_geometry_ftl.bytes_per_page_ftl);
	yaftl_info.nPagesBlockValidPagesDNumbers = yaftl_info.nPagesBlockReadCounts;
	yaftl_info.nPagesBlockValidPagesINumbers = yaftl_info.nPagesBlockReadCounts;
	yaftl_info.nPagesBlockUnkn0s = CEIL_DIVIDE(nand_geometry_ftl.usable_blocks_per_bank * sizeof(yaftl_info.blockArray->unkn0), nand_geometry_ftl.bytes_per_page_ftl);

	yaftl_info.ctrlBlockPageOffset =
		yaftl_info.nPagesTocPageIndices
		+ yaftl_info.nPagesBlockStatuses
		+ yaftl_info.nPagesBlockReadCounts
		+ yaftl_info.nPagesBlockUnkn0s
		+ yaftl_info.nPagesBlockValidPagesDNumbers
		+ yaftl_info.nPagesBlockValidPagesINumbers
		+ 2 * yaftl_info.nMetaPages
		+ 2;

	memset(&yaftl_info.blockStats, 0, sizeof(yaftl_info.blockStats));

	if(BTOC_Init())
		system_panic("yaftl: Actually (and before) it should free something, but,.. hey, we don't fuck up, do we?\r\n");

	yaftl_info.ftl2_buffer_x = (uint8_t*)BTOC_Alloc(~2, 1);
	yaftl_info.ftl2_buffer_x2000 = (uint8_t*)BTOC_Alloc(~1, 0);

	yaftl_inited = 1;

	return 0;
}

static uint32_t YAFTL_Restore() {
	bufferPrintf("YAFTL_Restore: Sorry... not yet!\r\n");
	return 0;
}

static void YAFTL_SetupFreeAndAllocd() {
	uint32_t i;

	yaftl_info.blockStats.field_4 = 0;
	yaftl_info.blockStats.numAllocated = 0;
	yaftl_info.blockStats.field_10 = 0;
	yaftl_info.blockStats.numIAllocated = 0;
	yaftl_info.blockStats.numFree = 0;

	for (i = 0; i < nand_geometry_ftl.usable_blocks_per_bank; i++) {
		switch (yaftl_info.blockArray[i].status) {
		case BLOCKSTATUS_ALLOCATED:
		case BLOCKSTATUS_GC:
			yaftl_info.blockStats.numAllocated++;
			break;

		case BLOCKSTATUS_I_ALLOCATED:
		case BLOCKSTATUS_I_GC:
			yaftl_info.blockStats.numIAllocated++;
			break;

		case BLOCKSTATUS_FREE:
			yaftl_info.blockStats.numFree++;
			break;
		}
	}

	yaftl_info.blockStats.field_10 = yaftl_info.unknCalculatedValue0 - yaftl_info.blockStats.numIAllocated;
	yaftl_info.blockStats.field_4 = yaftl_info.blockStats.numFree - yaftl_info.blockStats.field_10;
}

static uint32_t YAFTL_Open(uint32_t signature_bit) {
	uint16_t ftlCtrlBlockBuffer[3];
	uint32_t versionSupported = 1;

	memset(yaftl_info.tocArray, 0xFF, yaftl_info.tocArrayLength * sizeof(TOCStruct));
	memset(yaftl_info.blockArray, 0xFF, nand_geometry_ftl.usable_blocks_per_bank * sizeof(BlockStruct));

	uint32_t i;
	for (i = 0; i < nand_geometry_ftl.usable_blocks_per_bank; i++) {
		yaftl_info.blockArray[i].unkn0 = 0;
		yaftl_info.blockArray[i].validPagesDNo = 0;
		yaftl_info.blockArray[i].validPagesINo = 0;
		yaftl_info.blockArray[i].readCount = 0;
		yaftl_info.blockArray[i].status = 0;
		yaftl_info.blockArray[i].unkn5 = 0;
	}

	memcpy(ftlCtrlBlockBuffer, vfl_get_ftl_ctrl_block(vfl), sizeof(ftlCtrlBlockBuffer));

	if (ftlCtrlBlockBuffer[0] == 0xFFFF) {
		// No FTL ctrl block was found.
		return YAFTL_Restore();
	}

	for (i = 0; i < 3; i++) {
		yaftl_info.FTLCtrlBlock[i] = ftlCtrlBlockBuffer[i];
		yaftl_info.blockArray[ftlCtrlBlockBuffer[i]].status = BLOCKSTATUS_FTLCTRL;
	}

	if (!yaftl_info.pageBuffer) {
		system_panic("yaftl: This can't happen. This shouldn't happen. Whatever, it's doing something else then.\r\n");
		return -1;
	}

	// Find the latest (with max USN) FTL context block.
	uint32_t maxUsn = 0;
	uint32_t ftlCtrlBlock = 0;

	for (i = 0; i < 3; i++) {
		if (YAFTL_readPage(ftlCtrlBlockBuffer[i] * nand_geometry_ftl.pages_per_block_total_banks, yaftl_info.pageBuffer, yaftl_info.spareBuffer6, 0, 1, 0) == 0) {
			if (yaftl_info.spareBuffer6->usn != 0xFFFFFFFF && yaftl_info.spareBuffer6->usn > maxUsn) {
				maxUsn = yaftl_info.spareBuffer6->usn;
				ftlCtrlBlock = ftlCtrlBlockBuffer[i];
			}
		}
	}

	uint32_t some_val;

	if (!maxUsn) {
		yaftl_info.blockArray[ftlCtrlBlockBuffer[0]].status = BLOCKSTATUS_FTLCTRL_SEL;
		some_val = 5;
	} else {
		yaftl_info.blockArray[ftlCtrlBlock].status = BLOCKSTATUS_FTLCTRL_SEL;
		i = 0;
		while (TRUE) {
			if (i >= nand_geometry_ftl.pages_per_block_total_banks - yaftl_info.ctrlBlockPageOffset
				|| YAFTL_readPage(
					nand_geometry_ftl.pages_per_block_total_banks * ftlCtrlBlock + i,
					yaftl_info.pageBuffer,
					yaftl_info.spareBuffer6,
					0, 1, 0) != 0)
			{
				bufferPrintf("yaftl: no valid context slot was found, a restore is definitely needed.\r\n");
				yaftl_info.ftlCxtPage = ~yaftl_info.ctrlBlockPageOffset + nand_geometry_ftl.pages_per_block_total_banks * ftlCtrlBlock + i;
				YAFTL_readCxtInfo(yaftl_info.ftlCxtPage, yaftl_info.pageBuffer, 0, &versionSupported);
				some_val = 5;
				break;
			}

			if (YAFTL_readPage(nand_geometry_ftl.pages_per_block_total_banks * ftlCtrlBlock + i + yaftl_info.ctrlBlockPageOffset, yaftl_info.pageBuffer, yaftl_info.spareBuffer6, 0, 1, 0) == 1) {
				yaftl_info.ftlCxtPage = nand_geometry_ftl.pages_per_block_total_banks * ftlCtrlBlock + i;
				if (YAFTL_readCxtInfo(yaftl_info.ftlCxtPage, yaftl_info.pageBuffer, 1, &versionSupported) != 0)
					some_val = 5;
				else
					some_val = 0;
				break;
			}

			i += yaftl_info.ctrlBlockPageOffset + 1;
		}
	}

	bufferPrintf("yaftl: ftl context is at logical page %d\r\n", yaftl_info.ftlCxtPage);

	if (versionSupported != 1) {
		bufferPrintf("yaftl: unsupported low-level format version.\r\n");
		return -1;
	}

	int restoreNeeded = 0;

	if (some_val || signature_bit) {
		restoreNeeded = 1;
		yaftl_info.unk184_0xA *= 100;
	}

	yaftl_info.tocCaches = wmr_malloc(yaftl_info.unk184_0xA * sizeof(TOCCache));
	if (!yaftl_info.tocCaches) {
		bufferPrintf("yaftl: error allocating memory\r\n");
		return -1;
	}

	WMR_BufZone_Init(&yaftl_info.segment_info_temp);

	for (i = 0; i < yaftl_info.unk184_0xA; i++)
		yaftl_info.tocCaches[i].buffer = WMR_Buf_Alloc_ForDMA(&yaftl_info.segment_info_temp, nand_geometry_ftl.bytes_per_page_ftl);

	if (WMR_BufZone_FinishedAllocs(&yaftl_info.segment_info_temp) != 0)
		return -1;

	for (i = 0; i < yaftl_info.unk184_0xA; i++) {
		yaftl_info.tocCaches[i].buffer = WMR_BufZone_Rebase(&yaftl_info.segment_info_temp, yaftl_info.tocCaches[i].buffer);
		yaftl_info.tocCaches[i].state = 0xFFFF;
		yaftl_info.tocCaches[i].page = 0xFFFF;
		yaftl_info.tocCaches[i].useCount = 0;
		memset(yaftl_info.tocCaches[i].buffer, 0xFF, nand_geometry_ftl.bytes_per_page_ftl);
	}

	if (WMR_BufZone_FinishedRebases(&yaftl_info.segment_info_temp) != 0)
		return -1;

	if (restoreNeeded)
		return YAFTL_Restore();

	yaftl_info.pagesAvailable = yaftl_info.unk188_0x63 * (yaftl_info.total_pages_ftl - 1) / 100;
	yaftl_info.bytesPerPage = nand_geometry_ftl.bytes_per_page_ftl;

	yaftl_info.maxBlockUnkn0 = 0;
	yaftl_info.minBlockUnkn0 = 0xFFFFFFFF;

	for (i = 0; i < nand_geometry_ftl.usable_blocks_per_bank; i++) {
		if (yaftl_info.blockArray[i].status != BLOCKSTATUS_FTLCTRL
			&& yaftl_info.blockArray[i].status != BLOCKSTATUS_FTLCTRL_SEL)
		{
			if (yaftl_info.blockArray[i].unkn0 > yaftl_info.maxBlockUnkn0)
				yaftl_info.maxBlockUnkn0 = yaftl_info.blockArray[i].unkn0;

			if (yaftl_info.blockArray[i].unkn0 < yaftl_info.minBlockUnkn0)
				yaftl_info.minBlockUnkn0 = yaftl_info.blockArray[i].unkn0;
		}
	}

	YAFTL_SetupFreeAndAllocd();
	YAFTL_LoopThingy();

	bufferPrintf("yaftl: successfully opened!\r\n");

	return 0;
}

static int YAFTL_readMultiPages(uint32_t* pagesArray, uint32_t nPages, uint8_t* dataBuffer, SpareData* metaBuffers, uint32_t disableAES, uint32_t scrub) {
	uint32_t block, page, i, j;
	int ret, succeeded, status = 0, unkn = 0;
	int pagesToRead = nPages;

	if (metaBuffers == NULL)
		metaBuffers = yaftl_info.buffer20;

	for (i = 0; pagesToRead > nand_geometry_ftl.pages_per_block_total_banks; i++) {
		//ret = VFL_ReadScatteredPagesInVb(&buf[i * nand_geometry_ftl.pages_per_block_total_banks],nand_geometry_ftl.pages_per_block_total_banks, databuffer + i * nand_geometry_ftl.pages_per_block_total_banks * nand_geometry_ftl.bytes_per_page_ftl, metabuf_array, &unkn, 0, a4, &status);
		ret = 0; // For now. --Oranav

		if (ret) {
			if (status)
				return 0;
		} else {
			//bufferPrintf("yaftl: _readMultiPages: We got read failure!!\r\n");
			succeeded = 1;

			for (j = 0; j < nand_geometry_ftl.pages_per_block_total_banks; j++) {
				page = i * nand_geometry_ftl.pages_per_block_total_banks + j;
				ret = YAFTL_readPage(
						pagesArray[page],
						&dataBuffer[page * nand_geometry_ftl.bytes_per_page_ftl],
						&metaBuffers[page],
						disableAES,
						1,
						scrub);

				if (ret)
					 succeeded = 0;

				status = ret;
			}

			if (!succeeded) {
				bufferPrintf("yaftl: _readMultiPages: We weren't able to overcome read failure.\r\n");
				return 0;
			}

			//bufferPrintf("yaftl:_readMultiPages: We were able to overcome read failure!!\r\n");
		}

		block = pagesArray[(i + 1) * nand_geometry_ftl.pages_per_block_total_banks] / nand_geometry_ftl.pages_per_block_total_banks;
		yaftl_info.blockArray[block].readCount++;

		pagesToRead -= nand_geometry_ftl.pages_per_block_total_banks;
	}

	if (pagesToRead == 0)
		return 1;

	block = pagesArray[i * nand_geometry_ftl.pages_per_block_total_banks] / nand_geometry_ftl.pages_per_block_total_banks;
	yaftl_info.blockArray[block].readCount++;

	unkn = 0;
	// ret = VFL_ReadScatteredPagesInVb(&buf[i * nand_geometry_ftl.pages_per_block_total_banks], pagesToRead % nand_geometry_ftl.pages_per_block_total_banks, databuffer + i * nand_geometry_ftl.pages_per_block_total_banks * nand_geometry_ftl.bytes_per_page_ftl, metabuf_array, &unkn, 0, a4, &status);
	ret = 0; // For now. --Oranav

	if (ret)
		return !status;
	else {
		//bufferPrintf("yaftl:_readMultiPages: We got read failure!!\r\n");
		succeeded = 1;

		for (j = 0; j != pagesToRead; j++) {
			page = i * nand_geometry_ftl.pages_per_block_total_banks + j;
			ret = YAFTL_readPage(
					pagesArray[page],
					&dataBuffer[page * nand_geometry_ftl.bytes_per_page_ftl],
					&metaBuffers[page],
					disableAES,
					1,
					scrub);

			if (ret)
				succeeded = 0;

			status = ret;
		}

		if (!succeeded)
			return 0;

		//bufferPrintf("yaftl:_readMultiPages: We were able to overcome read failure!!\r\n");
	}

	return 1;
}

static int YAFTL_verifyMetaData(uint32_t lpnStart, SpareData* metaDatas, uint32_t nPages)
{
	uint32_t i;
	for (i = 0; i < nPages; i++) {
		if (metaDatas[i].lpn != (lpnStart + i))
		{
			bufferPrintf("YAFTL_Read: mismatch between lpn and metadata!\r\n");
			return 0;
		}

		if (metaDatas[i].type & 0x40)
			return 0;
	}

	return 1;
}

static uint32_t findFreeTOCCache() {
	uint32_t i;
	for (i = 0; i < yaftl_info.unk184_0xA; i++) {
		if (yaftl_info.tocCaches[i].state == 0xFFFF)
			return i;
	}

	return 0xFFFF;
}

static int YAFTL_Read(uint32_t lpn, uint32_t nPages, uint8_t* pBuf)
{
	int ret = 0;
	uint32_t tocPageNum, tocEntry, tocCacheNum, freeTOCCacheNum;
	uint32_t testMode, pagesRead = 0, numPages = 0;
	uint8_t* data = NULL;
	uint8_t* readBuf = pBuf;

	if (!pBuf && nPages != 1)
		return EINVAL;

	if ((lpn + nPages) >= yaftl_info.total_pages_ftl)
		return EINVAL;

	nand_device_set_ftl_region(vfl->get_device(vfl), lpn, 0, nPages, pBuf);

	yaftl_info.lastTOCPageRead = 0xFFFFFFFF;

	// Omitted for now. --Oranav
	// yaftl_info.unk33C = 0;

	testMode = (!pBuf && nPages == 1);

	if (testMode)
		*yaftl_info.unknBuffer3_ftl = 0xFFFFFFFF;

	while (pagesRead != nPages) {
		tocPageNum = (lpn + pagesRead) / yaftl_info.dwordsPerPage;
		if ((yaftl_info.tocArray[tocPageNum].cacheNum == 0xFFFF) && (yaftl_info.tocArray[tocPageNum].indexPage == 0xFFFFFFFF)) {
			if (testMode)
				return 0;

			if (!YAFTL_readMultiPages(yaftl_info.unknBuffer3_ftl, numPages, data, 0, 0, 1)
				|| !YAFTL_verifyMetaData(lpn + (data - pBuf) / nand_geometry_ftl.bytes_per_page_ftl, yaftl_info.buffer20, numPages))
				ret = EIO;

			numPages = 0;
			readBuf += nand_geometry_ftl.bytes_per_page_ftl;
		} else {
			// Should lookup TOC.
			tocCacheNum = yaftl_info.tocArray[tocPageNum].cacheNum;

			if (tocCacheNum != 0xFFFF) {
				// There's a cache for it!
				yaftl_info.tocCaches[tocCacheNum].useCount++;
				tocEntry = yaftl_info.tocCaches[tocCacheNum].buffer[(lpn + pagesRead) % yaftl_info.dwordsPerPage];
			} else {
				// TOC cache isn't valid, find whether we can cache the data.
				if ((freeTOCCacheNum = findFreeTOCCache()) != 0xFFFF)
				{
					// There's a free cache.
					ret = YAFTL_readPage(
							yaftl_info.tocArray[tocPageNum].indexPage,
							(uint8_t*)yaftl_info.tocCaches[freeTOCCacheNum].buffer,
							0, 0, 1, 1);

					if (ret != 0)
						goto YAFTL_READ_RETURN;

					yaftl_info.unkn_0x2A--;
					yaftl_info.tocCaches[freeTOCCacheNum].state = 2;
					yaftl_info.tocCaches[freeTOCCacheNum].useCount = 1;
					yaftl_info.tocCaches[freeTOCCacheNum].page = tocPageNum;
					yaftl_info.tocArray[tocPageNum].cacheNum = freeTOCCacheNum;

					tocEntry = yaftl_info.tocCaches[freeTOCCacheNum].buffer[(lpn + pagesRead) % yaftl_info.dwordsPerPage];
				} else {
					// No free cache. Is this the last TOC page we read?
					if (yaftl_info.lastTOCPageRead != tocPageNum) {
						// No, it isn't. Read it.
						if (YAFTL_readPage(
								yaftl_info.tocArray[tocPageNum].indexPage,
								(uint8_t*)yaftl_info.tocPageBuffer,
								0, 0, 1, 1) != 0)
							goto YAFTL_READ_RETURN;

						yaftl_info.lastTOCPageRead = tocPageNum;
					}

					tocEntry = yaftl_info.tocPageBuffer[(lpn + pagesRead) % yaftl_info.dwordsPerPage];
				}
			}

			// Okay, obtained the TOC entry.
			if (tocEntry == 0xFFFFFFFF) {
				if (testMode)
					return 0;

				if (!YAFTL_readMultiPages(yaftl_info.unknBuffer3_ftl, numPages, data, 0, 0, 1)
					|| !YAFTL_verifyMetaData(lpn + (data - pBuf) / nand_geometry_ftl.bytes_per_page_ftl,
									yaftl_info.buffer20,
									numPages))
					ret = EIO;

				numPages = 0;
				data = NULL;
			} else {
				yaftl_info.unknBuffer3_ftl[numPages] = tocEntry;
				numPages++;

				if (data == NULL)
					data = readBuf;

				if (numPages == nand_geometry_ftl.pages_per_block_total_banks) {
					if (testMode)
						return 0;

					if (!YAFTL_readMultiPages(yaftl_info.unknBuffer3_ftl, numPages, data, 0, 0, 1)
						|| !YAFTL_verifyMetaData(lpn + (data - pBuf) / nand_geometry_ftl.bytes_per_page_ftl,
										yaftl_info.buffer20,
										numPages))
						ret = EIO;

					numPages = 0;
					data = NULL;
				}
			}

			readBuf += nand_geometry_ftl.bytes_per_page_ftl;
		}

		pagesRead++;
	}

	if (numPages == 0)
	{
		if (testMode)
			ret = EIO;

		goto YAFTL_READ_RETURN;
	}

	if (testMode)
		return 0;

	if (!YAFTL_readMultiPages(yaftl_info.unknBuffer3_ftl, numPages, data, 0, 0, 1)
		|| !YAFTL_verifyMetaData(lpn + (data - pBuf) / nand_geometry_ftl.bytes_per_page_ftl,
						yaftl_info.buffer20,
						numPages))
		ret = EIO;

YAFTL_READ_RETURN:
	nand_device_set_ftl_region(vfl->get_device(vfl), 0, 0, 0, 0);
	return ret;
}

static error_t yaftl_read_mtd(mtd_t *_dev, void *_dest, uint32_t _off, int _amt)
{
	uint8_t* curLoc = (uint8_t*) _dest;
	uint32_t block_size = yaftl_info.bytesPerPage;
	int curPage = _off / block_size;
	int toRead = _amt;
	int pageOffset = _off - (curPage * block_size);
	uint8_t* tBuffer = (uint8_t*) malloc(block_size);
	while(toRead > 0) {
		uint32_t ret = YAFTL_Read(curPage, 1, tBuffer);
		if(FAILED(ret)) {
			free(tBuffer);
			return FALSE;
		}

		int read = (((block_size-pageOffset) > toRead) ? toRead : block_size-pageOffset);
		memcpy(curLoc, tBuffer + pageOffset, read);
		curLoc += read;
		toRead -= read;
		pageOffset = 0;
		curPage++;
	}

	free(tBuffer);
	return TRUE;
}

/*
static error_t ftl_write_mtd(mtd_t *_dev, void *_src, uint32_t _off, int _amt)
{
	int ret = ftl_write(_src, _off, _amt);
	if(FAILED(ret))
		return ret;

	return SUCCESS;
}
*/

static int yaftl_block_size(mtd_t *_dev)
{
	return yaftl_info.bytesPerPage;
}

static mtd_t yaftl_mtd = {
	.device = {
		.fourcc = FOURCC('Y', 'F', 'T', 'L'),
		.name = "Apple YAFTL Layer",
	},

	.read = yaftl_read_mtd,
	.write = NULL,

	.block_size = yaftl_block_size,

	.usage = mtd_filesystem,
};

error_t ftl_yaftl_open(ftl_device_t *_ftl, vfl_device_t *_vfl)
{
	vfl = _vfl;

	if(FAILED(YAFTL_Init()))
		return EIO;
	if(FAILED(YAFTL_Open(0)))
		return EIO;

	// FIXME: This is only to avoid warnings.
	yaftl_mtd.write = 0;

	if(!mtd_init(&yaftl_mtd))
		mtd_register(&yaftl_mtd);

	return SUCCESS;
}

error_t ftl_yaftl_read_single_page(ftl_device_t *_ftl, uint32_t _page, uint8_t *_buffer)
{
	error_t ret = YAFTL_Read(_page, 1, _buffer);

	if(FAILED(ret))
		return ret;
	else
		return SUCCESS;
}

error_t ftl_yaftl_device_init(ftl_yaftl_device_t *_ftl)
{
	memset(_ftl, 0, sizeof(*_ftl));

	_ftl->ftl.read_single_page = ftl_yaftl_read_single_page;

	_ftl->ftl.open = ftl_yaftl_open;

	return SUCCESS;
}

ftl_yaftl_device_t *ftl_yaftl_device_allocate()
{
	ftl_yaftl_device_t *ret = malloc(sizeof(ftl_yaftl_device_t));
	ftl_yaftl_device_init(ret);
	return ret;
}
