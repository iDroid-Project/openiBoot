#include "ftl/l2v.h"
#include "ftl/yaftl_mem.h"
#include "openiboot.h"
#include "util.h"

static void L2V_SetFirstNode(L2VNode* node);

static L2VDesc L2V;
//static L2VStruct L2VS;

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

L2VNode* L2V_EraseFirstNode()
{
	if (!L2V.firstNode)
		system_panic("L2V: currentNode not set\r\n");

	L2VNode* node = L2V.firstNode;
	L2V.firstNode = node->next;
	--L2V.nodeCount;
	return node;
}

void L2V_Search(GCReadC* _c)
{
	// for now
	_c->vpn = L2V_VPN_MISS;
	return;

	uint32_t nodeSize;
	uint32_t pageNum;
	uint32_t span, Span, vpn, Vpn;
	uint32_t bits, setting, Setting, value;

	uint32_t* node = (uint32_t*)((uint8_t*)&_c->node->next + _c->next_nOfs);

	if (L2V.previousNode != _c->field_24 || _c->pageIndex != _c->field_14)
		pageNum = _c->pageIndex;
	else if (_c->node->next == NULL || _c->next_nOfs > (_c->nodeSize - 4) ||
			*node == 0xFFFFFFFF)
		pageNum = _c->field_14;
	else
	{
		if (*node & 1)
		{
			bits = 14;
			span = GET_BITS(*node, 18, bits); //low bits for span
			vpn = GET_BITS(*node, 2, 16);
			setting = 1;
		}
		else
		{
			bits = 5;
			span = GET_BITS(*node, 27, bits);//low bits for span
			vpn = GET_BITS(*node, 2, 25);
			setting = 0;
		}

		if ((*node >> 1) & 1)
		{
			_c->nodeSize -= 2;
			span += *(uint16_t*)((uint8_t*)&_c->node->next + _c->nodeSize) << bits; // high bits for span

			if ((_c->next_nOfs) > (_c->nodeSize - 6)) //sizeof(lPtr_t) = 6
				system_panic("_c->next_nOfs) <= _c->nodeSize - sizeof(lPtr_t)");
		}

		if (span == 0)
			system_panic("L2V: span mustn't be zero!");

		if (!setting)
		{
			_c->vpn = vpn;
			_c->field_4 = 0;
			_c->span = span;
			_c->next_nOfs += 4;
			_c->field_14 = _c->pageIndex + _c->span;
			return;
		}

		pageNum = _c->pageIndex;
	}

	_c->field_14 = 0xFFFFFFFF;
	_c->node->next = NULL;
	_c->field_20 = 0;
	_c->vpn = L2V_VPN_MISS;

	uint32_t targTofs = pageNum % 0x8000;
	uint32_t targTree = pageNum / 0x8000; // root number
	if(L2V.numRoots <= targTree)
		system_panic("L2V: targTree has to be less than L2V.numRoots");

	if (L2V.Tree[targTree] & 1)
	{
		vpn = GET_BITS(L2V.Tree[targTree], 2, 16);
		value = 1;
	} else {
		vpn = GET_BITS(L2V.Tree[targTree], 2, 25);
		value = 0;
	}

	_c->next_nOfs = 0;

	L2VNode* pool = NULL;
	uint32_t offset = 0;
	uint32_t nextSpan = 0;

	while (1)
	{
		if (_c->field_20 > 31)
			system_panic("L2V_Search() fail!\r\n");

		if (value == 0)
		{
			_c->field_4 = targTofs - offset; // targTofs = pageNum % 0x8000

			_c->vpn = vpn;
			if (vpn <= 0x1FF0001)
				_c->vpn = vpn + targTofs - offset;

			_c->span = 0x8000 - _c->field_4;
			_c->node->next = pool;
			_c->field_14 = _c->pageIndex + 0x8000 - _c->field_4;
			_c->field_24 = L2V.previousNode;

			if (_c->field_20 > 25)
				L2V_Repack(targTree);

			return;
		} else {
			pool = &L2V.Pool[vpn];
			nodeSize = 0x40;
			uint32_t nOfs = 0;
			while (1)
			{
				node = (uint32_t*)((uint8_t*)&pool->next + nOfs);
				if (*node == 0xFFFFFFFF)
					system_panic("NAND index: bad tree\n");
				if (*node & 1)
				{
					bits = 14;
					Vpn = GET_BITS(*node, 2, bits);
					Span = GET_BITS(*node, 18, 14);
					Setting = 1;
				} else {
					bits = 5;
					Vpn = GET_BITS(*node, 27, bits);
					Span = GET_BITS(*node, 2, 25);
					Setting = 0;
				}

				if ((*node >> 1) & 1)
				{
					nodeSize -= 2; 
					Span += *(uint16_t*)((uint8_t*)&pool->next + nodeSize) << bits;
						if (nOfs > (nodeSize - 4)) //sizeof(lPtr_t) = 6
							system_panic("_c->next_nOfs) <= c->nodeSize - sizeof(lPtr_t)\n");
				}

				if (Span == 0);
					system_panic("L2V_Search fail!!\r\n");

				if (targTofs >= (Span + nextSpan))
				{
					nOfs += sizeof(uint16_t);
					nextSpan += Span;
					if(nOfs > nodeSize - 6)
						system_panic("NAND index: bad tree\n");
					continue;
				} else {
					_c->next_nOfs = nOfs + 4;
					_c->nodeSize = nodeSize;
					value = Setting;
					vpn = Vpn;
					break;
				}
			}
		}
	}
}

void L2V_Repack(uint32_t rootNum)
{
/*
	int nodeSize = 64;
	uint32_t vpn;

	if (!(L2V.Tree[rootNum] & 1))
		return;

	L2V.previousNode++;
	L2VS.bottomIdx = 0;
	L2VS.ovhSize = 0;
	L2VS.nodeSize = nodeSize;

	int i;
	for (i = 0; i < 3276; i++) {
		L2VS.Node[i] = NULL;
		L2VS.span[i] = 0; // 3276 to 6551
	}

	vpn = GET_BITS(L2V.Tree[rootNum], 2, 16);
	sub_80609390(L2VS,&L2V.Pool[vpn]);

	if (L2VS.ovhSize != 0)
	{
		void* memOffset = (void*)&L2VS.Node[L2VS.bottomIdx] + L2VS.ovhSize;
		memset(memOffset, 0xFF, L2VS.nodeSize - L2VS.ovhSize);
	}
	else
	{
		void* memOffset = &L2VS.Node[L2VS.bottomIdx];
		L2V_SetFirstNode(memOffset);
		L2VS.bottomIdx--;
	}

	uint32_t index = L2VS.bottomIdx + 1;
	while (L2VS.bottomIdx != 0)
	{
		L2VNode* node1 = L2V_EraseFirstNode();

		nOfs = 0;
		nodes = 0;
		spanCount = 0;
		index++;
		i = 0;

		do
		{
			if (nOfs + 6 > nodeSize)
			{
				memset((void*)node1 + nOfs, 0xFF, nodeSize - nOfs);
				&L2VS.Node[nodes] = node1;
				L2VS.span[nodes] = spanCount;
				node1 = sub_80608E5C();
				nodes++;
				nOfs = 0;
				spanCount = 0;
				index++;
			}

			if (L2VS.span[i] == 0)
				system_panic("((cu).span) != (0)\n");

			// [0:1]=config, [2:17] = vpn, [18:31] = span-lo
			*(uint32_t*)(&node1.Ofs + nOfs)= (L2VS.span[i] << 18) | (((&L2VS.Node[i] - &L2V.Pool[0]) / 0x40) & 0xFFFF) << 2 | 3;//repacking
			nodeSize -= 2;
			*(uint16_t*)(&node1.Ofs + nodeSize) = (uint16_t)L2VS.span[i] >> 14; //span-hi
			spanCount += L2VS.span[i];
			nOfs += 4;
			i++;
		} while (i < L2VS.bottomIdx);

		if (nOfs == 0)
		{
			yaFTL_L2V_SetNode(node1);
			nodes--;
			index--;
		}
		else
		{
			L2VS.Node[nodes] = node1;
			L2VS.span[i] = spanCount;
			memset(node2 + nOfs, 0xFF; nodeSize - nOfs);
		}

		if (nodes == 0 && spanCount != 0x8000)
			system_panic("(thisSpan) == ((1 << 15)) \n");

		L2VS.bottomIdx = nodes;
	}

	L2V.TreeNodes[rootNum] = index;

	if (L2VS.bottomIdx != 0)
		system_panic("(r->bottomIdx) == (0)\n");

	L2V.Tree[rootNum] = (((&L2VS.Node[0] - &L2V.Pool[0]) / 0x40) & 0xFFFF) << 2 | 1; // span = 0
	L2V.UpdatesSinceRepack[rootNum] = 0;
	
	if (L2VS.field_66EC > 0x1F)
		L2VS.field_66EC = 0;

	L2VS.field_666C[L2VS.field_66EC] = rootNum;
	L2VS.field_66EC++;

	return;
*/
}

void L2V_Update(uint32_t _start, uint32_t _count, uint32_t _vpn) {};
