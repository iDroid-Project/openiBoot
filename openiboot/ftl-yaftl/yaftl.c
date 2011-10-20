#include "ftl/yaftl.h"
#include "ftl/yaftl_mem.h"
#include "ftl/l2v.h"
#include "openiboot.h"
#include "commands.h"
#include "nand.h"
#include "vfl.h"
#include "ftl.h"
#include "mtd.h"
#include "bdev.h"
#include "util.h"
#include "h2fmi.h"

static YAFTLInfo sInfo;
static NAND_GEOMETRY_FTL sGeometry;

static uint32_t yaftl_inited = 0;
static vfl_device_t* vfl = 0;

static uint32_t BTOC_Init()
{
	sInfo.unk74_4 = 4;
	sInfo.unk78_counter = 0;
	sInfo.unkAC_2 = 2;
	sInfo.unkB0_1 = 1;

	sInfo.unk8c_buffer = yaftl_alloc(0x10);
	sInfo.unk88_buffer = yaftl_alloc(sInfo.unk74_4 << 2);
	sInfo.unk84_buffer = yaftl_alloc(sInfo.unk74_4 << 2);
	sInfo.unkB8_buffer = yaftl_alloc(sInfo.unkAC_2 << 2);
	sInfo.unkB4_buffer = yaftl_alloc(sInfo.unkAC_2 << 2);

	if(!sInfo.unk8c_buffer || !sInfo.unk8c_buffer || !sInfo.unk84_buffer || !sInfo.unkB8_buffer || !sInfo.unkB4_buffer)
		return -1;

	uint32_t i;
	for(i = 0; i < sInfo.unk74_4; i++) {
		sInfo.unk88_buffer[i] = -1;
	}

	for(i = 0; i < sInfo.unkAC_2; i++) {
		sInfo.unkB4_buffer[i] = -1;
		sInfo.unkB8_buffer[i] = yaftl_alloc(sGeometry.pagesPerSublk<<2);
		memset(sInfo.unkB8_buffer[i], 0xFF, sGeometry.pagesPerSublk<<2);
	}

	sInfo.unk7C_byteMask = (1 << sInfo.unk74_4) - 1;
	sInfo.unk80_3 = 3;

	bufzone_init(&sInfo.ftl_buffer2);

	for(i = 0; i < sInfo.unk74_4; i++) {
		sInfo.unk84_buffer[i] = bufzone_alloc(&sInfo.ftl_buffer2, sGeometry.bytesPerPage * sInfo.tocPagesPerBlock);
	}

	if(FAILED(bufzone_finished_allocs(&sInfo.ftl_buffer2)))
		return -1;

	for(i = 0; i < sInfo.unk74_4; i++) {
		sInfo.unk84_buffer[i] = bufzone_rebase(&sInfo.ftl_buffer2, sInfo.unk84_buffer[i]);
	}

	if (FAILED(bufzone_finished_rebases(&sInfo.ftl_buffer2)))
		return -1;

	return 0;
}

static uint32_t* BTOC_Alloc(uint32_t _arg0, uint32_t _unused_arg1)
{
	int32_t inited = 0x7FFFFFFF;
	sInfo.unkB0_1 = (sInfo.unkB0_1 + 1) % sInfo.unkAC_2;
	sInfo.unkB4_buffer[sInfo.unkB0_1] = _arg0;

	memset(sInfo.unkB8_buffer[sInfo.unkB0_1], 0xFF, sGeometry.pagesPerSublk * sizeof(uint32_t));

	sInfo.unk78_counter++;

	uint32_t found = -1;
	uint32_t i;
	for (i = 0; i < sInfo.unk74_4; i++) {
		if ((sInfo.unk80_3 & (1<<i)) && (sInfo.unk7C_byteMask & (1<<i)) && (inited > (int32_t)sInfo.unk8c_buffer[i])) {
			found = i;
			inited = sInfo.unk8c_buffer[i];
		}
	}

	if(found == -1)
		system_panic("yaftl: couldn't allocate a BTOC.\r\n");

	sInfo.unk7C_byteMask &= ~(1<<found);
	sInfo.unk88_buffer[found] = _arg0;
	sInfo.unk8c_buffer[found] = sInfo.unk78_counter;
	return sInfo.unk84_buffer[found];
}

static void YAFTL_LoopThingy()
{
	uint32_t i;
	for (i = 0; i < sInfo.unk74_4; i++) {
		if(sInfo.unk88_buffer[i] == ~3)
			sInfo.unk88_buffer[i] = sInfo.latestUserBlk.blockNum;
		if(sInfo.unk88_buffer[i] == ~2)
			sInfo.unk88_buffer[i] = sInfo.latestIndexBlk.blockNum;
	}
}

static void YAFTL_setupCleanSpare(SpareData* _spare_ptr)
{
	_spare_ptr->usn = ++sInfo.maxIndexUsn;
	_spare_ptr->type = PAGETYPE_FTL_CLEAN;
}

static error_t YAFTL_writeIndexTOC()
{
	int result;

	YAFTL_setupCleanSpare(sInfo.spareBuffer7);

	// Try to write the new index TOC.
	result = vfl_write_single_page(vfl,
			sInfo.ctrlBlockPageOffset + sInfo.ftlCxtPage, 
			(uint8_t*)sInfo.latestIndexBlk.tocBuffer,
			(uint8_t*)sInfo.spareBuffer7, 1);

	if (FAILED(result)) {
		// Must erase: first, erase the current FTL ctrl block.
		vfl_erase_single_block(vfl, sInfo.FTLCtrlBlock[sInfo.selCtrlBlockIndex], 1);
		sInfo.blockArray[sInfo.FTLCtrlBlock[sInfo.selCtrlBlockIndex]].eraseCount++;
		sInfo.totalEraseCount++;
		sInfo.blockArray[sInfo.FTLCtrlBlock[sInfo.selCtrlBlockIndex]].status = BLOCKSTATUS_FTLCTRL;

		// Select the new FTL ctrl block.
		++sInfo.selCtrlBlockIndex;
		if (sInfo.selCtrlBlockIndex > 2)
			sInfo.selCtrlBlockIndex = 0;

		// Erase it.
		sInfo.blockArray[sInfo.FTLCtrlBlock[sInfo.selCtrlBlockIndex]].status = BLOCKSTATUS_FTLCTRL_SEL;
		vfl_erase_single_block(vfl, sInfo.FTLCtrlBlock[sInfo.selCtrlBlockIndex], 1);
		sInfo.blockArray[sInfo.FTLCtrlBlock[sInfo.selCtrlBlockIndex]].eraseCount++;
		sInfo.totalEraseCount++;
		sInfo.ftlCxtPage = 0xFFFFFFFF;
	}

	return SUCCESS;
}

uint32_t YAFTL_writePage(uint32_t _page, uint8_t* _data_ptr, SpareData* _spare_ptr)
{
	int result;
	uint32_t block = _page / sGeometry.pagesPerSublk;

	if (sInfo.field_78)
	{
		YAFTL_writeIndexTOC();
		sInfo.field_78 = 0;
	}

	result = vfl_write_single_page(vfl, _page, _data_ptr, (uint8_t*)_spare_ptr, 1);

	if (FAILED(result)) {
		bufferPrintf("YAFTL_writePage: We got write failure: page %d, block %d, block status %d.\r\n",
			_page,
			block,
			sInfo.blockArray[block].status);

		return ERROR_ARG;
	}

	return result;
}

static uint32_t YAFTL_readPage(uint32_t _page, uint8_t* _data_ptr, SpareData* _spare_ptr, uint32_t _disable_aes, uint32_t _empty_ok, uint32_t _scrub)
{
	int refreshPage = 0, result;
	uint32_t block = _page / sGeometry.pagesPerSublk;

	uint8_t* meta_ptr = (uint8_t*)(_spare_ptr ? _spare_ptr : sInfo.spareBuffer18);

	result = vfl_read_single_page(vfl, _page, _data_ptr, meta_ptr, _empty_ok, &refreshPage, _disable_aes);

	if (FAILED(result)) {
		bufferPrintf("YAFTL_readPage: We got read failure: page %d, block %d, block status %d, scrub %d.\r\n",
			_page,
			block,
			sInfo.blockArray[block].status,
			_scrub);

		return ERROR_ARG;
	}

	sInfo.blockArray[block].readCount++;
	return result;
}

static uint32_t YAFTL_readCxtInfo(uint32_t _page, uint8_t* _ptr, uint8_t _extended, uint32_t *pSupported)
{
	uint32_t pageToRead;
	uint32_t i = 0, j = 0;

	uint16_t* _ptr16 = (uint16_t*)_ptr;
	uint32_t* _ptr32 = (uint32_t*)_ptr;

	uint32_t leftToRead = 0, perPage = 0;

	if (YAFTL_readPage(_page, _ptr, sInfo.spareBuffer5, 0, 1, 0))
		return ERROR_ARG;

	if (!(sInfo.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
		return ERROR_ARG;

	YAFTL_CXT *pCxt = (YAFTL_CXT*)_ptr;

	if (memcmp(pCxt->version, "CX01", 4)) {
		if (pSupported)
			*pSupported = 0;

		bufferPrintf("yaftl: Wrong context version.\r\n");
		return ERROR_ARG;
	}

	if (_extended) {
		sInfo.tocPagesPerBlock = pCxt->tocPagesPerBlock;
		sInfo.tocEntriesPerPage = pCxt->tocEntriesPerPage;
		sInfo.unknCalculatedValue0 = pCxt->unknCalculatedValue0;
		sInfo.tocArrayLength = pCxt->tocArrayLength;
		sInfo.totalPages = pCxt->totalPages;
		sInfo.unkn_0x2A = pCxt->unkn_0x2A;
		sInfo.latestUserBlk.blockNum = pCxt->latestUserBlock;
		sInfo.latestUserBlk.usedPages = pCxt->pagesUsedInLatestUserBlk;
		sInfo.latestIndexBlk.blockNum = pCxt->latestIndexBlock;
		sInfo.blockStats.field_4 = pCxt->blockStatsField4;
		sInfo.blockStats.field_10 = pCxt->blockStatsField10;
		sInfo.blockStats.numAllocated = pCxt->numAllocatedBlocks;
		sInfo.blockStats.numIAllocated = pCxt->numIAllocatedBlocks;
		sInfo.latestIndexBlk.usedPages = pCxt->pagesUsedInLatestIdxBlk;
		sInfo.maxIndexUsn = pCxt->maxIndexUsn;
		sInfo.unk184_0xA = pCxt->unk184_0xA;
		sInfo.unk188_0x63 = pCxt->unk188_0x63;

		DebugPrintf("yaftl: reading latestUserBlk.tocBuffer\r\n");

		// Read the user TOC into latestUserBlk.tocBuffer.
		pageToRead = _page + 1;
		for (i = 0; i < sInfo.tocPagesPerBlock; i++) {
			if (YAFTL_readPage(pageToRead + i, &((uint8_t*)sInfo.latestUserBlk.tocBuffer)[sGeometry.bytesPerPage * i], sInfo.spareBuffer5, 0, 1, 0) ||
					!(sInfo.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			{
				sInfo.field_78 = 0;
				return ERROR_ARG;
			}
		}

		DebugPrintf("yaftl: reading latestIndexBlk.tocBuffer\r\n");

		// Read the index TOC into latestIndexBlk.tocBuffer.
		pageToRead += sInfo.tocPagesPerBlock;
		for (i = 0; i < sInfo.tocPagesPerBlock; i++) {
			if (YAFTL_readPage(pageToRead + i, &((uint8_t*)sInfo.latestIndexBlk.tocBuffer)[sGeometry.bytesPerPage * i], sInfo.spareBuffer5, 0, 1, 0) ||
					!(sInfo.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			{
				sInfo.field_78 = 0;
				return ERROR_ARG;
			}
		}

		DebugPrintf("yaftl: reading tocArray's index pages\r\n");

		// Read tocArray's index pages.
		pageToRead += sInfo.tocPagesPerBlock + 1;
		leftToRead = sInfo.tocArrayLength;
		perPage = sGeometry.bytesPerPage / sizeof(sInfo.tocArray->indexPage);

		for (i = 0; i < sInfo.nPagesTocPageIndices; i++) {
			if (YAFTL_readPage(pageToRead + i, _ptr, sInfo.spareBuffer5, 0, 1, 0) ||
					!(sInfo.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			{
				sInfo.field_78 = 0;
				return ERROR_ARG;
			}

			for (j = 0; j < leftToRead && j < perPage; j++)
				sInfo.tocArray[i * perPage + j].indexPage = _ptr32[j];

			leftToRead -= perPage;
		}

		DebugPrintf("yaftl: reading blockArrays's statuses\r\n");

		// Read blockArray's statuses.
		pageToRead += sInfo.nPagesTocPageIndices;
		leftToRead = sGeometry.numBlocks;
		perPage = sGeometry.bytesPerPage / sizeof(sInfo.blockArray->status);

		for (i = 0; i < sInfo.nPagesBlockStatuses; i++) {
			if (YAFTL_readPage(pageToRead + i, _ptr, sInfo.spareBuffer5, 0, 1, 0) ||
					!(sInfo.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			{
				sInfo.field_78 = 0;
				return ERROR_ARG;
			}

			for (j = 0; j < leftToRead && j < perPage; j++)
				sInfo.blockArray[i * perPage + j].status = _ptr[j];

			leftToRead -= perPage;
		}
	} else {
		// Skip.
		pageToRead = _page + sInfo.nPagesTocPageIndices + 2 + (sInfo.tocPagesPerBlock * 2);
	}

	DebugPrintf("yaftl: reading blockArrays's read counts\r\n");

	// Read blockArray's readCounts.
	pageToRead += sInfo.nPagesBlockStatuses;
	leftToRead = sGeometry.numBlocks;
	perPage = sGeometry.bytesPerPage / sizeof(sInfo.blockArray->readCount);

	for (i = 0; i < sInfo.nPagesBlockReadCounts; i++) {
		if (YAFTL_readPage(pageToRead + i, _ptr, sInfo.spareBuffer5, 0, 1, 0) ||
				!(sInfo.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
		{
			sInfo.field_78 = 0;
			return ERROR_ARG;
		}

		for (j = 0; j < leftToRead && j < perPage; j++)
			sInfo.blockArray[i * perPage + j].readCount = _ptr16[j];

		leftToRead -= perPage;
	}

	DebugPrintf("yaftl: reading blockArrays's eraseCounts\r\n");

	// Read blockArray's eraseCounts.
	pageToRead += sInfo.nPagesBlockReadCounts;
	leftToRead = sGeometry.numBlocks;
	perPage = sGeometry.bytesPerPage / sizeof(sInfo.blockArray->eraseCount);

	for (i = 0; i < sInfo.nPagesBlockEraseCounts; i++) {
		if (YAFTL_readPage(pageToRead + i, _ptr, sInfo.spareBuffer5, 0, 1, 0) ||
				!(sInfo.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
		{
			sInfo.field_78 = 0;
			return ERROR_ARG;
		}

		for (j = 0; j < leftToRead && j < perPage; j++)
			sInfo.blockArray[i * perPage + j].eraseCount = _ptr32[j];

		leftToRead -= perPage;
	}

	if (_extended) {

		DebugPrintf("yaftl: reading blockArrays's validPagesINo\r\n");

		// Read blockArray's validPagesINo.
		pageToRead += sInfo.nPagesBlockEraseCounts;
		leftToRead = sGeometry.numBlocks;
		perPage = sGeometry.bytesPerPage / sizeof(sInfo.blockArray->validPagesINo);

		for (i = 0; i < sInfo.nPagesBlockValidPagesINumbers; i++) {
			if (YAFTL_readPage(pageToRead+i, _ptr, sInfo.spareBuffer5, 0, 1, 0) ||
					!(sInfo.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			{
				sInfo.field_78 = 0;
				return ERROR_ARG;
			}

			for (j = 0; j < leftToRead && j < perPage; j++) {
				sInfo.blockArray[i * perPage + j].validPagesINo = _ptr16[j];
				sInfo.blockStats.numValidIPages += sInfo.blockArray[i * perPage + j].validPagesINo;
			}

			leftToRead -= perPage;
		}

		DebugPrintf("yaftl: reading blockArrays's validPagesDNo\r\n");

		// Read blockArray's validPagesDNo.
		pageToRead += sInfo.nPagesBlockValidPagesINumbers;
		leftToRead = sGeometry.numBlocks;
		perPage = sGeometry.bytesPerPage / sizeof(sInfo.blockArray->validPagesDNo);

		for (i = 0; i < sInfo.nPagesBlockValidPagesDNumbers; i++) {
			if (YAFTL_readPage(pageToRead+i, _ptr, sInfo.spareBuffer5, 0, 1, 0) ||
					!(sInfo.spareBuffer5->type & PAGETYPE_FTL_CLEAN))
			{
				sInfo.field_78 = 0;
				return ERROR_ARG;
			}

			for (j = 0; j < leftToRead && j < perPage; j++) {
				sInfo.blockArray[i * perPage + j].validPagesDNo = _ptr16[j];
				sInfo.blockStats.numValidDPages += sInfo.blockArray[i * perPage + j].validPagesDNo;
			}

			leftToRead -= perPage;
		}
	}
	return 0;
}

static uint32_t YAFTL_Init()
{
	if(yaftl_inited)
		system_panic("Oh shit, yaftl already initialized!\r\n");

	memset(&sInfo, 0, sizeof(sInfo)); // 0x358

	sInfo.lastTOCPageRead = 0xFFFFFFFF;
	sInfo.ftlCxtPage = 0xFFFFFFFF;
	sInfo.unk184_0xA = 0xA;
	sInfo.unk188_0x63 = 0x63;

	uint16_t metaBytes, unkn20_1;
	error_t error = 0;

	error |= vfl_get_info(vfl, diPagesPerBlockTotalBanks, &sGeometry.pagesPerSublk, sizeof(sGeometry.pagesPerSublk));
	error |= vfl_get_info(vfl, diSomeThingFromVFLCXT, &sGeometry.numBlocks, sizeof(sGeometry.numBlocks));
	error |= vfl_get_info(vfl, diBytesPerPageFTL, &sGeometry.bytesPerPage, sizeof(sGeometry.bytesPerPage));
	error |= vfl_get_info(vfl, diTotalBanks, &sGeometry.total_banks_ftl, sizeof(sGeometry.total_banks_ftl));

	error |= vfl_get_info(vfl, diMetaBytes0xC, &metaBytes, sizeof(metaBytes));
	error |= vfl_get_info(vfl, diUnkn20_1, &unkn20_1, sizeof(unkn20_1));
	sGeometry.spareDataSize = metaBytes * unkn20_1;

	sGeometry.total_usable_pages = sGeometry.pagesPerSublk * sGeometry.numBlocks;

	if (FAILED(error))
		system_panic("yaftl: vfl get info failed!\r\n");

	if (sGeometry.spareDataSize != 0xC)
		system_panic("yaftl: spareDataSize isn't 0xC!\r\n");

	bufferPrintf("yaftl: got information from VFL.\r\n");
	bufferPrintf("pagesPerSublk: %d\r\n", sGeometry.pagesPerSublk);
	bufferPrintf("numBlocks: %d\r\n", sGeometry.numBlocks);
	bufferPrintf("bytesPerPage: %d\r\n", sGeometry.bytesPerPage);
	bufferPrintf("total_banks_ftl: %d\r\n", sGeometry.total_banks_ftl);
	bufferPrintf("metaBytes: %d\r\n", metaBytes);
	bufferPrintf("unkn20_1: %d\r\n", unkn20_1);
	bufferPrintf("total_usable_pages: %d\r\n", sGeometry.total_usable_pages);

	sInfo.tocPagesPerBlock = (sGeometry.pagesPerSublk * sizeof(uint32_t)) / sGeometry.bytesPerPage;
	if (((sGeometry.pagesPerSublk * sizeof(uint32_t)) % sGeometry.bytesPerPage) != 0)
		sInfo.tocPagesPerBlock++;

	sInfo.tocEntriesPerPage = sGeometry.bytesPerPage / sizeof(uint32_t);
	bufzone_init(&sInfo.zone);
	sInfo.pageBuffer = bufzone_alloc(&sInfo.zone, sGeometry.bytesPerPage);
	sInfo.tocPageBuffer = bufzone_alloc(&sInfo.zone, sGeometry.bytesPerPage);
	sInfo.pageBuffer2 = bufzone_alloc(&sInfo.zone, sGeometry.bytesPerPage * sInfo.tocPagesPerBlock);
	sInfo.spareBuffer3 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer4 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer5 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer6 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer7 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer8 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer9 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer10 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer11 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer12 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer13 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer14 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer15 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer16 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer17 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer18 = bufzone_alloc(&sInfo.zone, sizeof(SpareData));
	sInfo.spareBuffer19 = bufzone_alloc(&sInfo.zone, 32 * sGeometry.total_banks_ftl * sizeof(SpareData));
	sInfo.buffer20 = bufzone_alloc(&sInfo.zone, sGeometry.pagesPerSublk * sizeof(SpareData));

	if (FAILED(bufzone_finished_allocs(&sInfo.zone)))
		system_panic("YAFTL_Init: bufzone_finished_allocs failed!");

	sInfo.pageBuffer = bufzone_rebase(&sInfo.zone, sInfo.pageBuffer);
	sInfo.tocPageBuffer = bufzone_rebase(&sInfo.zone, sInfo.tocPageBuffer);
	sInfo.pageBuffer2 = bufzone_rebase(&sInfo.zone, sInfo.pageBuffer2);
	sInfo.spareBuffer3 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer3);
	sInfo.spareBuffer4 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer4);
	sInfo.spareBuffer5 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer5);
	sInfo.spareBuffer6 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer6);
	sInfo.spareBuffer7 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer7);
	sInfo.spareBuffer8 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer8);
	sInfo.spareBuffer9 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer9);
	sInfo.spareBuffer10 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer10);
	sInfo.spareBuffer11 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer11);
	sInfo.spareBuffer12 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer12);
	sInfo.spareBuffer13 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer13);
	sInfo.spareBuffer14 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer14);
	sInfo.spareBuffer15 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer15);
	sInfo.spareBuffer16 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer16);
	sInfo.spareBuffer17 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer17);
	sInfo.spareBuffer18 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer18);
	sInfo.spareBuffer19 = bufzone_rebase(&sInfo.zone, sInfo.spareBuffer19);
	sInfo.buffer20 = bufzone_rebase(&sInfo.zone, sInfo.buffer20);

	if (FAILED(bufzone_finished_rebases(&sInfo.zone)))
		return -1;

	sInfo.unknCalculatedValue0 = CEIL_DIVIDE((sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock) - (sGeometry.numBlocks - 8), (sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock) * sInfo.tocEntriesPerPage) * 3;

	sInfo.unknCalculatedValue1 = sGeometry.numBlocks - sInfo.unknCalculatedValue0 - 3;

	sInfo.totalPages =
		(sGeometry.numBlocks - 8) * (sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock)
		- sInfo.unknCalculatedValue0 * sGeometry.pagesPerSublk;

	sInfo.tocArrayLength = CEIL_DIVIDE(sGeometry.pagesPerSublk * sGeometry.numBlocks * 4, sGeometry.bytesPerPage);

	sInfo.tocArray = yaftl_alloc(sInfo.tocArrayLength * sizeof(TOCStruct));
	if (!sInfo.tocArray)
		system_panic("No buffer.\r\n");

	sInfo.blockArray = yaftl_alloc(sGeometry.numBlocks * sizeof(BlockStruct));
	if (!sInfo.blockArray)
		system_panic("No buffer.\r\n");

	sInfo.unknBuffer3_ftl = yaftl_alloc(sGeometry.pagesPerSublk << 2);
	if (!sInfo.unknBuffer3_ftl)
		system_panic("No buffer.\r\n");

	// Initialize number of pages in a context slot.
	sInfo.nPagesTocPageIndices = CEIL_DIVIDE(sInfo.tocArrayLength * sizeof(sInfo.tocArray->indexPage), sGeometry.bytesPerPage);
	sInfo.nPagesBlockStatuses = CEIL_DIVIDE(sGeometry.numBlocks * sizeof(sInfo.blockArray->status), sGeometry.bytesPerPage);
	sInfo.nPagesBlockReadCounts = CEIL_DIVIDE(sGeometry.numBlocks * sizeof(sInfo.blockArray->readCount), sGeometry.bytesPerPage);
	sInfo.nPagesBlockValidPagesDNumbers = sInfo.nPagesBlockReadCounts;
	sInfo.nPagesBlockValidPagesINumbers = sInfo.nPagesBlockReadCounts;
	sInfo.nPagesBlockEraseCounts = CEIL_DIVIDE(sGeometry.numBlocks * sizeof(sInfo.blockArray->eraseCount), sGeometry.bytesPerPage);

	sInfo.totalEraseCount = 0;
	sInfo.field_78 = 0;

	sInfo.ctrlBlockPageOffset =
		sInfo.nPagesTocPageIndices
		+ sInfo.nPagesBlockStatuses
		+ sInfo.nPagesBlockReadCounts
		+ sInfo.nPagesBlockEraseCounts
		+ sInfo.nPagesBlockValidPagesDNumbers
		+ sInfo.nPagesBlockValidPagesINumbers
		+ 2 * sInfo.tocPagesPerBlock
		+ 2;

	memset(&sInfo.blockStats, 0, sizeof(sInfo.blockStats));

	// TODO: Some more initialization is done in the kernel version of yaFTL.

	if(BTOC_Init())
		system_panic("yaftl: Actually (and before) it should free something, but,.. hey, we don't fuck up, do we?\r\n");

	if (FAILED(L2V_Init(sInfo.totalPages, sGeometry.numBlocks, sGeometry.pagesPerSublk)))
		system_panic("yaftl: L2V initialization has failed\r\n");

	sInfo.latestUserBlk.tocBuffer = BTOC_Alloc(~2, 1);
	sInfo.latestIndexBlk.tocBuffer = BTOC_Alloc(~1, 0);

	yaftl_inited = 1;

	return 0;
}

static void YAFTL_SetupFreeAndAllocd()
{
	uint32_t i;

	sInfo.blockStats.field_4 = 0;
	sInfo.blockStats.numAllocated = 0;
	sInfo.blockStats.field_10 = 0;
	sInfo.blockStats.numIAllocated = 0;
	sInfo.blockStats.numFree = 0;

	for (i = 0; i < sGeometry.numBlocks; i++) {
		switch (sInfo.blockArray[i].status) {
		case BLOCKSTATUS_ALLOCATED:
		case BLOCKSTATUS_GC:
			sInfo.blockStats.numAllocated++;
			break;

		case BLOCKSTATUS_I_ALLOCATED:
		case BLOCKSTATUS_I_GC:
			sInfo.blockStats.numIAllocated++;
			break;

		case BLOCKSTATUS_FREE:
			sInfo.blockStats.numFree++;
			break;
		}
	}

	sInfo.blockStats.field_10 = sInfo.unknCalculatedValue0 - sInfo.blockStats.numIAllocated;
	sInfo.blockStats.field_4 = sInfo.blockStats.numFree - sInfo.blockStats.field_10;
}

static uint32_t findFreeTOCCache()
{
	uint32_t i;
	for (i = 0; i < sInfo.unk184_0xA; i++) {
		if (sInfo.tocCaches[i].state == 0xFFFF)
			return i;
	}

	return 0xFFFF;
}

/* Open & restore */

void copyBTOC(uint32_t *dest, uint32_t *src, uint32_t maxVal)
{
	uint32_t i;
	uint32_t val;

	for (i = 0; i < (sInfo.tocPagesPerBlock * sGeometry.bytesPerPage) / sizeof(uint32_t); i++) {
		val = src[i];
		if (val >= maxVal)
			val = 0xFFFFFFFF;

		dest[i] = val;
	}
}

static BlockListNode *addBlockToList(BlockListNode *listHead, uint32_t blockNumber, uint32_t usn)
{
	BlockListNode *block = yaftl_alloc(sizeof(BlockListNode));

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

static uint32_t readBTOCPages(uint32_t offset, uint32_t *data, SpareData *spare, uint8_t disable_aes, uint8_t scrub, uint32_t max)
{
	uint32_t i, j, result;
	uint8_t *rawBuff = (uint8_t*)data;

	// Read all the BTOC (block table of contents) pages.
	for (i = 0; i < sInfo.tocPagesPerBlock; i++) {
		result = YAFTL_readPage(offset + i,
								&rawBuff[i * sGeometry.bytesPerPage],
								spare, disable_aes, 1, scrub);

		if (result)
			return result;
	}

	// Validate the data.
	for (j = 0; j < (sInfo.tocPagesPerBlock * sGeometry.bytesPerPage) / sizeof(uint32_t); j++) {
		if (data[j] >= max)
			data[j] = 0xFFFFFFFF;
	}

	return 0;
}

static uint32_t restoreIndexBlock(uint16_t blockNumber, uint8_t first)
{
	uint32_t i;
	uint32_t result, readSucceeded;
	uint32_t blockOffset = blockNumber * sGeometry.pagesPerSublk;
	uint32_t *BTOCBuffer = sInfo.pageBuffer2;
	SpareData *spare = sInfo.spareBuffer3;

	result = YAFTL_readPage(blockOffset, (uint8_t*)sInfo.tocPageBuffer, spare, 0, 1, 0);

	if (!result && !(spare->type & PAGETYPE_INDEX)) {
		bufferPrintf("yaftl: restoreIndexBlock called with a non-index block %d.\r\n", blockNumber);
		return 0;
	}

	// Read all the block-table-of-contents pages of this block.
	result = readBTOCPages(blockOffset + sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock,
							BTOCBuffer, spare, 0, 0, sInfo.tocArrayLength);

	if (result > 1) {
		// Read failed.
		readSucceeded = 0;
		sInfo.blockArray[blockNumber].unkn5 = 0x80;
	} else {
		readSucceeded = 1;
	}

	if (first) {
		// The first block has the largest USN.
		sInfo.latestIndexBlk.blockNum = blockNumber;
		sInfo.blockArray[blockNumber].status = BLOCKSTATUS_I_GC;
	} else {
		sInfo.blockArray[blockNumber].status = BLOCKSTATUS_I_ALLOCATED;
	}

	// If we read non-empty pages, and they are indeed BTOC:
	if (result == 0 && (spare->type & 0xC) == 0xC) {
		// Loop through the index pages in this block, and fill the information according to the BTOC.
		for (i = 0; i < sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock; i++) {
			uint32_t index = sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock - i - 1;

			if (BTOCBuffer[index] < sInfo.tocArrayLength
				&& sInfo.tocArray[BTOCBuffer[index]].indexPage == 0xFFFFFFFF) {
				// Valid index page number, and the data is missing. Fill in the information.
				sInfo.tocArray[BTOCBuffer[index]].indexPage = blockOffset + index;
				++sInfo.blockArray[blockNumber].validPagesINo;
				++sInfo.blockStats.numValidIPages;
			}
		}

		if (first) {
			// If this is the latest index block (i.e., the first in the list),
			// update latestIndexBlk.tocBuffer, and maxIndexUsn.
			sInfo.latestIndexBlk.usedPages = sGeometry.pagesPerSublk;
			copyBTOC(sInfo.latestIndexBlk.tocBuffer, BTOCBuffer, sInfo.tocArrayLength);

			if (spare->usn > sInfo.maxIndexUsn)
				sInfo.maxIndexUsn = spare->usn;
		}

		return 0;
	}

	// Else - reading the BTOC has failed. Must go through each page.
	if (first) {
		sInfo.latestIndexBlk.usedPages = sGeometry.pagesPerSublk;

		if (!readSucceeded)
			sInfo.latestIndexBlk.usedPages -= sInfo.tocPagesPerBlock;
	}

	// Loop through each non-BTOC page (i.e., index page) in this block.
	for (i = 0; i < sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock; i++) {
		uint32_t page = blockOffset
						+ sGeometry.pagesPerSublk
						- sInfo.tocPagesPerBlock
						- i - 1;

		result = YAFTL_readPage(page, (uint8_t*)sInfo.tocPageBuffer, spare, 0, 1, 0);

		uint32_t lpn = spare->lpn;

		if (result > 1 || (!result && (!(spare->type & PAGETYPE_INDEX) || lpn >= sInfo.tocArrayLength))) {
			// Read failed, or non-empty page with invalid data:
			// Not marked as index, or lpn is too high.
			sInfo.blockArray[blockNumber].unkn5 = 0x80;
			readSucceeded = 0;
			continue;
		}

		if (lpn != 0xFFFFFFFF) {
			// Valid LPN.
			if (first) {
				// First (most updated) index block - update latestIndexBlk.tocBuffer and maxIndexUsn.
				sInfo.latestIndexBlk.tocBuffer[page - blockOffset] = lpn;

				if (spare->usn > sInfo.maxIndexUsn)
					sInfo.maxIndexUsn = spare->usn;
			}

			if (sInfo.tocArray[lpn].indexPage == 0xFFFFFFFF) {
				// The information about this index page is missing - fill it in.
				sInfo.tocArray[lpn].indexPage = page;
				++sInfo.blockArray[blockNumber].validPagesINo;
				++sInfo.blockStats.numValidIPages;
			}

			// WTF? It must be another variable then. --Oranav
			readSucceeded = 0;
			continue;
		}

		if (first && readSucceeded)
			sInfo.latestIndexBlk.usedPages = page - blockOffset;
	}

	return 0;
}

static uint32_t restoreUserBlock(uint32_t blockNumber, uint32_t *bootBuffer, uint8_t firstBlock, uint32_t lpnOffset, uint32_t bootLength, BlockLpn *blockLpns)
{
	uint8_t readSucceeded;
	uint32_t result, i;
	uint32_t blockOffset = blockNumber * sGeometry.pagesPerSublk;
	uint32_t *buff = sInfo.pageBuffer2;
	SpareData *spare = sInfo.spareBuffer4;

	// If the lpn offset is off-boundaries, or this block is full of valid index pages - don't restore.
	if ((lpnOffset && (lpnOffset > blockLpns[blockNumber].lpnMax || blockLpns[blockNumber].lpnMin - lpnOffset >= bootLength))
		|| sInfo.blockArray[blockNumber].validPagesINo == sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock) {
		return 0;
	}

	if (lpnOffset && firstBlock) {
		memset(
			&sInfo.latestUserBlk.tocBuffer[sInfo.latestUserBlk.usedPages],
			0xFF,
			(sGeometry.pagesPerSublk - sInfo.latestUserBlk.usedPages) * sizeof(uint32_t));

		copyBTOC(buff, sInfo.latestUserBlk.tocBuffer, sInfo.totalPages);

		result = 0;
		spare->type = 0x8;
		readSucceeded = 1;
	} else {
		result = readBTOCPages(blockOffset + sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock, buff, spare, 0, 0, sInfo.totalPages);
		if (result > 1) {
			// Read failed.
			sInfo.blockArray[blockNumber].unkn5 = 0x80;
			readSucceeded = 0;
		} else {
			readSucceeded = 1;
		}
	}

	if (!lpnOffset) {
		// First time we're in this block.
		if (firstBlock) {
			sInfo.latestUserBlk.blockNum = blockNumber;
			sInfo.blockArray[blockNumber].status = BLOCKSTATUS_GC;
		} else {
			sInfo.blockArray[blockNumber].status = BLOCKSTATUS_ALLOCATED;
		}
	}

	if (result == 0 && (spare->type & 0xC) == 0x8) {
		// BTOC reading succeeded, process the information.
		uint32_t nUserPages;

		if (lpnOffset && firstBlock)
			nUserPages = sInfo.latestUserBlk.usedPages;
		else
			nUserPages = sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock;

		for (i = 0; i < nUserPages; i++) {
			uint32_t lpn = buff[nUserPages - 1 - i];

			if (lpn >= sInfo.totalPages)
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
				++sInfo.blockArray[blockNumber].validPagesINo;

				if (lpn != 0xFFFFFFFF && bootBuffer[lpn - lpnOffset] == 0xFFFFFFFF) {
					bootBuffer[lpn - lpnOffset] = blockOffset + nUserPages - 1 - i;
					++sInfo.blockArray[blockNumber].validPagesDNo;
					++sInfo.blockStats.numValidDPages;
				}
			}
		}

		if (firstBlock && !lpnOffset) {
			sInfo.latestUserBlk.usedPages = sGeometry.pagesPerSublk;
			copyBTOC(sInfo.latestUserBlk.tocBuffer, buff, sInfo.totalPages);
		}

		return 0;
	}

	// If we're here, then reading the user-block BTOC has failed.
	if (firstBlock) {
		if (result == 0)
			sInfo.latestUserBlk.usedPages = sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock;
		else
			sInfo.latestUserBlk.usedPages = sGeometry.pagesPerSublk;

		// Calculate the number of user pages in this block, and the number of index pages.
		for (i = 0; i < sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock; i++) {
			// If we find a non-user page, stop here.
			if (YAFTL_readPage(blockOffset + i, (uint8_t*)sInfo.tocPageBuffer, spare, 0, 1, 0) != 0
				|| !(spare->type & PAGETYPE_LBN)
				|| spare->lpn >= sInfo.totalPages) {
				break;
			}
		}

		sInfo.latestUserBlk.usedPages = i;
		sInfo.blockArray[blockNumber].validPagesINo = sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock - i;
	}

	// Read each user page in this block.
	for (i = 0; i < sInfo.latestUserBlk.usedPages; i++) {
		// Read the current page.
		result = YAFTL_readPage(blockOffset + sInfo.latestUserBlk.usedPages - 1 - i, (uint8_t*)sInfo.tocPageBuffer, spare, 0, 1, 0);

		// If the read has failed, or the page wasn't a user page, mark as error.
		if (result > 1
			|| (result == 0 && (!(spare->type & PAGETYPE_LBN) || spare->lpn >= sInfo.totalPages))) {
			if (!lpnOffset)
				++sInfo.blockArray[blockNumber].validPagesINo;
			sInfo.blockArray[blockNumber].unkn5 = 0x80;
			readSucceeded = 0;
		}

		if (!lpnOffset && spare->lpn == 0xFFFFFFFF)
			++sInfo.blockArray[blockNumber].validPagesINo;

		if (firstBlock && spare->lpn == 0xFFFFFFFF && readSucceeded) {
			sInfo.latestUserBlk.usedPages -= i + 1;
			continue;
		}

		if (!lpnOffset && spare->lpn < sInfo.totalPages) {
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
			sInfo.latestUserBlk.tocBuffer[sInfo.latestUserBlk.usedPages - 1 - i] = spare->lpn;

		if (lpnOffset > spare->lpn || spare->lpn - lpnOffset >= bootLength) {
			readSucceeded = 0;
			continue;
		}

		++sInfo.blockArray[blockNumber].validPagesINo;

		if (bootBuffer[spare->lpn - lpnOffset] == 0xFFFFFFFF) {
			bootBuffer[spare->lpn - lpnOffset] = blockOffset + sInfo.latestUserBlk.usedPages - 1 - i;
			++sInfo.blockArray[blockNumber].validPagesDNo;
			++sInfo.blockStats.numValidDPages;
		}

		readSucceeded = 0;
	}

	return 0;
}

static uint32_t YAFTL_Restore(uint8_t ftlCtrlBlockPresent)
{
	uint32_t i, j, result = 0;
	uint8_t *tempBuffer;

	bufferPrintf("yaftl: context is invalid. performing a read-only restore...\r\n");

	tempBuffer = yaftl_alloc(sGeometry.bytesPerPage);
	if (!tempBuffer) {
		bufferPrintf("yaftl: out of memory.\r\n");
		return EIO;
	}

	// Reset blockArray.
	for (i = 0; i < sGeometry.numBlocks; i++) {
		sInfo.blockArray[i].validPagesDNo = 0;
		sInfo.blockArray[i].validPagesINo = 0;

		if (sInfo.blockArray[i].status != BLOCKSTATUS_FTLCTRL
			&& sInfo.blockArray[i].status != BLOCKSTATUS_FTLCTRL_SEL) {
			sInfo.blockArray[i].status = 0;
		}
	}

	// Reset tocArray and blockStats.
	sInfo.latestIndexBlk.blockNum = 0xFFFFFFFF;
	memset(sInfo.tocArray, 0xFF, sizeof(TOCStruct) * sInfo.tocArrayLength);
	memset(&sInfo.blockStats, 0, sizeof(BlockStats));

	// Add all available blocks to the block lists.
	BlockListNode *userListHead = NULL, *indexListHead = NULL;

	for (i = 0; i < sGeometry.numBlocks; i++) {
		uint8_t status = sInfo.blockArray[i].status;

		// Ignore FTL blocks.
		if (ftlCtrlBlockPresent && (status == BLOCKSTATUS_FTLCTRL || status == BLOCKSTATUS_FTLCTRL_SEL))
			continue;

		// Try to find a readable page.
		result = 0xFFFFFFFF;
		for (j = 0; j < sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock; j++) {
			result = YAFTL_readPage(i * sGeometry.pagesPerSublk + j,
						tempBuffer, sInfo.spareBuffer10, 0, 1, 0);

			if (result <= 1)
				break;
		}

		// If no readable page was found, skip this block.
		if (result > 1)
			continue;

		uint8_t type = sInfo.spareBuffer10->type;

		if ((type != 0xFF || sInfo.spareBuffer10->lpn != 0xFFFFFFFF) && result != 1) {
			// User or index page. Determine which.
			if (type & PAGETYPE_INDEX) {
				if (!(indexListHead = addBlockToList(indexListHead, i, sInfo.spareBuffer10->usn)))
					system_panic("yaftl: out of memory.\r\n");
			} else if (type & PAGETYPE_LBN) {
				if (!(userListHead = addBlockToList(userListHead, i, sInfo.spareBuffer10->usn)))
					system_panic("yaftl: out of memory.\r\n");
			} else {
				bufferPrintf("yaftl: warning - block %d doesn't belong to anything (type 0x%02x).\r\n", i, type);
			}
		} else {
			sInfo.blockArray[i].status = BLOCKSTATUS_FREE;
			sInfo.blockArray[i].unkn5 = 1;
		}
	}

	sInfo.unkn_0x2A = sInfo.unk184_0xA;

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
	if (sInfo.latestIndexBlk.blockNum == 0xFFFFFFFF) {
		for (i = 0; i < sGeometry.numBlocks; i++) {
			if (sInfo.blockArray[i].status == BLOCKSTATUS_FREE) {
				sInfo.latestIndexBlk.blockNum = i;
				sInfo.latestIndexBlk.usedPages = 0;
				memset(sInfo.latestIndexBlk.tocBuffer, 0xFF, sInfo.tocPagesPerBlock * sGeometry.bytesPerPage);
				sInfo.blockArray[i].status = BLOCKSTATUS_I_GC;
				break;
			}
		}
	}

	// Restore user blocks.
	if (userListHead) {
		BlockListNode *currUserNode, *prevUserNode;
		BlockLpn *blockLpns = (BlockLpn*)yaftl_alloc(sGeometry.numBlocks * sizeof(BlockLpn));

		// TODO: figure this out.
		uint32_t *bootBuffer = (uint32_t*)yaftl_alloc(0x800000);
		uint32_t entriesInBootBuffer = 0x800000 / sizeof(uint32_t);

		if (!blockLpns || !bootBuffer) {
			bufferPrintf("yaftl: out of memory.\r\n");

			if (blockLpns)
				free(blockLpns);
			else if (bootBuffer)
				free(bootBuffer);

			goto restore_error;
		}

		memset(blockLpns, 0xFF, sGeometry.numBlocks * sizeof(BlockLpn));

		for (i = 0; i < sInfo.totalPages; i += entriesInBootBuffer) {
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
				if (i + entriesInBootBuffer >= sInfo.totalPages) {
					free(prevUserNode);
				}
			}

			memset(tempBuffer, 0xFF, sGeometry.bytesPerPage);

			uint32_t pagesInBootBuffer = entriesInBootBuffer / sInfo.tocEntriesPerPage;

			for (j = 0; j < pagesInBootBuffer && (i + j * sInfo.tocEntriesPerPage) < sInfo.totalPages; j++) {
				uint32_t bootBufferOffset = j * sInfo.tocEntriesPerPage;
				uint32_t tocNum = i / sInfo.tocEntriesPerPage + j;
				uint32_t indexPage = sInfo.tocArray[tocNum].indexPage;
				uint32_t cacheNum;

				memset(tempBuffer, 0xFF, sGeometry.bytesPerPage);

				if (memcmp(&bootBuffer[bootBufferOffset], tempBuffer, sGeometry.bytesPerPage) != 0) {
					if (indexPage != 0xFFFFFFFF && YAFTL_readPage(indexPage, tempBuffer, 0, 0, 1, 0) != 0) {
						bufferPrintf("yaftl: index page is unreadable at 0x%08x\r\n", indexPage);
						memset(tempBuffer, 0xFF, sGeometry.bytesPerPage);
					}

					if (memcmp(&bootBuffer[bootBufferOffset], tempBuffer, sGeometry.bytesPerPage) != 0) {
						if (indexPage != 0xFFFFFFFF) {
							--sInfo.blockArray[indexPage / sGeometry.pagesPerSublk].validPagesINo;
							--sInfo.blockStats.numValidIPages;
						}

						cacheNum = findFreeTOCCache();
						if (cacheNum == 0xFFFF) {
							free(bootBuffer);
							free(blockLpns);
							bufferPrintf("yaftl: out of TOC caches, but restore might have already succeeded.\r\n");
							goto restore_final;
						}

						TOCCache *tocCache = &sInfo.tocCaches[cacheNum];
						memcpy(tocCache->buffer, &bootBuffer[bootBufferOffset], sGeometry.bytesPerPage);
						tocCache->state = 1;
						tocCache->page = tocNum;
						sInfo.tocArray[tocNum].cacheNum = cacheNum;
						sInfo.tocArray[tocNum].indexPage = 0xFFFFFFFF;
					}
				} else {
					if (indexPage != 0xFFFFFFFF && sInfo.blockArray[indexPage].validPagesINo)
						--sInfo.blockArray[indexPage].validPagesINo;

					cacheNum = sInfo.tocArray[tocNum].cacheNum;
					if (cacheNum != 0xFFFF) {
						sInfo.tocCaches[cacheNum].state = 0xFFFF;
						sInfo.tocCaches[cacheNum].useCount = 0;
					}

					sInfo.tocArray[tocNum].indexPage = 0xFFFFFFFF;
					sInfo.tocArray[tocNum].cacheNum = 0xFFFF;
				}
			}
		}

		free(bootBuffer);
		free(blockLpns);

		YAFTL_SetupFreeAndAllocd();

		for (i = 0; i < sGeometry.numBlocks; i++) {
			uint8_t status = sInfo.blockArray[i].status;

			if (status == BLOCKSTATUS_ALLOCATED || status == BLOCKSTATUS_GC)
				sInfo.blockArray[i].validPagesINo = 0;
		}
	} else {
		// No user pages. Find a free block, and choose it to be the latest user page.
		for (i = 0; i < sGeometry.numBlocks && sInfo.blockArray[i].status != BLOCKSTATUS_FREE; i++);

		if (i < sGeometry.numBlocks) {
			sInfo.latestUserBlk.blockNum = i;
			sInfo.blockArray[i].status = BLOCKSTATUS_GC;
		}

		sInfo.latestUserBlk.usedPages = 0;
		memset(sInfo.latestUserBlk.tocBuffer, 0xFF, sInfo.tocPagesPerBlock * sGeometry.bytesPerPage);
		YAFTL_SetupFreeAndAllocd();
	}

	sInfo.blockStats.field_1C = 1;

restore_final:
	YAFTL_LoopThingy();

	if (sInfo.ftlCxtPage != 0xFFFFFFFF) {
		sInfo.ftlCxtPage =
			sGeometry.pagesPerSublk
			+ (sInfo.ftlCxtPage / sGeometry.pagesPerSublk) * sGeometry.pagesPerSublk
			- 1;
	}

	// I know this seems strange, but that's the logic they do here. --Oranav
	for (i = 0; i < sGeometry.numBlocks; i++) {
		sInfo.maxBlockEraseCount = 0;
		sInfo.minBlockEraseCount = 0xFFFFFFFF;

		if (sInfo.blockArray[i].status != BLOCKSTATUS_FTLCTRL
			&& sInfo.blockArray[i].status != BLOCKSTATUS_FTLCTRL_SEL) {
			if (sInfo.blockArray[i].eraseCount)
				sInfo.maxBlockEraseCount = sInfo.blockArray[i].eraseCount;

			if (sInfo.blockArray[i].eraseCount < sInfo.minBlockEraseCount)
				sInfo.minBlockEraseCount = sInfo.blockArray[i].eraseCount;
		}
	}

	sInfo.pagesAvailable = sInfo.unk188_0x63 * (sInfo.totalPages - 1) / 0x64u;
	sInfo.bytesPerPage = sGeometry.bytesPerPage;

	bufferPrintf("yaftl: restore done.\r\n");

	free(tempBuffer);
	return 0;

restore_error:
	free(tempBuffer);
	return result;
}

static uint32_t YAFTL_Open(uint32_t signature_bit)
{
	uint16_t ftlCtrlBlockBuffer[3];
	uint32_t versionSupported = 1;

	memset(sInfo.tocArray, 0xFF, sInfo.tocArrayLength * sizeof(TOCStruct));
	memset(sInfo.blockArray, 0xFF, sGeometry.numBlocks * sizeof(BlockStruct));

	uint32_t i;
	for (i = 0; i < sGeometry.numBlocks; i++) {
		sInfo.blockArray[i].eraseCount = 0;
		sInfo.blockArray[i].validPagesDNo = 0;
		sInfo.blockArray[i].validPagesINo = 0;
		sInfo.blockArray[i].readCount = 0;
		sInfo.blockArray[i].status = 0;
		sInfo.blockArray[i].unkn5 = 0;
	}

	memcpy(ftlCtrlBlockBuffer, vfl_get_ftl_ctrl_block(vfl), sizeof(ftlCtrlBlockBuffer));

	if (ftlCtrlBlockBuffer[0] == 0xFFFF) {
		// No FTL ctrl block was found.
		return YAFTL_Restore(0);
	}

	for (i = 0; i < 3; i++) {
		sInfo.FTLCtrlBlock[i] = ftlCtrlBlockBuffer[i];
		sInfo.blockArray[ftlCtrlBlockBuffer[i]].status = BLOCKSTATUS_FTLCTRL;
	}

	if (!sInfo.pageBuffer) {
		system_panic("yaftl: This can't happen. This shouldn't happen. Whatever, it's doing something else then.\r\n");
		return -1;
	}

	// Find the latest (with max USN) FTL context block.
	uint32_t maxUsn = 0;
	uint32_t ftlCtrlBlock = 0;

	for (i = 0; i < 3; i++) {
		if (YAFTL_readPage(ftlCtrlBlockBuffer[i] * sGeometry.pagesPerSublk, sInfo.pageBuffer, sInfo.spareBuffer6, 0, 0, 0) == 0) {
			if (sInfo.spareBuffer6->usn != 0xFFFFFFFF && sInfo.spareBuffer6->usn > maxUsn) {
				maxUsn = sInfo.spareBuffer6->usn;
				ftlCtrlBlock = ftlCtrlBlockBuffer[i];
				sInfo.selCtrlBlockIndex = i;
			}
		}
	}

	uint32_t some_val;

	if (!maxUsn) {
		sInfo.blockArray[ftlCtrlBlockBuffer[0]].status = BLOCKSTATUS_FTLCTRL_SEL;
		sInfo.selCtrlBlockIndex = 0;
		some_val = 5;
	} else {
		sInfo.blockArray[ftlCtrlBlock].status = BLOCKSTATUS_FTLCTRL_SEL;
		i = 0;
		while (TRUE) {
			if (i >= sGeometry.pagesPerSublk - sInfo.ctrlBlockPageOffset
				|| YAFTL_readPage(
					sGeometry.pagesPerSublk * ftlCtrlBlock + i,
					sInfo.pageBuffer,
					sInfo.spareBuffer6,
					0, 1, 0) != 0)
			{
				bufferPrintf("yaftl: no valid context slot was found, a restore is definitely needed.\r\n");
				sInfo.ftlCxtPage = ~sInfo.ctrlBlockPageOffset + sGeometry.pagesPerSublk * ftlCtrlBlock + i;
				sInfo.field_78 = 0;
				YAFTL_readCxtInfo(sInfo.ftlCxtPage, sInfo.pageBuffer, 0, &versionSupported);
				some_val = 5;
				break;
			}

			if (YAFTL_readPage(sGeometry.pagesPerSublk * ftlCtrlBlock + i + sInfo.ctrlBlockPageOffset, sInfo.pageBuffer, sInfo.spareBuffer6, 0, 1, 0) == 1) {
				sInfo.ftlCxtPage = sGeometry.pagesPerSublk * ftlCtrlBlock + i;
				if (YAFTL_readCxtInfo(sInfo.ftlCxtPage, sInfo.pageBuffer, 1, &versionSupported) != 0)
					some_val = 5;
				else
					some_val = 0;
				sInfo.field_78 = 0;
				break;
			}

			i += sInfo.ctrlBlockPageOffset + 1;
		}
	}

	bufferPrintf("yaftl: ftl context is at logical page %d\r\n", sInfo.ftlCxtPage);

	if (versionSupported != 1) {
		bufferPrintf("yaftl: unsupported low-level format version.\r\n");
		return -1;
	}

	int restoreNeeded = 0;

	if (some_val || signature_bit) {
		restoreNeeded = 1;
		sInfo.unk184_0xA *= 100;
	}

	sInfo.tocCaches = yaftl_alloc(sInfo.unk184_0xA * sizeof(TOCCache));
	if (!sInfo.tocCaches) {
		bufferPrintf("yaftl: error allocating memory\r\n");
		return -1;
	}

	bufzone_init(&sInfo.segment_info_temp);

	for (i = 0; i < sInfo.unk184_0xA; i++)
		sInfo.tocCaches[i].buffer = bufzone_alloc(&sInfo.segment_info_temp, sGeometry.bytesPerPage);

	if (FAILED(bufzone_finished_allocs(&sInfo.segment_info_temp)))
		return -1;

	for (i = 0; i < sInfo.unk184_0xA; i++) {
		sInfo.tocCaches[i].buffer = bufzone_rebase(&sInfo.segment_info_temp, sInfo.tocCaches[i].buffer);
		sInfo.tocCaches[i].state = 0xFFFF;
		sInfo.tocCaches[i].page = 0xFFFF;
		sInfo.tocCaches[i].useCount = 0;
		memset(sInfo.tocCaches[i].buffer, 0xFF, sGeometry.bytesPerPage);
	}

	if (FAILED(bufzone_finished_rebases(&sInfo.segment_info_temp)))
		return -1;

	if (restoreNeeded)
		return YAFTL_Restore(1);

	sInfo.pagesAvailable = sInfo.unk188_0x63 * (sInfo.totalPages - 1) / 100;
	sInfo.bytesPerPage = sGeometry.bytesPerPage;

	sInfo.maxBlockEraseCount = 0;
	sInfo.minBlockEraseCount = 0xFFFFFFFF;

	for (i = 0; i < sGeometry.numBlocks; i++) {
		if (sInfo.blockArray[i].status != BLOCKSTATUS_FTLCTRL
			&& sInfo.blockArray[i].status != BLOCKSTATUS_FTLCTRL_SEL)
		{
			if (sInfo.blockArray[i].eraseCount > sInfo.maxBlockEraseCount)
				sInfo.maxBlockEraseCount = sInfo.blockArray[i].eraseCount;

			if (sInfo.blockArray[i].eraseCount < sInfo.minBlockEraseCount)
				sInfo.minBlockEraseCount = sInfo.blockArray[i].eraseCount;
		}
	}

	YAFTL_SetupFreeAndAllocd();
	YAFTL_LoopThingy();

	bufferPrintf("yaftl: successfully opened!\r\n");

	return 0;
}

/* Read */

static int YAFTL_readMultiPages(uint32_t* pagesArray, uint32_t nPages, uint8_t* dataBuffer, SpareData* metaBuffers, uint32_t disableAES, uint32_t scrub)
{
	uint32_t block, page, i, j;
	int ret, succeeded, status = 0, unkn = 0;
	int pagesToRead = nPages;

	if (metaBuffers == NULL)
		metaBuffers = sInfo.buffer20;

	for (i = 0; pagesToRead > sGeometry.pagesPerSublk; i++) {
		//ret = VFL_ReadScatteredPagesInVb(&buf[i * sGeometry.pagesPerSublk],sGeometry.pagesPerSublk, databuffer + i * sGeometry.pagesPerSublk * sGeometry.bytesPerPage, metabuf_array, &unkn, 0, a4, &status);
		ret = 0; // For now. --Oranav

		if (ret) {
			if (status)
				return 0;
		} else {
			//bufferPrintf("yaftl: _readMultiPages: We got read failure!!\r\n");
			succeeded = 1;

			for (j = 0; j < sGeometry.pagesPerSublk; j++) {
				page = i * sGeometry.pagesPerSublk + j;
				ret = YAFTL_readPage(
						pagesArray[page],
						&dataBuffer[page * sGeometry.bytesPerPage],
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

		block = pagesArray[(i + 1) * sGeometry.pagesPerSublk] / sGeometry.pagesPerSublk;
		sInfo.blockArray[block].readCount++;

		pagesToRead -= sGeometry.pagesPerSublk;
	}

	if (pagesToRead == 0)
		return 1;

	block = pagesArray[i * sGeometry.pagesPerSublk] / sGeometry.pagesPerSublk;
	sInfo.blockArray[block].readCount++;

	unkn = 0;
	// ret = VFL_ReadScatteredPagesInVb(&buf[i * sGeometry.pagesPerSublk], pagesToRead % sGeometry.pagesPerSublk, databuffer + i * sGeometry.pagesPerSublk * sGeometry.bytesPerPage, metabuf_array, &unkn, 0, a4, &status);
	ret = 0; // For now. --Oranav

	if (ret)
		return !status;
	else {
		//bufferPrintf("yaftl:_readMultiPages: We got read failure!!\r\n");
		succeeded = 1;

		for (j = 0; j != pagesToRead; j++) {
			page = i * sGeometry.pagesPerSublk + j;
			ret = YAFTL_readPage(
					pagesArray[page],
					&dataBuffer[page * sGeometry.bytesPerPage],
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

	if ((lpn + nPages) >= sInfo.totalPages)
		return EINVAL;

	nand_device_set_ftl_region(vfl->get_device(vfl), lpn, 0, nPages, pBuf);

	sInfo.lastTOCPageRead = 0xFFFFFFFF;

	// Omitted for now. --Oranav
	// sInfo.unk33C = 0;

	testMode = (!pBuf && nPages == 1);

	if (testMode)
		*sInfo.unknBuffer3_ftl = 0xFFFFFFFF;

	while (pagesRead != nPages) {
		tocPageNum = (lpn + pagesRead) / sInfo.tocEntriesPerPage;
		if ((sInfo.tocArray[tocPageNum].cacheNum == 0xFFFF) && (sInfo.tocArray[tocPageNum].indexPage == 0xFFFFFFFF)) {
			if (testMode)
				return 0;

			h2fmi_set_emf(emf, 0);
			if (!YAFTL_readMultiPages(sInfo.unknBuffer3_ftl, numPages, data, 0, 0, 1)
				|| !YAFTL_verifyMetaData(lpn + (data - pBuf) / sGeometry.bytesPerPage, sInfo.buffer20, numPages))
				ret = EIO;
			h2fmi_set_emf(0, 0);

			numPages = 0;
			readBuf += sGeometry.bytesPerPage;
		} else {
			// Should lookup TOC.
			tocCacheNum = sInfo.tocArray[tocPageNum].cacheNum;

			if (tocCacheNum != 0xFFFF) {
				// There's a cache for it!
				sInfo.tocCaches[tocCacheNum].useCount++;
				tocEntry = sInfo.tocCaches[tocCacheNum].buffer[(lpn + pagesRead) % sInfo.tocEntriesPerPage];
			} else {
				// TOC cache isn't valid, find whether we can cache the data.
				if ((freeTOCCacheNum = findFreeTOCCache()) != 0xFFFF)
				{
					// There's a free cache.
					ret = YAFTL_readPage(
							sInfo.tocArray[tocPageNum].indexPage,
							(uint8_t*)sInfo.tocCaches[freeTOCCacheNum].buffer,
							0, 0, 1, 1);

					if (ret != 0)
						goto YAFTL_READ_RETURN;

					sInfo.unkn_0x2A--;
					sInfo.tocCaches[freeTOCCacheNum].state = 2;
					sInfo.tocCaches[freeTOCCacheNum].useCount = 1;
					sInfo.tocCaches[freeTOCCacheNum].page = tocPageNum;
					sInfo.tocArray[tocPageNum].cacheNum = freeTOCCacheNum;

					tocEntry = sInfo.tocCaches[freeTOCCacheNum].buffer[(lpn + pagesRead) % sInfo.tocEntriesPerPage];
				} else {
					// No free cache. Is this the last TOC page we read?
					if (sInfo.lastTOCPageRead != tocPageNum) {
						// No, it isn't. Read it.
						if (YAFTL_readPage(
								sInfo.tocArray[tocPageNum].indexPage,
								(uint8_t*)sInfo.tocPageBuffer,
								0, 0, 1, 1) != 0)
							goto YAFTL_READ_RETURN;

						sInfo.lastTOCPageRead = tocPageNum;
					}

					tocEntry = sInfo.tocPageBuffer[(lpn + pagesRead) % sInfo.tocEntriesPerPage];
				}
			}

			// Okay, obtained the TOC entry.
			if (tocEntry == 0xFFFFFFFF) {
				if (testMode)
					return 0;

				h2fmi_set_emf(emf, 0);
				if (!YAFTL_readMultiPages(sInfo.unknBuffer3_ftl, numPages, data, 0, 0, 1)
					|| !YAFTL_verifyMetaData(lpn + (data - pBuf) / sGeometry.bytesPerPage,
									sInfo.buffer20,
									numPages))
					ret = EIO;
				h2fmi_set_emf(0, 0);

				numPages = 0;
				data = NULL;
			} else {
				sInfo.unknBuffer3_ftl[numPages] = tocEntry;
				numPages++;

				if (data == NULL)
					data = readBuf;

				if (numPages == sGeometry.pagesPerSublk) {
					if (testMode)
						return 0;

					h2fmi_set_emf(emf, 0);
					if (!YAFTL_readMultiPages(sInfo.unknBuffer3_ftl, numPages, data, 0, 0, 1)
						|| !YAFTL_verifyMetaData(lpn + (data - pBuf) / sGeometry.bytesPerPage,
										sInfo.buffer20,
										numPages))
						ret = EIO;
					h2fmi_set_emf(0, 0);

					numPages = 0;
					data = NULL;
				}
			}

			readBuf += sGeometry.bytesPerPage;
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
	if (!YAFTL_readMultiPages(sInfo.unknBuffer3_ftl, numPages, data, 0, 0, 1)
		|| !YAFTL_verifyMetaData(lpn + (data - pBuf) / sGeometry.bytesPerPage,
						sInfo.buffer20,
						numPages))
		ret = EIO;
	h2fmi_set_emf(0, 0);

YAFTL_READ_RETURN:
	nand_device_set_ftl_region(vfl->get_device(vfl), 0, 0, 0, 0);
	return ret;
}

/* Write */

static int YAFTL_Write(uint32_t _pageStart, uint32_t _numPages, uint8_t* _pBuf)
{
	uint32_t i;
	int finished = FALSE;

	struct {
		uint32_t pageStart;
		uint32_t numPages;
		uint8_t *pBuf;
	} try;

	// Write index TOC if necessary
	if (sInfo.field_78) {
		YAFTL_writeIndexTOC();
		sInfo.field_78 = 0;
	}

	if (_pageStart + _numPages >= sInfo.totalPages)
		return EINVAL;

	nand_device_set_ftl_region(vfl->get_device(vfl),
			_pageStart, 0, _numPages, _pBuf);
	YAFTL_InvalidatePages(_pageStart, _numPages);

	try.pageStart = _pageStart;
	try.pBuf = _pBuf;
	try.numPages = _numPages;

	while (!finished) {
		uint32_t currentPage = try.pageStart;
		uint32_t toWrite = try.numPages;
		uint8_t* pBufOffset = try.pBuf;

		sub_80606834(try.numPages);
		
		while (toWrite != 0) {
			error_t result = SUCCESS;
			uint32_t maxWritePages;
			uint32_t physWriteLimit;
			uint32_t* cache;
			uint32_t cacheNum;

			// Check for overflow; will happen when the current writeable block
			// is filled
			if (sInfo.latestUserBlk.usedPages >= sGeometry.pagesPerSublk -
					sInfo.tocPagesPerBlock) {
				sub_80600A68(1);
				sub_8060111C(sInfo.latestUserBlk.blockNum, 0,
						BLOCKSTATUS_ALLOCATED);

				try.pageStart = currentPage;
				try.numPages = toWrite;
				try.pBuf = pBufOffset;
			}

			physWriteLimit = 32 * sGeometry.total_banks_ftl -
				sInfo.latestUserBlk.usedPages % sGeometry.total_banks_ftl;

			maxWritePages = sGeometry.pagesPerSublk -
				(sInfo.tocPagesPerBlock + sInfo.latestUserBlk.usedPages);

			if (maxWritePages > physWriteLimit)
				maxWritePages = physWriteLimit;

			if (maxWritePages > toWrite)
				maxWritePages = toWrite;

			sub_8060E564(sInfo.spareBuffer19, currentPage, maxWritePages);

			// TODO: Use VSVFL_writeMultiplePagesInVb.
			for (i = 0; i < maxWritePages && !FAILED(result); ++i) {
				uint32_t page = sInfo.latestUserBlk.blockNum
					* sGeometry.pagesPerSublk + i;

				// FIXME: Should we really use sGeometry.bytesPerPage???
				result = vfl_write_single_page(vfl, page,
						&pBufOffset[sGeometry.bytesPerPage * i],
						(uint8_t*)&sInfo.spareBuffer19[i], TRUE);
			}

			if (FAILED(result)) {
				sub_8060111C(sInfo.latestUserBlk.blockNum, 0,
						BLOCKSTATUS_ALLOCATED);
				YAFTL_InvalidatePages(try.pageStart, try.numPages);

				if (sInfo.blockArray[sInfo.latestUserBlk.blockNum]
						.status != BLOCKSTATUS_FREE) {
					YAFTL_GarbageCollection(sInfo.latestUserBlk.blockNum, 1);
				}

				break;
			}

			// TODO: Maybe a better code logic?
			// This iteration was successful, so set finished to TRUE.
			finished = TRUE;

			// Update latestUserBlk.tocBuffer
			for (i = 0; i < maxWritePages; ++i) {
				sInfo.latestUserBlk.tocBuffer[sInfo.latestUserBlk.usedPages
					+ i] = currentPage + i;
			}

			// Update L2V
			L2V_Update(currentPage, maxWritePages, sInfo.latestUserBlk.usedPages
					+ (sInfo.latestUserBlk.blockNum
					* sGeometry.pagesPerSublk));

			// Update caches
			cache = YAFTL_RefreshTOCCaches(currentPage, &cacheNum);
			for (i = 0; i < maxWritePages; ++i) {
				cache[cacheNum++] = sInfo.latestUserBlk.usedPages
					+ (sInfo.latestUserBlk.blockNum
					* sGeometry.pagesPerSublk) + i;

				// If we've reached the end of this TOC cache, get the next one.
				if (cacheNum >= sInfo.tocEntriesPerPage) {
					cache = YAFTL_RefreshTOCCaches(currentPage + i + 1,
							&cacheNum);
				}
			}

			// Update info data
			currentPage += maxWritePages;
			sInfo.latestUserBlk.usedPages += maxWritePages;
			sInfo.blockStats.numValidDPages += maxWritePages;
			sInfo.blockArray[sInfo.latestUserBlk.blockNum].validPagesDNo
				+= maxWritePages;

			// TODO: qword_80618510 += maxWritePages;
			
			toWrite -= maxWritePages;
			pBufOffset += sGeometry.bytesPerPage * maxWritePages;
		}
	}

	// TODO: ++qword_806184D0 += _numPages;
	sInfo.field_DC[2] += _numPages;
	if (sInfo.field_DC[2] >= sGeometry.total_usable_pages / 2
			|| sInfo.field_DC[3] >= sGeometry.numBlocks / 2) {
		YAFTL_Flush();
	}

	nand_device_set_ftl_region(vfl->get_device(vfl), 0, 0, 0, 0);
	return 0;
}

/* MTD */

static error_t yaftl_read_mtd(mtd_t *_dev, void *_dest, uint64_t _off, int _amt)
{
	uint32_t emf = h2fmi_get_emf();
	uint8_t* curLoc = (uint8_t*) _dest;
	uint32_t block_size = sInfo.bytesPerPage;
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
	return sInfo.bytesPerPage;
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
