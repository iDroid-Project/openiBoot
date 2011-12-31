#include "openiboot.h"
#include "util.h"
#include "vfl.h"
#include "ftl/yaftl_common.h"
#include "ftl/yaftl_gc.h"

FTLStats sFTLStats;
YAFTLInfo sInfo;
YAFTLGeometry sGeometry;
YAFTLStats sStats;
uint32_t yaftl_inited = 0;
vfl_device_t* vfl = 0;

/* Internals */

static void deallocBTOC(uint32_t* btoc)
{
	uint32_t i;

	for (i = 0; i != sInfo.numBtocCaches; ++i) {
		if (sInfo.btocCaches[i] == btoc) {
			sInfo.btocCacheMissing |= 1 << i;
			return;
		}
	}

	system_panic("YAFTL: couldn't deallocate BTOC %p\r\n", btoc);
}

static void setupIndexSpare(SpareData* _pSpare, uint32_t _lpn)
{
	// TODO: Load lastWrittenBlock properly (i.e. in YAFTL_Open)!!!
	if (sInfo.lastWrittenBlock != sInfo.latestIndexBlk.blockNum) {
		sInfo.lastWrittenBlock = sInfo.latestIndexBlk.blockNum;
		++sInfo.maxIndexUsn;
	}

	_pSpare->lpn = _lpn;
	_pSpare->type = PAGETYPE_INDEX;
	_pSpare->usn = sInfo.maxIndexUsn;
}

static int writeIndexPage(void* _pBuf, SpareData* _pSpare)
{
	uint32_t page;

	while (TRUE) {
		if (sInfo.latestIndexBlk.usedPages >= sGeometry.pagesPerSublk -
				sInfo.tocPagesPerBlock) {
			// Current index block is full, obtain a fresh one.
			YAFTL_closeLatestBlock(FALSE);
			YAFTL_allocateNewBlock(FALSE);

			if (sInfo.latestIndexBlk.usedPages != 0) {
				system_panic("YAFTL: writeIndexPage expected a fresh index "
						"block, but %d of its pages are used\r\n",
						sInfo.latestIndexBlk.usedPages);
			}
		}

		page = sInfo.latestIndexBlk.blockNum * sGeometry.pagesPerSublk +
			sInfo.latestIndexBlk.usedPages;

		_pSpare->usn = sInfo.maxIndexUsn;
		sInfo.latestIndexBlk.tocBuffer[sInfo.latestIndexBlk.usedPages] =
			_pSpare->lpn;

		if (YAFTL_writePage(page, (uint8_t*)_pBuf, _pSpare) == 0)
			break;

		gcListPushBack(&sInfo.gc.index.list, sInfo.latestIndexBlk.blockNum);
		YAFTL_allocateNewBlock(FALSE);
	}

	++sInfo.blockArray[sInfo.latestIndexBlk.blockNum].validPagesINo;
	++sInfo.blockStats.numValidIPages;
	++sStats.indexPages;
	++sInfo.latestIndexBlk.usedPages;
	sInfo.tocArray[_pSpare->lpn].indexPage = page;
	return 0;
}

/* Externals */

void YAFTL_setupCleanSpare(SpareData* _spare_ptr)
{
	_spare_ptr->usn = ++sInfo.maxIndexUsn;
	_spare_ptr->type = PAGETYPE_FTL_CLEAN;
}

int YAFTL_readMultiPages(uint32_t* pagesArray, uint32_t nPages, uint8_t* dataBuffer, SpareData* metaBuffers, uint32_t disableAES, uint32_t scrub)
{
	uint32_t block, page, i, j;
	int ret, succeeded, status = 0, unkn = 0;
	int pagesToRead = nPages;

	if (metaBuffers == NULL)
		metaBuffers = sInfo.buffer20;

	for (i = 0; pagesToRead > sGeometry.pagesPerSublk; i++) {
		// TODO
		//ret = VFL_ReadScatteredPagesInVb(&buf[i * sGeometry.pagesPerSublk],sGeometry.pagesPerSublk, databuffer + i * sGeometry.pagesPerSublk * sGeometry.bytesPerPage, metabuf_array, &unkn, 0, a4, &status);
		ret = 0;

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

int YAFTL_writeMultiPages(uint32_t _block, uint32_t _start, size_t _numPages, uint8_t* _pBuf, SpareData* _pSpare, uint8_t _scrub)
{
	error_t result;
	uint32_t i;

	if (sInfo.field_78) {
		YAFTL_writeIndexTOC();
		sInfo.field_78 = FALSE;
	}

	for (i = 0; i < _numPages; ++i) {
		result = vfl_write_single_page(
			vfl,
			_block * sGeometry.pagesPerSublk + _start + i,
			_pBuf,
			(uint8_t*)_pSpare,
			_scrub);

		if (FAILED(result)) {
			bufferPrintf("YAFTL: writeMultiPages got write failure: block %d, "
					"page %d, result %08x, block status %d\r\n", _block,
					_start + i, result, sInfo.blockArray[_block].status);

			return 1;
		}
	}

	return 0;
}

void YAFTL_allocateNewBlock(uint8_t isUserBlock)
{
	uint32_t bestBlk = 0xFFFFFFFF;
	uint32_t minEraseCnt = 0xFFFFFFFF;
	uint32_t currBlk;

	++sInfo.field_DC[0];

	if (isUserBlock)
		sInfo.latestUserBlk.usn = sInfo.maxIndexUsn;
	else
		sInfo.latestIndexBlk.usn = sInfo.maxIndexUsn;

	for (currBlk = 0; currBlk < sGeometry.numBlocks; ++currBlk) {
		uint8_t status = sInfo.blockArray[currBlk].status;
		uint8_t unkn5 = sInfo.blockArray[currBlk].unkn5;

		// Skip non-free blocks -- we can't use them.
		if (status != BLOCKSTATUS_FREE)
			continue;

		// Erase blocks which need it.
		if ((isUserBlock && unkn5 == 2) || (!isUserBlock && unkn5 == 4)) {
			if (sInfo.field_78) {
				YAFTL_writeIndexTOC();
				sInfo.field_78 = FALSE;
			}

			if (FAILED(vfl_erase_single_block(vfl, currBlk, TRUE))) {
				system_panic(
						"YAFTL: YAFTL_allocateNewBlock failed to erase block %d\r\n",
						currBlk);
			}

			sInfo.blockArray[currBlk].unkn5 = 0;
			++sInfo.blockArray[currBlk].eraseCount;
			++sInfo.field_DC[3];
			
			if (sInfo.blockArray[currBlk].eraseCount > sInfo.maxBlockEraseCount)
				sInfo.maxBlockEraseCount = sInfo.blockArray[currBlk].eraseCount;
		}

		if (unkn5 != 2 && unkn5 != 5) {
			// Candidate found.
			if (sInfo.blockArray[currBlk].eraseCount < minEraseCnt) {
				if (sInfo.blockArray[currBlk].validPagesINo) {
					system_panic("YAFTL: YAFTL_allocateNewBlock found an empty block"
							" (%d) with validPagesINo != 0\r\n", currBlk);
				}

				if (sInfo.blockArray[currBlk].validPagesDNo) {
					system_panic("YAFTL: YAFTL_allocateNewBlock found an empty block"
							" (%d) with validPagesDNo != 0\r\n", currBlk);
				}

				minEraseCnt = sInfo.blockArray[currBlk].eraseCount;
				bestBlk = currBlk;
			}
		}
	}

	if (bestBlk == 0xFFFFFFFF)
		system_panic("YAFTL: YAFTL_allocateNewBlock is out of blocks\r\n");

	if (isUserBlock) {
		deallocBTOC(sInfo.latestUserBlk.tocBuffer);
	} else {
		deallocBTOC(sInfo.latestIndexBlk.tocBuffer);
	}

	if (sInfo.blockArray[bestBlk].unkn5 == 1) {
		// Needs erasing.
		if (sInfo.field_78) {
			YAFTL_writeIndexTOC();
			sInfo.field_78 = FALSE;
		}

		if (FAILED(vfl_erase_single_block(vfl, bestBlk, TRUE)))
			return;

		++sInfo.blockArray[bestBlk].eraseCount;
		++sInfo.field_DC[3];
		sInfo.blockArray[bestBlk].unkn5 = 0;
	}

	// Store the data about the new chosen block.
	if (isUserBlock) {
		sInfo.blockArray[sInfo.latestUserBlk.blockNum].status =
			BLOCKSTATUS_ALLOCATED;

		sInfo.latestUserBlk.blockNum = bestBlk;
		sInfo.blockArray[bestBlk].status = BLOCKSTATUS_GC;
		sInfo.latestUserBlk.usedPages = 0;
		sInfo.latestUserBlk.tocBuffer = YAFTL_allocBTOC(bestBlk);
		memset(sInfo.latestUserBlk.tocBuffer, 0xFF, sInfo.tocPagesPerBlock *
				sGeometry.bytesPerPage);

		--sInfo.blockStats.numAvailable;
		++sInfo.blockStats.numAllocated;
	} else {
		sInfo.blockArray[sInfo.latestIndexBlk.blockNum].status =
			BLOCKSTATUS_I_ALLOCATED;

		sInfo.latestIndexBlk.blockNum = bestBlk;
		sInfo.blockArray[bestBlk].status = BLOCKSTATUS_I_GC;
		sInfo.latestIndexBlk.usedPages = 0;
		sInfo.latestIndexBlk.tocBuffer = YAFTL_allocBTOC(bestBlk);
		memset(sInfo.latestIndexBlk.tocBuffer, 0xFF, sInfo.tocPagesPerBlock *
				sGeometry.bytesPerPage);

		--sInfo.blockStats.numIAvailable;
		++sInfo.blockStats.numIAllocated;
	}

	--sInfo.blockStats.numFree;
	--sStats.freeBlocks;
}

error_t YAFTL_closeLatestBlock(uint8_t isUserBlock)
{
	SpareData* spare = sInfo.spareBuffer12;
	uint32_t usedPages;
	uint32_t blockNum;
	uint8_t* tocBuffer;
	uint8_t i;

	// FIXME: We must initialize sInfo.latest*Block.usn somewhere!
	if (isUserBlock) {
		spare->type = PAGETYPE_CLOSED;
		spare->usn = sInfo.latestUserBlk.usn;

		usedPages = sInfo.latestUserBlk.usedPages;
		blockNum = sInfo.latestUserBlk.blockNum;
		tocBuffer = (uint8_t*)sInfo.latestUserBlk.tocBuffer;
	} else {
		spare->type = PAGETYPE_CLOSED | PAGETYPE_INDEX;
		spare->usn = sInfo.latestIndexBlk.usn;

		usedPages = sInfo.latestIndexBlk.usedPages;
		blockNum = sInfo.latestIndexBlk.blockNum;
		tocBuffer = (uint8_t*)sInfo.latestIndexBlk.tocBuffer;
	}

	if (usedPages == sGeometry.pagesPerSublk - sInfo.tocPagesPerBlock) {
		// Block is indeed full; close it.
		uint32_t page = blockNum * sGeometry.pagesPerSublk + usedPages;

		for (i = 0; i < sInfo.tocPagesPerBlock; ++i) {
			error_t result = YAFTL_writePage(page,
					&tocBuffer[sGeometry.bytesPerPage * i], spare);

			if (FAILED(result))
				return EINVAL;
		}
	}

	return SUCCESS;
}

uint32_t* YAFTL_allocBTOC(uint32_t _block)
{
	int32_t minUsn = INT_MAX;
	int32_t found = -1;
	uint32_t i;

	sInfo.unkB0_1 = (sInfo.unkB0_1 + 1) % sInfo.unkAC_2;
	sInfo.unkB4_buffer[sInfo.unkB0_1] = _block;
	memset(sInfo.unkB8_buffer[sInfo.unkB0_1], 0xFF,
			sGeometry.pagesPerSublk * sizeof(uint32_t));

	sInfo.btocCurrUsn++;

	for (i = 0; i < sInfo.numBtocCaches; i++) {
		if ((sInfo.unk80_3 & (1<<i))
				&& (sInfo.btocCacheMissing & (1<<i))
				&& (sInfo.btocCacheUsn[i] < minUsn)) {
			found = i;
			minUsn = sInfo.btocCacheUsn[i];
		}
	}

	if (found == -1)
		system_panic("yaftl: couldn't allocate a BTOC\r\n");

	sInfo.btocCacheMissing &= ~(1<<found);
	sInfo.btocCacheBlocks[found] = _block;
	sInfo.btocCacheUsn[found] = sInfo.btocCurrUsn;

	return sInfo.btocCaches[found];
}

uint32_t* YAFTL_getBTOC(uint32_t _block)
{
	uint32_t* cache = NULL;
	int32_t maxUsn = -1;
	uint32_t i;

	for (i = 0; i < sInfo.numBtocCaches; ++i) {
		if (sInfo.btocCacheBlocks[i] == _block
				&& sInfo.btocCacheUsn[i] > maxUsn) {
			cache = sInfo.btocCaches[i];
			maxUsn = sInfo.btocCacheUsn[i];
		}
	}

	return cache;
}

error_t YAFTL_readBTOCPages(uint32_t offset, uint32_t *data, SpareData *spare, uint8_t disable_aes, uint8_t scrub, uint32_t max)
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

	return SUCCESS;
}

error_t YAFTL_readPage(uint32_t _page, uint8_t* _data_ptr, SpareData* _spare_ptr, uint32_t _disable_aes, uint32_t _empty_ok, uint32_t _scrub)
{
	error_t result;
	int refreshPage = 0;
	uint32_t block = _page / sGeometry.pagesPerSublk;

	uint8_t* meta_ptr = (uint8_t*)(_spare_ptr ? _spare_ptr : sInfo.spareBuffer18);

	result = vfl_read_single_page(vfl, _page, _data_ptr, meta_ptr, _empty_ok,
		&refreshPage, _disable_aes);

	if (FAILED(result)) {
		bufferPrintf("YAFTL_readPage: We got read failure: page %d, block %d, block status %d, scrub %d.\r\n",
			_page,
			block,
			sInfo.blockArray[block].status,
			_scrub);

		return ERROR_ARG;
	}

	sInfo.blockArray[block].readCount++;

	if (sInfo.blockArray[block].readCount > REFRESH_TRIGGER) {
		sInfo.refreshNeeded = TRUE;

		if (!refreshPage)
			return result;
	} else {
		if (!refreshPage)
			return result;

		sInfo.refreshNeeded = TRUE;
	}

	bufferPrintf("YAFTL: refresh triggered at page %d\r\n", _page);
	++sStats.refreshes;
	sInfo.blockArray[block].readCount = REFRESH_TRIGGER + 1;

	return result;
}

uint32_t YAFTL_writePage(uint32_t _page, uint8_t* _data_ptr, SpareData* _spare_ptr)
{
	error_t result;
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

	return 0;
}

error_t YAFTL_writeIndexTOC()
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

uint32_t YAFTL_findFreeTOCCache()
{
	uint32_t i;
	for (i = 0; i < sInfo.numCaches; i++) {
		if (sInfo.tocCaches[i].state == CACHESTATE_FREE)
			return i;
	}

	return 0xFFFF;
}

uint32_t YAFTL_clearEntryInCache(uint16_t _cacheIdx)
{
	typedef struct {
		uint16_t useCount;
		uint16_t idx;
	} bestcache_t;

	bestcache_t bestDirty, bestClean;
	uint32_t i;

	if (_cacheIdx == 0xFFFF) {
		// Any cache is okay.
		bestDirty.useCount = 0xFFFF;
		bestDirty.idx = 0xFFFF;
		bestClean.useCount = 0xFFFF;
		bestClean.idx = 0xFFFF;

		for (i = 0; i < sInfo.numCaches; ++i) {
			TOCCache* cache = &sInfo.tocCaches[i];

			if (cache->state == CACHESTATE_FREE) {
				return i;
			} else if (cache->state == CACHESTATE_DIRTY
					&& cache->useCount < bestDirty.useCount) {
				bestDirty.useCount = cache->useCount;
				bestDirty.idx = i;
			} else if (cache->state == CACHESTATE_CLEAN
					&& cache->useCount < bestClean.useCount) {
				bestClean.useCount = cache->useCount;
				bestClean.idx = i;
			}
		}
	} else {
		// TODO
		system_panic("YAFTL: clearEntryInCache by a specific cache isn't "
				"implemented yet!\r\n");
		return 0; // Avoid warnings
	}

	if (bestClean.idx != 0xFFFF) {
		// Clear this clean cache.
		sInfo.tocArray[sInfo.tocCaches[bestClean.idx].page].cacheNum = 0xFFFF;
		memset(sInfo.tocCaches[bestClean.idx].buffer, 0xFF,
				sGeometry.bytesPerPage);

		sInfo.tocCaches[bestClean.idx].state = CACHESTATE_FREE;
		sInfo.tocCaches[bestClean.idx].useCount = 0;
		++sInfo.numFreeCaches;
		return bestClean.idx;
	}

	if (bestDirty.idx != 0xFFFF) {
		// Oh well. Let's clear this dirty cache.
		SpareData* pSpare = sInfo.spareBuffer11;
		uint32_t oldPage = sInfo.tocCaches[bestDirty.idx].page;
		uint32_t oldIndexPage = sInfo.tocArray[oldPage].indexPage;

		sInfo.tocArray[oldPage].cacheNum = 0xFFFF;
		setupIndexSpare(pSpare, oldPage);

		if (oldIndexPage != 0xFFFFFFFF) {
			// Invalidate the index page.
			uint32_t oldIndexBlock = oldIndexPage / sGeometry.pagesPerSublk;

			if (sInfo.blockArray[oldIndexBlock].validPagesINo == 0) {
				system_panic("YAFTL: clearEntryInCache tried to invalidate an "
						"index page in block %d, but it has no index pages\r\n",
						oldIndexBlock);
			}

			--sInfo.blockArray[oldIndexBlock].validPagesINo;
			--sInfo.blockStats.numValidIPages;
			--sStats.indexPages;
		}

		// Write the dirty index page.
		error_t result = writeIndexPage(sInfo.tocCaches[bestDirty.idx].buffer,
				pSpare);

		if (result) {
			system_panic("YAFTL: clearEntryInCache failed to write dirty index"
					" page in %p\r\n", sInfo.tocCaches[bestDirty.idx].buffer);
		}

		sInfo.tocArray[oldPage].cacheNum = 0xFFFF;
		sInfo.tocCaches[bestDirty.idx].state = CACHESTATE_FREE;
		sInfo.tocCaches[bestDirty.idx].useCount = 0;
		memset(sInfo.tocCaches[bestDirty.idx].buffer, 0xFF,
				sGeometry.bytesPerPage);

		++sInfo.numFreeCaches;
		return bestDirty.idx;
	}

	return 0xFFFF;
}

void YAFTL_copyBTOC(uint32_t *dest, uint32_t *src, uint32_t maxVal)
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
