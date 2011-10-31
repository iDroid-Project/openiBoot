#include "ftl/l2v.h"
#include "ftl/yaftl_mem.h"
#include "openiboot.h"
#include "util.h"

static void L2V_SetFirstNode(L2VNode* node);

static L2VDesc L2V;

error_t L2V_Init(uint32_t totalPages, uint32_t numBlocks, uint32_t pagesPerSublk)
{
	int32_t i;

	if (numBlocks * pagesPerSublk > 0x1FF0001)
		system_panic("L2V: Tree bitspace not sufficient for geometry %dx%d\r\n", numBlocks, pagesPerSublk);

	L2V.numBlocks = numBlocks;
	L2V.pagesPerSublk = pagesPerSublk;
	L2V.numRoots = (totalPages >> 15) + 1;
	L2V.nextNode = NULL;

	L2V.Tree = (uint32_t*)yaftl_alloc(L2V.numRoots * sizeof(uint32_t));
	if (!L2V.Tree)
		system_panic("L2V: tree allocation has failed\r\n");

	L2V.TreeNodes = (uint32_t*)yaftl_alloc(L2V.numRoots * sizeof(uint32_t));
	if (!L2V.TreeNodes)
		system_panic("L2V: tree nodes allocation has failed\r\n");

	L2V.UpdatesSinceRepack = (uint32_t*)yaftl_alloc(L2V.numRoots * sizeof(uint32_t));
	if (!L2V.UpdatesSinceRepack)
		system_panic("L2V: updates since repack allocation has failed\r\n");

	for (i = 0; i < L2V.numRoots; ++i)
		L2V.Tree[i] = 0x7FC000Cu;

	L2V.previousNode = NULL;
	L2V.Pool = (L2VNode*)yaftl_alloc(0x10000 * sizeof(L2VNode));
	if (!L2V.Pool)
		return ENOMEM;

	L2V.firstNode = NULL;
	L2V.nodeCount = 0;
	for (i = 0xFFF0; i >= 0; i -= 0x10)
		L2V_SetFirstNode(&L2V.Pool[i]);

	return SUCCESS;
}

void L2V_Open()
{
	uint32_t tocIndex = 0;
	uint32_t gcIndex = 0;
	uint32_t i;
	uint8_t* pageBuffer = sInfo.gc.index.pageBuffer2;
	SpareData* spareArray = sInfo.gc.index.spareArray;

	while (tocIndex < sInfo.tocArrayLength && L2V.nodeCount > 128) {
		// Update the GC zone.
		gcIndex = 0;
		while (gcIndex < sInfo.gcPages && tocIndex < sInfo.tocArrayLength) {
			if (sInfo.tocArray[tocIndex].indexPage != 0xFFFFFFFF)
				sInfo.gc.index.zone[gcIndex++] = sInfo.tocArray[tocIndex].indexPage;

			++tocIndex;
		}

		if (YAFTL_readMultiPages(sInfo.gc.index.zone, gcIndex, pageBuffer,
				spareArray, FALSE, FALSE) != 1)
		{
			// Manually read each page.
			for (i = 0; i < gcIndex; ++i) {
				if (YAFTL_readPage(sInfo.gc.index.zone[i],
						&pageBuffer[i * sGeometry.bytesPerPage],
						&spareArray[i], FALSE, TRUE, FALSE) != 0)
				{
					spareArray[i].lpn = 0xFFFFFFFF;
				}
			}
		}

		// Update L2V with the read data.
		for (i = 0; i < gcIndex && L2V.nodeCount > 128; i++) {
			if (spareArray[i].type & PAGETYPE_INDEX
				&& spareArray[i].lpn < sInfo.tocArrayLength)
			{
				L2V_UpdateFromTOC(spareArray[i].lpn,
					(uint32_t*)&pageBuffer[i * sGeometry.bytesPerPage]);
			}
		}
	}
}

void L2V_UpdateFromTOC(uint32_t _tocIdx, uint32_t* _tocBuffer)
{
	uint32_t i;
	uint32_t entry = 0xFFFFFFFF;
	uint32_t count = 0;
	uint32_t lpn = _tocIdx * sInfo.tocEntriesPerPage;
	uint32_t firstLpn = 0xFFFFFFFF;

	for (i = 0; i < sInfo.tocEntriesPerPage && L2V.nodeCount > 128; ++i, ++lpn)
	{
		if (count == 0) {
			// This should be the first LPN.
			entry = _tocBuffer[i];
			firstLpn = lpn;
			++count;
			continue;
		}

		if (entry == 0xFFFFFFFF) {
			if (_tocBuffer[i] == 0xFFFFFFFF) {
				// We're still in a row, because both indices are invalid.
				++count;
				continue;
			}

			// Bummer, we lost our row.
			entry = L2V_VPN_SPECIAL;
		} else if (entry + count == _tocBuffer[i]) {
			// Yay! still a row.
			++count;
			continue;
		}

		// Row is done: update L2V, and start a new one.
		L2V_Update(firstLpn, count, entry);
		entry = _tocBuffer[i];
		firstLpn = lpn;
		count = 1;
	}
}

static void L2V_SetFirstNode(L2VNode* node)
{
	if (node < &L2V.Pool[0] || node > &L2V.Pool[0x10000])
		system_panic("L2V: new first node must be in pool range\r\n");

	node->next = L2V.firstNode;
	L2V.firstNode = node;
	++L2V.nodeCount;
}
