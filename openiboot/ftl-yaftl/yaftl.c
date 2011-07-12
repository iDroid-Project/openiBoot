#include "ftl/yaftl.h"
#include "openiboot.h"
#include "commands.h"
#include "nand.h"
#include "vfl.h"
#include "ftl.h"
#include "mtd.h"
#include "bdev.h"
#include "util.h"
#include "h2fmi.h"

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
		yaftl_info.unkB8_buffer[i] = wmr_malloc(ftlGeometry.pagesPerSublk<<2);
		memset(yaftl_info.unkB8_buffer[i], 0xFF, ftlGeometry.pagesPerSublk<<2);
	}

	yaftl_info.unk7C_byteMask = (1 << yaftl_info.unk74_4) - 1;
	yaftl_info.unk80_3 = 3;

	WMR_BufZone_Init(&yaftl_info.ftl_buffer2);

	for(i = 0; i < yaftl_info.unk74_4; i++) {
		yaftl_info.unk84_buffer[i] = WMR_Buf_Alloc_ForDMA(&yaftl_info.ftl_buffer2, ftlGeometry.bytesPerPage * yaftl_info.tocPagesPerBlock);
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

	memset(yaftl_info.unkB8_buffer[yaftl_info.unkB0_1], 0xFF, ftlGeometry.pagesPerSublk * sizeof(uint32_t));

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
		system_panic("yaftl: couldn't allocate a BTOC.\r\n");

	yaftl_info.unk7C_byteMask &= ~(1<<found);
	yaftl_info.unk88_buffer[found] = _arg0;
	yaftl_info.unk8c_buffer[found] = yaftl_info.unk78_counter;
	return yaftl_info.unk84_buffer[found];
}

static void YAFTL_LoopThingy() {
	uint32_t i;
	for (i = 0; i < yaftl_info.unk74_4; i++) {
		if(yaftl_info.unk88_buffer[i] == ~3)
			yaftl_info.unk88_buffer[i] = yaftl_info.latestUserBlock;
		if(yaftl_info.unk88_buffer[i] == ~2)
			yaftl_info.unk88_buffer[i] = yaftl_info.latestIndexBlock;
	}
}

static void YAFTL_setupCleanSpare(SpareData* _spare_ptr) {
	_spare_ptr->usn = ++yaftl_info.maxIndexUsn;
	_spare_ptr->type = PAGETYPE_FTL_CLEAN;
}

static uint32_t YAFTL_writeIndexTOCBuffer() {
	int result;

	YAFTL_setupCleanSpare((SpareData*)yaftl_info.spareBuffer7);
	result = vfl_write_single_page(vfl, yaftl_info.ctrlBlockPageOffset + yaftl_info.ftlCxtPage, (uint8_t*)yaftl_info.indexTOCBuffer, (uint8_t*)yaftl_info.spareBuffer7, 1);

	if (FAILED(result)) {
		vfl_erase_single_block(vfl, yaftl_info.FTLCtrlBlock[yaftl_info.selCtrlBlockIndex], 1);
		yaftl_info.blockArray[yaftl_info.FTLCtrlBlock[yaftl_info.selCtrlBlockIndex]].eraseCount++;
		yaftl_info.field_70++;
		yaftl_info.blockArray[yaftl_info.FTLCtrlBlock[yaftl_info.selCtrlBlockIndex++]].status = 2;
		if (yaftl_info.selCtrlBlockIndex > 2)
			yaftl_info.selCtrlBlockIndex = 0;
		yaftl_info.blockArray[yaftl_info.FTLCtrlBlock[yaftl_info.selCtrlBlockIndex]].status = 16;
		vfl_erase_single_block(vfl, yaftl_info.FTLCtrlBlock[yaftl_info.selCtrlBlockIndex], 1);
		yaftl_info.blockArray[yaftl_info.FTLCtrlBlock[yaftl_info.selCtrlBlockIndex]].eraseCount++;
		yaftl_info.field_70++;
		yaftl_info.ftlCxtPage = -1;
	}

	return result;
}

uint32_t YAFTL_writePage(uint32_t _page, uint8_t* _data_ptr, SpareData* _spare_ptr) {
	int result;
	uint32_t block = _page / ftlGeometry.pagesPerSublk;

	if (yaftl_info.field_78)
	{
		YAFTL_writeIndexTOCBuffer();
		yaftl_info.field_78 = 0;
	}

	result = vfl_write_single_page(vfl, _page, _data_ptr, (uint8_t*)_spare_ptr, 1);

	if (FAILED(result)) {
		bufferPrintf("YAFTL_writePage: We got write failure: page %d, block %d, block status %d.\r\n",
			_page,
			block,
			yaftl_info.blockArray[block].status);

		return ERROR_ARG;
	}

	return result;
}

static uint32_t YAFTL_readPage(uint32_t _page, uint8_t* _data_ptr, SpareData* _spare_ptr, uint32_t _disable_aes, uint32_t _empty_ok, uint32_t _scrub) {
	int refreshPage = 0, result;
	uint32_t block = _page / ftlGeometry.pagesPerSublk;

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
		yaftl_info.tocPagesPerBlock = yaftl_cxt->tocPagesPerBlock;
		yaftl_info.tocEntriesPerPage = yaftl_cxt->tocEntriesPerPage;
		yaftl_info.unknCalculatedValue0 = yaftl_cxt->unknCalculatedValue0;
		yaftl_info.tocArrayLength = yaftl_cxt->tocArrayLength;
		yaftl_info.totalPages = yaftl_cxt->totalPages;
		yaftl_info.unkn_0x2A = yaftl_cxt->unkn_0x2A;
		yaftl_info.latestUserBlock = yaftl_cxt->latestUserBlock;
		yaftl_info.userPagesPerBlock = yaftl_cxt->userPagesPerBlock;
		yaftl_info.latestIndexBlock = yaftl_cxt->latestIndexBlock;
		yaftl_info.blockStats.field_4 = yaftl_cxt->blockStatsField4;
		yaftl_info.blockStats.field_10 = yaftl_cxt->blockStatsField10;
		yaftl_info.blockStats.numAllocated = yaftl_cxt->numAllocatedBlocks;
		yaftl_info.blockStats.numIAllocated = yaftl_cxt->numIAllocatedBlocks;
		yaftl_info.unk64 = yaftl_cxt->unk64;
		yaftl_info.maxIndexUsn = yaftl_cxt->maxIndexUsn;
		yaftl_info.unk184_0xA = yaftl_cxt->unk184_0xA;
		yaftl_info.unk188_0x63 = yaftl_cxt->unk188_0x63;

		DebugPrintf("yaftl: reading userTOCBuffer\r\n");

		// Read the user TOC into userTOCBuffer.
		pageToRead = _page + 1;
		for (i = 0; i < yaftl_info.tocPagesPerBlock; i++) {
			if (YAFTL_readPage(pageToRead + i, &((uint8_t*)yaftl_info.userTOCBuffer)[ftlGeometry.bytesPerPage * i], yaftl_info.spareBuffer5, 0, 1, 0) ||
					!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			{
				yaftl_info.field_78 = 0;
				return ERROR_ARG;
			}
		}

		DebugPrintf("yaftl: reading indexTOCBuffer\r\n");

		// Read the index TOC into indexTOCBuffer.
		pageToRead += yaftl_info.tocPagesPerBlock;
		for (i = 0; i < yaftl_info.tocPagesPerBlock; i++) {
			if (YAFTL_readPage(pageToRead + i, &((uint8_t*)yaftl_info.indexTOCBuffer)[ftlGeometry.bytesPerPage * i], yaftl_info.spareBuffer5, 0, 1, 0) ||
					!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			{
				yaftl_info.field_78 = 0;
				return ERROR_ARG;
			}
		}

		DebugPrintf("yaftl: reading tocArray's index pages\r\n");

		// Read tocArray's index pages.
		pageToRead += yaftl_info.tocPagesPerBlock + 1;
		leftToRead = yaftl_info.tocArrayLength;
		perPage = ftlGeometry.bytesPerPage / sizeof(yaftl_info.tocArray->indexPage);

		for (i = 0; i < yaftl_info.nPagesTocPageIndices; i++) {
			if (YAFTL_readPage(pageToRead + i, _ptr, yaftl_info.spareBuffer5, 0, 1, 0) ||
					!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			{
				yaftl_info.field_78 = 0;
				return ERROR_ARG;
			}

			for (j = 0; j < leftToRead && j < perPage; j++)
				yaftl_info.tocArray[i * perPage + j].indexPage = _ptr32[j];

			leftToRead -= perPage;
		}

		DebugPrintf("yaftl: reading blockArrays's statuses\r\n");

		// Read blockArray's statuses.
		pageToRead += yaftl_info.nPagesTocPageIndices;
		leftToRead = ftlGeometry.numBlocks;
		perPage = ftlGeometry.bytesPerPage / sizeof(yaftl_info.blockArray->status);

		for (i = 0; i < yaftl_info.nPagesBlockStatuses; i++) {
			if (YAFTL_readPage(pageToRead + i, _ptr, yaftl_info.spareBuffer5, 0, 1, 0) ||
					!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			{
				yaftl_info.field_78 = 0;
				return ERROR_ARG;
			}

			for (j = 0; j < leftToRead && j < perPage; j++)
				yaftl_info.blockArray[i * perPage + j].status = _ptr[j];

			leftToRead -= perPage;
		}
	} else {
		// Skip.
		pageToRead = _page + yaftl_info.nPagesTocPageIndices + 2 + (yaftl_info.tocPagesPerBlock * 2);
	}

	DebugPrintf("yaftl: reading blockArrays's read counts\r\n");

	// Read blockArray's readCounts.
	pageToRead += yaftl_info.nPagesBlockStatuses;
	leftToRead = ftlGeometry.numBlocks;
	perPage = ftlGeometry.bytesPerPage / sizeof(yaftl_info.blockArray->readCount);

	for (i = 0; i < yaftl_info.nPagesBlockReadCounts; i++) {
		if (YAFTL_readPage(pageToRead + i, _ptr, yaftl_info.spareBuffer5, 0, 1, 0) ||
				!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
		{
			yaftl_info.field_78 = 0;
			return ERROR_ARG;
		}

		for (j = 0; j < leftToRead && j < perPage; j++)
			yaftl_info.blockArray[i * perPage + j].readCount = _ptr16[j];

		leftToRead -= perPage;
	}

	DebugPrintf("yaftl: reading blockArrays's eraseCounts\r\n");

	// Read blockArray's eraseCounts.
	pageToRead += yaftl_info.nPagesBlockReadCounts;
	leftToRead = ftlGeometry.numBlocks;
	perPage = ftlGeometry.bytesPerPage / sizeof(yaftl_info.blockArray->eraseCount);

	for (i = 0; i < yaftl_info.nPagesBlockEraseCounts; i++) {
		if (YAFTL_readPage(pageToRead + i, _ptr, yaftl_info.spareBuffer5, 0, 1, 0) ||
				!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
		{
			yaftl_info.field_78 = 0;
			return ERROR_ARG;
		}

		for (j = 0; j < leftToRead && j < perPage; j++)
			yaftl_info.blockArray[i * perPage + j].eraseCount = _ptr32[j];

		leftToRead -= perPage;
	}

	if (_extended) {

		DebugPrintf("yaftl: reading blockArrays's validPagesINo\r\n");

		// Read blockArray's validPagesINo.
		pageToRead += yaftl_info.nPagesBlockEraseCounts;
		leftToRead = ftlGeometry.numBlocks;
		perPage = ftlGeometry.bytesPerPage / sizeof(yaftl_info.blockArray->validPagesINo);

		for (i = 0; i < yaftl_info.nPagesBlockValidPagesINumbers; i++) {
			if (YAFTL_readPage(pageToRead+i, _ptr, yaftl_info.spareBuffer5, 0, 1, 0) ||
					!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			{
				yaftl_info.field_78 = 0;
				return ERROR_ARG;
			}

			for (j = 0; j < leftToRead && j < perPage; j++) {
				yaftl_info.blockArray[i * perPage + j].validPagesINo = _ptr16[j];
				yaftl_info.blockStats.numValidIPages += yaftl_info.blockArray[i * perPage + j].validPagesINo;
			}

			leftToRead -= perPage;
		}

		DebugPrintf("yaftl: reading blockArrays's validPagesDNo\r\n");

		// Read blockArray's validPagesDNo.
		pageToRead += yaftl_info.nPagesBlockValidPagesINumbers;
		leftToRead = ftlGeometry.numBlocks;
		perPage = ftlGeometry.bytesPerPage / sizeof(yaftl_info.blockArray->validPagesDNo);

		for (i = 0; i < yaftl_info.nPagesBlockValidPagesDNumbers; i++) {
			if (YAFTL_readPage(pageToRead+i, _ptr, yaftl_info.spareBuffer5, 0, 1, 0) ||
					!(yaftl_info.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			{
				yaftl_info.field_78 = 0;
				return ERROR_ARG;
			}

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

	error |= vfl_get_info(vfl, diPagesPerBlockTotalBanks, &ftlGeometry.pagesPerSublk, sizeof(ftlGeometry.pagesPerSublk));
	error |= vfl_get_info(vfl, diSomeThingFromVFLCXT, &ftlGeometry.numBlocks, sizeof(ftlGeometry.numBlocks));
	error |= vfl_get_info(vfl, diBytesPerPageFTL, &ftlGeometry.bytesPerPage, sizeof(ftlGeometry.bytesPerPage));
	error |= vfl_get_info(vfl, diTotalBanks, &ftlGeometry.total_banks_ftl, sizeof(ftlGeometry.total_banks_ftl));

	error |= vfl_get_info(vfl, diMetaBytes0xC, &metaBytes, sizeof(metaBytes));
	error |= vfl_get_info(vfl, diUnkn20_1, &unkn20_1, sizeof(unkn20_1));
	ftlGeometry.spareDataSize = metaBytes * unkn20_1;

	ftlGeometry.total_usable_pages = ftlGeometry.pagesPerSublk * ftlGeometry.numBlocks;

	if (FAILED(error))
		system_panic("yaftl: vfl get info failed!\r\n");

	if (ftlGeometry.spareDataSize != 0xC)
		system_panic("yaftl: spareDataSize isn't 0xC!\r\n");

	bufferPrintf("yaftl: got information from VFL.\r\n");
	bufferPrintf("pagesPerSublk: %d\r\n", ftlGeometry.pagesPerSublk);
	bufferPrintf("numBlocks: %d\r\n", ftlGeometry.numBlocks);
	bufferPrintf("bytesPerPage: %d\r\n", ftlGeometry.bytesPerPage);
	bufferPrintf("total_banks_ftl: %d\r\n", ftlGeometry.total_banks_ftl);
	bufferPrintf("metaBytes: %d\r\n", metaBytes);
	bufferPrintf("unkn20_1: %d\r\n", unkn20_1);
	bufferPrintf("total_usable_pages: %d\r\n", ftlGeometry.total_usable_pages);

	yaftl_info.tocPagesPerBlock = (ftlGeometry.pagesPerSublk * sizeof(uint32_t)) / ftlGeometry.bytesPerPage;
	if (((ftlGeometry.pagesPerSublk * sizeof(uint32_t)) % ftlGeometry.bytesPerPage) != 0)
		yaftl_info.tocPagesPerBlock++;

	yaftl_info.tocEntriesPerPage = ftlGeometry.bytesPerPage / sizeof(uint32_t);
	WMR_BufZone_Init(&yaftl_info.zone);
	yaftl_info.pageBuffer = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, ftlGeometry.bytesPerPage);
	yaftl_info.tocPageBuffer = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, ftlGeometry.bytesPerPage);
	yaftl_info.pageBuffer2 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, ftlGeometry.bytesPerPage * yaftl_info.tocPagesPerBlock);
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
	yaftl_info.buffer19 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, ftlGeometry.total_banks_ftl * 0x180);
	yaftl_info.buffer20 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, ftlGeometry.pagesPerSublk * sizeof(SpareData));

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

	yaftl_info.unknCalculatedValue0 = CEIL_DIVIDE((ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock) - (ftlGeometry.numBlocks - 8), (ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock) * yaftl_info.tocEntriesPerPage) * 3;

	yaftl_info.unknCalculatedValue1 = ftlGeometry.numBlocks - yaftl_info.unknCalculatedValue0 - 3;

	yaftl_info.totalPages =
		(ftlGeometry.numBlocks - 8) * (ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock)
		- yaftl_info.unknCalculatedValue0 * ftlGeometry.pagesPerSublk;

	yaftl_info.tocArrayLength = CEIL_DIVIDE(ftlGeometry.pagesPerSublk * ftlGeometry.numBlocks * 4, ftlGeometry.bytesPerPage);

	yaftl_info.tocArray = wmr_malloc(yaftl_info.tocArrayLength * sizeof(TOCStruct));
	if (!yaftl_info.tocArray)
		system_panic("No buffer.\r\n");

	yaftl_info.blockArray = wmr_malloc(ftlGeometry.numBlocks * sizeof(BlockStruct));
	if (!yaftl_info.blockArray)
		system_panic("No buffer.\r\n");

	yaftl_info.unknBuffer3_ftl = wmr_malloc(ftlGeometry.pagesPerSublk << 2);
	if (!yaftl_info.unknBuffer3_ftl)
		system_panic("No buffer.\r\n");

	// Initialize number of pages in a context slot.
	yaftl_info.nPagesTocPageIndices = CEIL_DIVIDE(yaftl_info.tocArrayLength * sizeof(yaftl_info.tocArray->indexPage), ftlGeometry.bytesPerPage);
	yaftl_info.nPagesBlockStatuses = CEIL_DIVIDE(ftlGeometry.numBlocks * sizeof(yaftl_info.blockArray->status), ftlGeometry.bytesPerPage);
	yaftl_info.nPagesBlockReadCounts = CEIL_DIVIDE(ftlGeometry.numBlocks * sizeof(yaftl_info.blockArray->readCount), ftlGeometry.bytesPerPage);
	yaftl_info.nPagesBlockValidPagesDNumbers = yaftl_info.nPagesBlockReadCounts;
	yaftl_info.nPagesBlockValidPagesINumbers = yaftl_info.nPagesBlockReadCounts;
	yaftl_info.nPagesBlockEraseCounts = CEIL_DIVIDE(ftlGeometry.numBlocks * sizeof(yaftl_info.blockArray->eraseCount), ftlGeometry.bytesPerPage);

	yaftl_info.field_70 = 0;
	yaftl_info.field_78 = 0;

	yaftl_info.ctrlBlockPageOffset =
		yaftl_info.nPagesTocPageIndices
		+ yaftl_info.nPagesBlockStatuses
		+ yaftl_info.nPagesBlockReadCounts
		+ yaftl_info.nPagesBlockEraseCounts
		+ yaftl_info.nPagesBlockValidPagesDNumbers
		+ yaftl_info.nPagesBlockValidPagesINumbers
		+ 2 * yaftl_info.tocPagesPerBlock
		+ 2;

	memset(&yaftl_info.blockStats, 0, sizeof(yaftl_info.blockStats));

	if(BTOC_Init())
		system_panic("yaftl: Actually (and before) it should free something, but,.. hey, we don't fuck up, do we?\r\n");

	yaftl_info.userTOCBuffer = BTOC_Alloc(~2, 1);
	yaftl_info.indexTOCBuffer = BTOC_Alloc(~1, 0);

	yaftl_inited = 1;

	return 0;
}

static void YAFTL_SetupFreeAndAllocd() {
	uint32_t i;

	yaftl_info.blockStats.field_4 = 0;
	yaftl_info.blockStats.numAllocated = 0;
	yaftl_info.blockStats.field_10 = 0;
	yaftl_info.blockStats.numIAllocated = 0;
	yaftl_info.blockStats.numFree = 0;

	for (i = 0; i < ftlGeometry.numBlocks; i++) {
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

static uint32_t findFreeTOCCache() {
	uint32_t i;
	for (i = 0; i < yaftl_info.unk184_0xA; i++) {
		if (yaftl_info.tocCaches[i].state == 0xFFFF)
			return i;
	}

	return 0xFFFF;
}

void copyBTOC(uint32_t *dest, uint32_t *src, uint32_t maxVal) {
	uint32_t i;
	uint32_t val;

	for (i = 0; i < (yaftl_info.tocPagesPerBlock * ftlGeometry.bytesPerPage) / sizeof(uint32_t); i++) {
		val = src[i];
		if (val >= maxVal)
			val = 0xFFFFFFFF;

		dest[i] = val;
	}
}

static BlockListNode *addBlockToList(BlockListNode *listHead, uint32_t blockNumber, uint32_t usn) {
	BlockListNode *block = wmr_malloc(sizeof(BlockListNode));

	if (!block)
		return NULL;

	block->usn = usn;
	block->next = NULL;
	block->prev = NULL;
	block->blockNumber = blockNumber;

	// If the list isn't empty, insert our block.
	// We sort the blocks by their USN, in descending order,
	// so we will consider the newest blocks first when we restore.
	if (listHead) {
		BlockListNode *curr;
		for (curr = listHead; curr->next && usn < curr->usn; curr = curr->next);

		if (usn < curr->usn) {
			// Our node is the last.
			block->prev = curr;
			curr->next = block;
		} else {
			// Insert our node.
			BlockListNode *prev = curr->prev;

			if (usn == curr->usn)
				bufferPrintf("yaftl: found two blocks (%d, %d) with the same usn.\r\n", blockNumber, curr->blockNumber);

			block->next = curr;
			block->prev = prev;
			curr->prev = block;

			if (prev)
				prev->next = block;
			else
				listHead = block;
		}
	} else {
		listHead = block;
	}

	return listHead;
}

static uint32_t readBTOCPages(uint32_t offset, uint32_t *data, SpareData *spare, uint8_t disable_aes, uint8_t scrub, uint32_t max) {
	uint32_t i, j, result;
	uint8_t *rawBuff = (uint8_t*)data;

	// Read all the BTOC (block table of contents) pages.
	for (i = 0; i < yaftl_info.tocPagesPerBlock; i++) {
		result = YAFTL_readPage(offset + i,
								&rawBuff[i * ftlGeometry.bytesPerPage],
								spare, disable_aes, 1, scrub);

		if (result)
			return result;
	}

	// Validate the data.
	for (j = 0; j < (yaftl_info.tocPagesPerBlock * ftlGeometry.bytesPerPage) / sizeof(uint32_t); j++) {
		if (data[j] >= max)
			data[j] = 0xFFFFFFFF;
	}

	return 0;
}

static uint32_t restoreIndexBlock(uint16_t blockNumber, uint8_t first) {
	uint32_t i;
	uint32_t result, readSucceeded;
	uint32_t blockOffset = blockNumber * ftlGeometry.pagesPerSublk;
	uint32_t *BTOCBuffer = yaftl_info.pageBuffer2;
	SpareData *spare = yaftl_info.spareBuffer3;

	result = YAFTL_readPage(blockOffset, (uint8_t*)yaftl_info.tocPageBuffer, spare, 0, 1, 0);

	if (!result && !(spare->type & PAGETYPE_INDEX)) {
		bufferPrintf("yaftl: restoreIndexBlock called with a non-index block %d.\r\n", blockNumber);
		return 0;
	}

	// Read all the block-table-of-contents pages of this block.
	result = readBTOCPages(blockOffset + ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock,
							BTOCBuffer, spare, 0, 0, yaftl_info.tocArrayLength);

	if (result > 1) {
		// Read failed.
		readSucceeded = 0;
		yaftl_info.blockArray[blockNumber].unkn5 = 0x80;
	} else {
		readSucceeded = 1;
	}

	if (first) {
		// The first block has the largest USN.
		yaftl_info.latestIndexBlock = blockNumber;
		yaftl_info.blockArray[blockNumber].status = BLOCKSTATUS_I_GC;
	} else {
		yaftl_info.blockArray[blockNumber].status = BLOCKSTATUS_I_ALLOCATED;
	}

	// If we read non-empty pages, and they are indeed BTOC:
	if (result == 0 && (spare->type & 0xC) == 0xC) {
		// Loop through the index pages in this block, and fill the information according to the BTOC.
		for (i = 0; i < ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock; i++) {
			uint32_t index = ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock - i - 1;

			if (BTOCBuffer[index] < yaftl_info.tocArrayLength
				&& yaftl_info.tocArray[BTOCBuffer[index]].indexPage == 0xFFFFFFFF) {
				// Valid index page number, and the data is missing. Fill in the information.
				yaftl_info.tocArray[BTOCBuffer[index]].indexPage = blockOffset + index;
				++yaftl_info.blockArray[blockNumber].validPagesINo;
				++yaftl_info.blockStats.numValidIPages;
			}
		}

		if (first) {
			// If this is the latest index block (i.e., the first in the list),
			// updsate indexTOCBuffer, and maxIndexUsn.
			yaftl_info.unk64 = ftlGeometry.pagesPerSublk;
			copyBTOC(yaftl_info.indexTOCBuffer, BTOCBuffer, yaftl_info.tocArrayLength);

			if (spare->usn > yaftl_info.maxIndexUsn)
				yaftl_info.maxIndexUsn = spare->usn;
		}

		return 0;
	}

	// Else - reading the BTOC has failed. Must go through each page.
	if (first) {
		yaftl_info.unk64 = ftlGeometry.pagesPerSublk;

		if (!readSucceeded)
			yaftl_info.unk64 -= yaftl_info.tocPagesPerBlock;
	}

	// Loop through each non-BTOC page (i.e., index page) in this block.
	for (i = 0; i < ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock; i++) {
		uint32_t page = blockOffset
						+ ftlGeometry.pagesPerSublk
						- yaftl_info.tocPagesPerBlock
						- i - 1;

		result = YAFTL_readPage(page, (uint8_t*)yaftl_info.tocPageBuffer, spare, 0, 1, 0);

		uint32_t lpn = spare->lpn;

		if (result > 1 || (!result && (!(spare->type & PAGETYPE_INDEX) || lpn >= yaftl_info.tocArrayLength))) {
			// Read failed, or non-empty page with invalid data:
			// Not marked as index, or lpn is too high.
			yaftl_info.blockArray[blockNumber].unkn5 = 0x80;
			readSucceeded = 0;
			continue;
		}

		if (lpn != 0xFFFFFFFF) {
			// Valid LPN.
			if (first) {
				// First (most updated) index block - update indexTOCBuffer and maxIndexUsn.
				yaftl_info.indexTOCBuffer[page - blockOffset] = lpn;

				if (spare->usn > yaftl_info.maxIndexUsn)
					yaftl_info.maxIndexUsn = spare->usn;
			}

			if (yaftl_info.tocArray[lpn].indexPage == 0xFFFFFFFF) {
				// The information about this index page is missing - fill it in.
				yaftl_info.tocArray[lpn].indexPage = page;
				++yaftl_info.blockArray[blockNumber].validPagesINo;
				++yaftl_info.blockStats.numValidIPages;
			}

			// WTF? It must be another variable then. --Oranav
			readSucceeded = 0;
			continue;
		}

		if (first && readSucceeded)
			yaftl_info.unk64 = page - blockOffset;
	}

	return 0;
}

static uint32_t restoreUserBlock(uint32_t blockNumber, uint32_t *bootBuffer, uint8_t firstBlock, uint32_t lpnOffset, uint32_t bootLength, BlockLpn *blockLpns) {
	uint8_t readSucceeded;
	uint32_t result, i;
	uint32_t blockOffset = blockNumber * ftlGeometry.pagesPerSublk;
	uint32_t *buff = yaftl_info.pageBuffer2;
	SpareData *spare = yaftl_info.spareBuffer4;

	// If the lpn offset is off-boundaries, or this block is full of valid index pages - don't restore.
	if ((lpnOffset && (lpnOffset > blockLpns[blockNumber].lpnMax || blockLpns[blockNumber].lpnMin - lpnOffset >= bootLength))
		|| yaftl_info.blockArray[blockNumber].validPagesINo == ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock) {
		return 0;
	}

	if (lpnOffset && firstBlock) {
		memset(
			&yaftl_info.userTOCBuffer[yaftl_info.userPagesPerBlock],
			0xFF,
			(ftlGeometry.pagesPerSublk - yaftl_info.userPagesPerBlock) * sizeof(uint32_t));

		copyBTOC(buff, yaftl_info.userTOCBuffer, yaftl_info.totalPages);

		result = 0;
		spare->type = 0x8;
		readSucceeded = 1;
	} else {
		result = readBTOCPages(blockOffset + ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock, buff, spare, 0, 0, yaftl_info.totalPages);
		if (result > 1) {
			// Read failed.
			yaftl_info.blockArray[blockNumber].unkn5 = 0x80;
			readSucceeded = 0;
		} else {
			readSucceeded = 1;
		}
	}

	if (!lpnOffset) {
		// First time we're in this block.
		if (firstBlock) {
			yaftl_info.latestUserBlock = blockNumber;
			yaftl_info.blockArray[blockNumber].status = BLOCKSTATUS_GC;
		} else {
			yaftl_info.blockArray[blockNumber].status = BLOCKSTATUS_ALLOCATED;
		}
	}

	if (result == 0 && (spare->type & 0xC) == 0x8) {
		// BTOC reading succeeded, process the information.
		uint32_t nUserPages;

		if (lpnOffset && firstBlock)
			nUserPages = yaftl_info.userPagesPerBlock;
		else
			nUserPages = ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock;

		for (i = 0; i < nUserPages; i++) {
			uint32_t lpn = buff[nUserPages - 1 - i];

			if (lpn >= yaftl_info.totalPages)
				lpn = 0xFFFFFFFF;

			if (lpnOffset) {
				uint32_t lpnMax = blockLpns[blockNumber].lpnMax;
				uint32_t lpnMin = blockLpns[blockNumber].lpnMin;

				if (lpnMax != lpnMin || lpnMax != 0xFFFFFFFF) {
					if (lpnMax < lpn)
						blockLpns[blockNumber].lpnMax = lpn;
					if (lpnMin > lpn)
						blockLpns[blockNumber].lpnMin = lpn;
				} else {
					blockLpns[blockNumber].lpnMax = lpn;
					blockLpns[blockNumber].lpnMin = lpn;
				}
			}

			if (lpnOffset <= lpn && lpn - lpnOffset < bootLength) {
				++yaftl_info.blockArray[blockNumber].validPagesINo;

				if (lpn != 0xFFFFFFFF && bootBuffer[lpn - lpnOffset] == 0xFFFFFFFF) {
					bootBuffer[lpn - lpnOffset] = blockOffset + nUserPages - 1 - i;
					++yaftl_info.blockArray[blockNumber].validPagesDNo;
					++yaftl_info.blockStats.numValidDPages;
				}
			}
		}

		if (firstBlock && !lpnOffset) {
			yaftl_info.userPagesPerBlock = ftlGeometry.pagesPerSublk;
			copyBTOC(yaftl_info.userTOCBuffer, buff, yaftl_info.totalPages);
		}

		return 0;
	}

	// If we're here, then reading the user-block BTOC has failed.
	if (firstBlock) {
		if (result == 0)
			yaftl_info.userPagesPerBlock = ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock;
		else
			yaftl_info.userPagesPerBlock = ftlGeometry.pagesPerSublk;

		// Calculate the number of user pages in this block, and the number of index pages.
		for (i = 0; i < ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock; i++) {
			// If we find a non-user page, stop here.
			if (YAFTL_readPage(blockOffset + i, (uint8_t*)yaftl_info.tocPageBuffer, spare, 0, 1, 0) != 0
				|| !(spare->type & PAGETYPE_LBN)
				|| spare->lpn >= yaftl_info.totalPages) {
				break;
			}
		}

		yaftl_info.userPagesPerBlock = i;
		yaftl_info.blockArray[blockNumber].validPagesINo = ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock - i;
	}

	// Read each user page in this block.
	for (i = 0; i < yaftl_info.userPagesPerBlock; i++) {
		// Read the current page.
		result = YAFTL_readPage(blockOffset + yaftl_info.userPagesPerBlock - 1 - i, (uint8_t*)yaftl_info.tocPageBuffer, spare, 0, 1, 0);

		// If the read has failed, or the page wasn't a user page, mark as error.
		if (result > 1
			|| (result == 0 && (!(spare->type & PAGETYPE_LBN) || spare->lpn >= yaftl_info.totalPages))) {
			if (!lpnOffset)
				++yaftl_info.blockArray[blockNumber].validPagesINo;
			yaftl_info.blockArray[blockNumber].unkn5 = 0x80;
			readSucceeded = 0;
		}

		if (!lpnOffset && spare->lpn == 0xFFFFFFFF)
			++yaftl_info.blockArray[blockNumber].validPagesINo;

		if (firstBlock && spare->lpn == 0xFFFFFFFF && readSucceeded) {
			yaftl_info.userPagesPerBlock = yaftl_info.userPagesPerBlock - 1 - i;
			continue;
		}

		if (!lpnOffset && spare->lpn < yaftl_info.totalPages) {
			// First time we're in this block, and this is a user page with a valid LPN.
			// Update lpnMax and lpnMin for this block.
			uint32_t lpnMax = blockLpns[blockNumber].lpnMax;
			uint32_t lpnMin = blockLpns[blockNumber].lpnMin;

			if (lpnMax != lpnMin || lpnMin != 0xFFFFFFFF) {
				if (spare->lpn > lpnMax)
					blockLpns[blockNumber].lpnMax = spare->lpn;
				if (spare->lpn < lpnMin)
					blockLpns[blockNumber].lpnMin = spare->lpn;
			} else {
				blockLpns[blockNumber].lpnMax = spare->lpn;
				blockLpns[blockNumber].lpnMin = spare->lpn;
			}
		}

		if (firstBlock)
			yaftl_info.userTOCBuffer[yaftl_info.userPagesPerBlock - 1 - i] = spare->lpn;

		if (lpnOffset > spare->lpn || spare->lpn - lpnOffset >= bootLength) {
			readSucceeded = 0;
			continue;
		}

		++yaftl_info.blockArray[blockNumber].validPagesINo;

		if (bootBuffer[spare->lpn - lpnOffset] == 0xFFFFFFFF) {
			bootBuffer[spare->lpn - lpnOffset] = blockOffset + yaftl_info.userPagesPerBlock - 1 - i;
			++yaftl_info.blockArray[blockNumber].validPagesDNo;
			++yaftl_info.blockStats.numValidDPages;
		}

		readSucceeded = 0;
	}

	return 0;
}

static uint32_t YAFTL_Restore(uint8_t ftlCtrlBlockPresent) {
	uint32_t i, j, result = 0;
	uint8_t *tempBuffer;

	bufferPrintf("yaftl: context is invalid. performing a read-only restore...\r\n");

	tempBuffer = wmr_malloc(ftlGeometry.bytesPerPage);
	if (!tempBuffer) {
		bufferPrintf("yaftl: out of memory.\r\n");
		return EIO;
	}

	// Reset blockArray.
	for (i = 0; i < ftlGeometry.numBlocks; i++) {
		yaftl_info.blockArray[i].validPagesDNo = 0;
		yaftl_info.blockArray[i].validPagesINo = 0;

		if (yaftl_info.blockArray[i].status != BLOCKSTATUS_FTLCTRL
			&& yaftl_info.blockArray[i].status != BLOCKSTATUS_FTLCTRL_SEL) {
			yaftl_info.blockArray[i].status = 0;
		}
	}

	// Reset tocArray and blockStats.
	yaftl_info.latestIndexBlock = 0xFFFFFFFF;
	memset(yaftl_info.tocArray, 0xFF, sizeof(TOCStruct) * yaftl_info.tocArrayLength);
	memset(&yaftl_info.blockStats, 0, sizeof(BlockStats));

	// Add all available blocks to the block lists.
	BlockListNode *userListHead = NULL, *indexListHead = NULL;

	for (i = 0; i < ftlGeometry.numBlocks; i++) {
		uint8_t status = yaftl_info.blockArray[i].status;

		// Ignore FTL blocks.
		if (ftlCtrlBlockPresent && (status == BLOCKSTATUS_FTLCTRL || status == BLOCKSTATUS_FTLCTRL_SEL))
			continue;

		// Try to find a readable page.
		result = 0xFFFFFFFF;
		for (j = 0; j < ftlGeometry.pagesPerSublk - yaftl_info.tocPagesPerBlock; j++) {
			result = YAFTL_readPage(i * ftlGeometry.pagesPerSublk + j,
						tempBuffer, yaftl_info.spareBuffer10, 0, 1, 0);

			if (result <= 1)
				break;
		}

		// If no readable page was found, skip this block.
		if (result > 1)
			continue;

		uint8_t type = yaftl_info.spareBuffer10->type;

		if ((type != 0xFF || yaftl_info.spareBuffer10->lpn != 0xFFFFFFFF) && result != 1) {
			// User or index page. Determine which.
			if (type & PAGETYPE_INDEX) {
				if (!(indexListHead = addBlockToList(indexListHead, i, yaftl_info.spareBuffer10->usn)))
					system_panic("yaftl: out of memory.\r\n");
			} else if (type & PAGETYPE_LBN) {
				if (!(userListHead = addBlockToList(userListHead, i, yaftl_info.spareBuffer10->usn)))
					system_panic("yaftl: out of memory.\r\n");
			} else {
				bufferPrintf("yaftl: warning - block %d doesn't belong to anything (type 0x%02x).\r\n", i, type);
			}
		} else {
			yaftl_info.blockArray[i].status = BLOCKSTATUS_FREE;
			yaftl_info.blockArray[i].unkn5 = 1;
		}
	}

	yaftl_info.unkn_0x2A = yaftl_info.unk184_0xA;

	// Restore index blocks.
	if (indexListHead) {
		BlockListNode *prevIndexNode = indexListHead;

		while (indexListHead) {
			DebugPrintf("yaftl: restoring index block %d\r\n", indexListHead->blockNumber);
			result = restoreIndexBlock(indexListHead->blockNumber, indexListHead == prevIndexNode);

			if (result)
				goto restore_error;

			prevIndexNode = indexListHead;
			indexListHead = indexListHead->next;
			free(prevIndexNode);
		}
	}

	// If we haven't found any index block, use a free one.
	if (yaftl_info.latestIndexBlock == 0xFFFFFFFF) {
		for (i = 0; i < ftlGeometry.numBlocks; i++) {
			if (yaftl_info.blockArray[i].status == BLOCKSTATUS_FREE) {
				yaftl_info.latestIndexBlock = i;
				yaftl_info.unk64 = 0;
				memset(yaftl_info.indexTOCBuffer, 0xFF, yaftl_info.tocPagesPerBlock * ftlGeometry.bytesPerPage);
				yaftl_info.blockArray[i].status = BLOCKSTATUS_I_GC;
				break;
			}
		}
	}

	// Restore user blocks.
	if (userListHead) {
		BlockListNode *currUserNode, *prevUserNode;
		BlockLpn *blockLpns = (BlockLpn*)wmr_malloc(ftlGeometry.numBlocks * sizeof(BlockLpn));

		// TODO: figure this out.
		uint32_t *bootBuffer = (uint32_t*)wmr_malloc(0x800000);
		uint32_t entriesInBootBuffer = 0x800000 / sizeof(uint32_t);

		if (!blockLpns || !bootBuffer) {
			bufferPrintf("yaftl: out of memory.\r\n");

			if (blockLpns)
				free(blockLpns);
			else if (bootBuffer)
				free(bootBuffer);

			goto restore_error;
		}

		memset(blockLpns, 0xFF, ftlGeometry.numBlocks * sizeof(BlockLpn));

		for (i = 0; i < yaftl_info.totalPages; i += entriesInBootBuffer) {
			memset(bootBuffer, 0xFF, entriesInBootBuffer * sizeof(uint32_t));
			currUserNode = prevUserNode = userListHead;

			while (currUserNode) {
				DebugPrintf("yaftl: restoring user block %d\r\n", currUserNode->blockNumber);
				result = restoreUserBlock(
						currUserNode->blockNumber,
						bootBuffer,
						currUserNode == prevUserNode,
						i,
						entriesInBootBuffer,
						blockLpns);

				if (result) {
					free(bootBuffer);
					free(blockLpns);
					goto restore_error;
				}

				prevUserNode = currUserNode;
				currUserNode = currUserNode->next;
				if (i + entriesInBootBuffer >= yaftl_info.totalPages) {
					free(prevUserNode);
				}
			}

			memset(tempBuffer, 0xFF, ftlGeometry.bytesPerPage);

			uint32_t pagesInBootBuffer = entriesInBootBuffer / yaftl_info.tocEntriesPerPage;

			for (j = 0; j < pagesInBootBuffer && (i + j * yaftl_info.tocEntriesPerPage) < yaftl_info.totalPages; j++) {
				uint32_t bootBufferOffset = j * yaftl_info.tocEntriesPerPage;
				uint32_t tocNum = i / yaftl_info.tocEntriesPerPage + j;
				uint32_t indexPage = yaftl_info.tocArray[tocNum].indexPage;
				uint32_t cacheNum;

				memset(tempBuffer, 0xFF, ftlGeometry.bytesPerPage);

				if (memcmp(&bootBuffer[bootBufferOffset], tempBuffer, ftlGeometry.bytesPerPage) != 0) {
					if (indexPage != 0xFFFFFFFF && YAFTL_readPage(indexPage, tempBuffer, 0, 0, 1, 0) != 0) {
						bufferPrintf("yaftl: index page is unreadable at 0x%08x\r\n", indexPage);
						memset(tempBuffer, 0xFF, ftlGeometry.bytesPerPage);
					}

					if (memcmp(&bootBuffer[bootBufferOffset], tempBuffer, ftlGeometry.bytesPerPage) != 0) {
						if (indexPage != 0xFFFFFFFF) {
							--yaftl_info.blockArray[indexPage / ftlGeometry.pagesPerSublk].validPagesINo;
							--yaftl_info.blockStats.numValidIPages;
						}

						cacheNum = findFreeTOCCache();
						if (cacheNum == 0xFFFF) {
							free(bootBuffer);
							free(blockLpns);
							bufferPrintf("yaftl: out of TOC caches, but restore might have already succeeded.\r\n");
							goto restore_final;
						}

						TOCCache *tocCache = &yaftl_info.tocCaches[cacheNum];
						memcpy(tocCache->buffer, &bootBuffer[bootBufferOffset], ftlGeometry.bytesPerPage);
						tocCache->state = 1;
						tocCache->page = tocNum;
						yaftl_info.tocArray[tocNum].cacheNum = cacheNum;
						yaftl_info.tocArray[tocNum].indexPage = 0xFFFFFFFF;
					}
				} else {
					if (indexPage != 0xFFFFFFFF && yaftl_info.blockArray[indexPage].validPagesINo)
						--yaftl_info.blockArray[indexPage].validPagesINo;

					cacheNum = yaftl_info.tocArray[tocNum].cacheNum;
					if (cacheNum != 0xFFFF) {
						yaftl_info.tocCaches[cacheNum].state = 0xFFFF;
						yaftl_info.tocCaches[cacheNum].useCount = 0;
					}

					yaftl_info.tocArray[tocNum].indexPage = 0xFFFFFFFF;
					yaftl_info.tocArray[tocNum].cacheNum = 0xFFFF;
				}
			}
		}

		free(bootBuffer);
		free(blockLpns);

		YAFTL_SetupFreeAndAllocd();

		for (i = 0; i < ftlGeometry.numBlocks; i++) {
			uint8_t status = yaftl_info.blockArray[i].status;

			if (status == BLOCKSTATUS_ALLOCATED || status == BLOCKSTATUS_GC)
				yaftl_info.blockArray[i].validPagesINo = 0;
		}
	} else {
		// No user pages. Find a free block, and choose it to be the latest user page.
		for (i = 0; i < ftlGeometry.numBlocks && yaftl_info.blockArray[i].status != BLOCKSTATUS_FREE; i++);

		if (i < ftlGeometry.numBlocks) {
			yaftl_info.latestUserBlock = i;
			yaftl_info.blockArray[i].status = BLOCKSTATUS_GC;
		}

		yaftl_info.userPagesPerBlock = 0;
		memset(yaftl_info.userTOCBuffer, 0xFF, yaftl_info.tocPagesPerBlock * ftlGeometry.bytesPerPage);
		YAFTL_SetupFreeAndAllocd();
	}

	yaftl_info.blockStats.field_1C = 1;

restore_final:
	YAFTL_LoopThingy();

	if (yaftl_info.ftlCxtPage != 0xFFFFFFFF) {
		yaftl_info.ftlCxtPage =
			ftlGeometry.pagesPerSublk
			+ (yaftl_info.ftlCxtPage / ftlGeometry.pagesPerSublk) * ftlGeometry.pagesPerSublk
			- 1;
	}

	// I know this seems strange, but that's the logic they do here. --Oranav
	for (i = 0; i < ftlGeometry.numBlocks; i++) {
		yaftl_info.maxBlockEraseCount = 0;
		yaftl_info.minBlockEraseCount = 0xFFFFFFFF;

		if (yaftl_info.blockArray[i].status != BLOCKSTATUS_FTLCTRL
			&& yaftl_info.blockArray[i].status != BLOCKSTATUS_FTLCTRL_SEL) {
			if (yaftl_info.blockArray[i].eraseCount)
				yaftl_info.maxBlockEraseCount = yaftl_info.blockArray[i].eraseCount;

			if (yaftl_info.blockArray[i].eraseCount < yaftl_info.minBlockEraseCount)
				yaftl_info.minBlockEraseCount = yaftl_info.blockArray[i].eraseCount;
		}
	}

	yaftl_info.pagesAvailable = yaftl_info.unk188_0x63 * (yaftl_info.totalPages - 1) / 0x64u;
	yaftl_info.bytesPerPage = ftlGeometry.bytesPerPage;

	bufferPrintf("yaftl: restore done.\r\n");

	free(tempBuffer);
	return 0;

restore_error:
	free(tempBuffer);
	return result;
}

static uint32_t YAFTL_Open(uint32_t signature_bit) {
	uint16_t ftlCtrlBlockBuffer[3];
	uint32_t versionSupported = 1;

	memset(yaftl_info.tocArray, 0xFF, yaftl_info.tocArrayLength * sizeof(TOCStruct));
	memset(yaftl_info.blockArray, 0xFF, ftlGeometry.numBlocks * sizeof(BlockStruct));

	uint32_t i;
	for (i = 0; i < ftlGeometry.numBlocks; i++) {
		yaftl_info.blockArray[i].eraseCount = 0;
		yaftl_info.blockArray[i].validPagesDNo = 0;
		yaftl_info.blockArray[i].validPagesINo = 0;
		yaftl_info.blockArray[i].readCount = 0;
		yaftl_info.blockArray[i].status = 0;
		yaftl_info.blockArray[i].unkn5 = 0;
	}

	memcpy(ftlCtrlBlockBuffer, vfl_get_ftl_ctrl_block(vfl), sizeof(ftlCtrlBlockBuffer));

	if (ftlCtrlBlockBuffer[0] == 0xFFFF) {
		// No FTL ctrl block was found.
		return YAFTL_Restore(0);
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
		if (YAFTL_readPage(ftlCtrlBlockBuffer[i] * ftlGeometry.pagesPerSublk, yaftl_info.pageBuffer, yaftl_info.spareBuffer6, 0, 0, 0) == 0) {
			if (yaftl_info.spareBuffer6->usn != 0xFFFFFFFF && yaftl_info.spareBuffer6->usn > maxUsn) {
				maxUsn = yaftl_info.spareBuffer6->usn;
				ftlCtrlBlock = ftlCtrlBlockBuffer[i];
				yaftl_info.selCtrlBlockIndex = i;
			}
		}
	}

	uint32_t some_val;

	if (!maxUsn) {
		yaftl_info.blockArray[ftlCtrlBlockBuffer[0]].status = BLOCKSTATUS_FTLCTRL_SEL;
		yaftl_info.selCtrlBlockIndex = 0;
		some_val = 5;
	} else {
		yaftl_info.blockArray[ftlCtrlBlock].status = BLOCKSTATUS_FTLCTRL_SEL;
		i = 0;
		while (TRUE) {
			if (i >= ftlGeometry.pagesPerSublk - yaftl_info.ctrlBlockPageOffset
				|| YAFTL_readPage(
					ftlGeometry.pagesPerSublk * ftlCtrlBlock + i,
					yaftl_info.pageBuffer,
					yaftl_info.spareBuffer6,
					0, 1, 0) != 0)
			{
				bufferPrintf("yaftl: no valid context slot was found, a restore is definitely needed.\r\n");
				yaftl_info.ftlCxtPage = ~yaftl_info.ctrlBlockPageOffset + ftlGeometry.pagesPerSublk * ftlCtrlBlock + i;
				yaftl_info.field_78 = 0;
				YAFTL_readCxtInfo(yaftl_info.ftlCxtPage, yaftl_info.pageBuffer, 0, &versionSupported);
				some_val = 5;
				break;
			}

			if (YAFTL_readPage(ftlGeometry.pagesPerSublk * ftlCtrlBlock + i + yaftl_info.ctrlBlockPageOffset, yaftl_info.pageBuffer, yaftl_info.spareBuffer6, 0, 1, 0) == 1) {
				yaftl_info.ftlCxtPage = ftlGeometry.pagesPerSublk * ftlCtrlBlock + i;
				if (YAFTL_readCxtInfo(yaftl_info.ftlCxtPage, yaftl_info.pageBuffer, 1, &versionSupported) != 0)
					some_val = 5;
				else
					some_val = 0;
				yaftl_info.field_78 = 0;
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
		yaftl_info.tocCaches[i].buffer = WMR_Buf_Alloc_ForDMA(&yaftl_info.segment_info_temp, ftlGeometry.bytesPerPage);

	if (WMR_BufZone_FinishedAllocs(&yaftl_info.segment_info_temp) != 0)
		return -1;

	for (i = 0; i < yaftl_info.unk184_0xA; i++) {
		yaftl_info.tocCaches[i].buffer = WMR_BufZone_Rebase(&yaftl_info.segment_info_temp, yaftl_info.tocCaches[i].buffer);
		yaftl_info.tocCaches[i].state = 0xFFFF;
		yaftl_info.tocCaches[i].page = 0xFFFF;
		yaftl_info.tocCaches[i].useCount = 0;
		memset(yaftl_info.tocCaches[i].buffer, 0xFF, ftlGeometry.bytesPerPage);
	}

	if (WMR_BufZone_FinishedRebases(&yaftl_info.segment_info_temp) != 0)
		return -1;

	if (restoreNeeded)
		return YAFTL_Restore(1);

	yaftl_info.pagesAvailable = yaftl_info.unk188_0x63 * (yaftl_info.totalPages - 1) / 100;
	yaftl_info.bytesPerPage = ftlGeometry.bytesPerPage;

	yaftl_info.maxBlockEraseCount = 0;
	yaftl_info.minBlockEraseCount = 0xFFFFFFFF;

	for (i = 0; i < ftlGeometry.numBlocks; i++) {
		if (yaftl_info.blockArray[i].status != BLOCKSTATUS_FTLCTRL
			&& yaftl_info.blockArray[i].status != BLOCKSTATUS_FTLCTRL_SEL)
		{
			if (yaftl_info.blockArray[i].eraseCount > yaftl_info.maxBlockEraseCount)
				yaftl_info.maxBlockEraseCount = yaftl_info.blockArray[i].eraseCount;

			if (yaftl_info.blockArray[i].eraseCount < yaftl_info.minBlockEraseCount)
				yaftl_info.minBlockEraseCount = yaftl_info.blockArray[i].eraseCount;
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

	for (i = 0; pagesToRead > ftlGeometry.pagesPerSublk; i++) {
		//ret = VFL_ReadScatteredPagesInVb(&buf[i * ftlGeometry.pagesPerSublk],ftlGeometry.pagesPerSublk, databuffer + i * ftlGeometry.pagesPerSublk * ftlGeometry.bytesPerPage, metabuf_array, &unkn, 0, a4, &status);
		ret = 0; // For now. --Oranav

		if (ret) {
			if (status)
				return 0;
		} else {
			//bufferPrintf("yaftl: _readMultiPages: We got read failure!!\r\n");
			succeeded = 1;

			for (j = 0; j < ftlGeometry.pagesPerSublk; j++) {
				page = i * ftlGeometry.pagesPerSublk + j;
				ret = YAFTL_readPage(
						pagesArray[page],
						&dataBuffer[page * ftlGeometry.bytesPerPage],
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

		block = pagesArray[(i + 1) * ftlGeometry.pagesPerSublk] / ftlGeometry.pagesPerSublk;
		yaftl_info.blockArray[block].readCount++;

		pagesToRead -= ftlGeometry.pagesPerSublk;
	}

	if (pagesToRead == 0)
		return 1;

	block = pagesArray[i * ftlGeometry.pagesPerSublk] / ftlGeometry.pagesPerSublk;
	yaftl_info.blockArray[block].readCount++;

	unkn = 0;
	// ret = VFL_ReadScatteredPagesInVb(&buf[i * ftlGeometry.pagesPerSublk], pagesToRead % ftlGeometry.pagesPerSublk, databuffer + i * ftlGeometry.pagesPerSublk * ftlGeometry.bytesPerPage, metabuf_array, &unkn, 0, a4, &status);
	ret = 0; // For now. --Oranav

	if (ret)
		return !status;
	else {
		//bufferPrintf("yaftl:_readMultiPages: We got read failure!!\r\n");
		succeeded = 1;

		for (j = 0; j != pagesToRead; j++) {
			page = i * ftlGeometry.pagesPerSublk + j;
			ret = YAFTL_readPage(
					pagesArray[page],
					&dataBuffer[page * ftlGeometry.bytesPerPage],
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

static int YAFTL_Read(uint32_t lpn, uint32_t nPages, uint8_t* pBuf)
{
	uint32_t emf = h2fmi_get_emf();
	h2fmi_set_emf(0, 0);
	int ret = 0;
	uint32_t tocPageNum, tocEntry, tocCacheNum, freeTOCCacheNum;
	uint32_t testMode, pagesRead = 0, numPages = 0;
	uint8_t* data = NULL;
	uint8_t* readBuf = pBuf;

	if (!pBuf && nPages != 1)
		return EINVAL;

	if ((lpn + nPages) >= yaftl_info.totalPages)
		return EINVAL;

	nand_device_set_ftl_region(vfl->get_device(vfl), lpn, 0, nPages, pBuf);

	yaftl_info.lastTOCPageRead = 0xFFFFFFFF;

	// Omitted for now. --Oranav
	// yaftl_info.unk33C = 0;

	testMode = (!pBuf && nPages == 1);

	if (testMode)
		*yaftl_info.unknBuffer3_ftl = 0xFFFFFFFF;

	while (pagesRead != nPages) {
		tocPageNum = (lpn + pagesRead) / yaftl_info.tocEntriesPerPage;
		if ((yaftl_info.tocArray[tocPageNum].cacheNum == 0xFFFF) && (yaftl_info.tocArray[tocPageNum].indexPage == 0xFFFFFFFF)) {
			if (testMode)
				return 0;

			h2fmi_set_emf(emf, 0);
			if (!YAFTL_readMultiPages(yaftl_info.unknBuffer3_ftl, numPages, data, 0, 0, 1)
				|| !YAFTL_verifyMetaData(lpn + (data - pBuf) / ftlGeometry.bytesPerPage, yaftl_info.buffer20, numPages))
				ret = EIO;
			h2fmi_set_emf(0, 0);

			numPages = 0;
			readBuf += ftlGeometry.bytesPerPage;
		} else {
			// Should lookup TOC.
			tocCacheNum = yaftl_info.tocArray[tocPageNum].cacheNum;

			if (tocCacheNum != 0xFFFF) {
				// There's a cache for it!
				yaftl_info.tocCaches[tocCacheNum].useCount++;
				tocEntry = yaftl_info.tocCaches[tocCacheNum].buffer[(lpn + pagesRead) % yaftl_info.tocEntriesPerPage];
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

					tocEntry = yaftl_info.tocCaches[freeTOCCacheNum].buffer[(lpn + pagesRead) % yaftl_info.tocEntriesPerPage];
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

					tocEntry = yaftl_info.tocPageBuffer[(lpn + pagesRead) % yaftl_info.tocEntriesPerPage];
				}
			}

			// Okay, obtained the TOC entry.
			if (tocEntry == 0xFFFFFFFF) {
				if (testMode)
					return 0;

				h2fmi_set_emf(emf, 0);
				if (!YAFTL_readMultiPages(yaftl_info.unknBuffer3_ftl, numPages, data, 0, 0, 1)
					|| !YAFTL_verifyMetaData(lpn + (data - pBuf) / ftlGeometry.bytesPerPage,
									yaftl_info.buffer20,
									numPages))
					ret = EIO;
				h2fmi_set_emf(0, 0);

				numPages = 0;
				data = NULL;
			} else {
				yaftl_info.unknBuffer3_ftl[numPages] = tocEntry;
				numPages++;

				if (data == NULL)
					data = readBuf;

				if (numPages == ftlGeometry.pagesPerSublk) {
					if (testMode)
						return 0;

					h2fmi_set_emf(emf, 0);
					if (!YAFTL_readMultiPages(yaftl_info.unknBuffer3_ftl, numPages, data, 0, 0, 1)
						|| !YAFTL_verifyMetaData(lpn + (data - pBuf) / ftlGeometry.bytesPerPage,
										yaftl_info.buffer20,
										numPages))
						ret = EIO;
					h2fmi_set_emf(0, 0);

					numPages = 0;
					data = NULL;
				}
			}

			readBuf += ftlGeometry.bytesPerPage;
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

	h2fmi_set_emf(emf, 0);
	if (!YAFTL_readMultiPages(yaftl_info.unknBuffer3_ftl, numPages, data, 0, 0, 1)
		|| !YAFTL_verifyMetaData(lpn + (data - pBuf) / ftlGeometry.bytesPerPage,
						yaftl_info.buffer20,
						numPages))
		ret = EIO;
	h2fmi_set_emf(0, 0);

YAFTL_READ_RETURN:
	nand_device_set_ftl_region(vfl->get_device(vfl), 0, 0, 0, 0);
	return ret;
}

static error_t yaftl_read_mtd(mtd_t *_dev, void *_dest, uint64_t _off, int _amt)
{
	uint32_t emf = h2fmi_get_emf();
	uint8_t* curLoc = (uint8_t*) _dest;
	uint32_t block_size = yaftl_info.bytesPerPage;
	int curPage = _off / block_size;
	int toRead = _amt;
	int pageOffset = _off - (curPage * block_size);
	uint8_t* tBuffer = (uint8_t*) malloc(block_size);
	while(toRead > 0) {
		if((&_dev->bdev
				&& (_dev->bdev.part_mode == partitioning_gpt || _dev->bdev.part_mode == partitioning_lwvm)
					&& _dev->bdev.handle->pIdx == 1)
				|| emf) {
			h2fmi_set_emf(1, curPage);
		}
		else
			h2fmi_set_emf(0, 0);
		uint32_t ret = YAFTL_Read(curPage, 1, tBuffer);
		if(FAILED(ret)) {
			h2fmi_set_emf(0, 0);
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

	h2fmi_set_emf(0, 0);
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

static int64_t yaftl_block_size(mtd_t *_dev)
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
