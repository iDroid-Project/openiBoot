#include "openiboot.h"
#include "util.h"
#include "ftl/yaftl.h"
#include "ftl/yaftl_gc.h"
#include "ftl/l2v.h"

/* Internals */

static uint32_t gcListPopFront(GCList* _list)
{
	uint32_t block;
	
	if (_list->head == _list->tail)
		system_panic("YAFTL: gcListPopFront was called but list is empty\r\n");

	block = _list->block[_list->head++];
	if (_list->head > GCLIST_MAX)
		_list->head = 0;

	return block;
}

static int gcHandleVpnMiss(GCData* _data, uint8_t _scrub)
{
	uint32_t btocEntry = _data->btoc[_data->btocIdx];
	uint32_t indexPageNo = btocEntry / sInfo.tocEntriesPerPage;
	uint32_t page;
	uint32_t cache;
	SpareData* pSpare = sInfo.gcSpareBuffer;

	if (indexPageNo >= sInfo.tocArrayLength) {
		system_panic("YAFTL: gcHandleVpnMiss got an out-of-range index page "
				"%d\r\n", indexPageNo);
	}

	page = sInfo.tocArray[indexPageNo].indexPage;
	cache = sInfo.tocArray[indexPageNo].cacheNum;

	if (cache == 0xFFFF) {
		if (page == 0xFFFFFFFF) {
			return -2;
		}

		// There is an index page, but it's not cached. Obtain it.
		cache = YAFTL_findFreeTOCCache();
		error_t status;

		if (cache == 0xFFFF) {
			cache = YAFTL_clearEntryInCache(0xFFFF);
			if (cache == 0xFFFF) {
				_data->state = 3;
				return -1;
			}
		}

		status = YAFTL_readPage(page, (uint8_t*)sInfo.tocCaches[cache].buffer,
				pSpare, FALSE, TRUE, _scrub);

		if (status != 0) {
			if (sInfo.field_78) {
				YAFTL_writeIndexTOC();
				sInfo.field_78 = TRUE;
			}

			system_panic("YAFTL: gcHandleVpnMiss Index UECC page 0x%08x status"
					" %08x\r\n", page, status);
		} else {
			if (!(pSpare->type & PAGETYPE_INDEX)) {
				if (sInfo.field_78) {
					YAFTL_writeIndexTOC();
					sInfo.field_78 = TRUE;
				}

				system_panic("YAFTL: gcHandleVpnMiss Invalid index metadata "
						"0x%02x\r\n", pSpare->type);
			} else {
				--sInfo.numFreeCaches;
				sInfo.tocCaches[cache].state = CACHESTATE_CLEAN;
				sInfo.tocCaches[cache].useCount = 1;
				sInfo.tocCaches[cache].page = indexPageNo;
				sInfo.tocArray[indexPageNo].cacheNum = cache;
			}
		}
	}

	return sInfo.tocCaches[cache].buffer[btocEntry % sInfo.tocEntriesPerPage];
}

static int gcFreeDataPages(int32_t _numPages, uint8_t _scrub)
{
	while (TRUE) {
		if (sInfo.gc.data.state == 0) {
			while (sInfo.blockStats.numIAvailable <= 1)
				gcFreeIndexPages(0xFFFFFFFF, _scrub);

			gcChooseBlock(&sInfo.gc.data, BLOCKSTATUS_ALLOCATED);
			if (sInfo.gc.data.totalValidPages != 0) {
				sInfo.gc.data.btocIdx = 0;
				sInfo.gc.data.numInvalidatedPages = 0;
				gcPopulateBTOC(&sInfo.gc.data, _scrub, sInfo.totalPages);
				sInfo.blockArray[sInfo.gc.data.chosenBlock].status =
					BLOCKSTATUS_CURRENT;

				sInfo.gc.data.uECC = 0;
				sInfo.gc.data.state = 1;
				return 0;
			} else {
				sInfo.gc.data.state = 2;
			}
		} else if (sInfo.gc.data.state == 1) {
			uint32_t gcPageNum;
			uint8_t continueAfterWhile = TRUE;

			if (sInfo.blockArray[sInfo.gc.data.chosenBlock].validPagesINo
					!= 0) {
				system_panic("YAFTL: gcFreeDataPages chose a block with no"
						" valid index pages\r\n");
			}

			if (sInfo.blockArray[sInfo.gc.data.chosenBlock].validPagesDNo
					== 0) {
				sInfo.gc.data.state = 2;
				continue;
			}

			// TODO: Understand this. Some sort of sorting score?
			gcPageNum = sInfo.gcPages;

			if (_numPages > 0) {
				uint32_t invalidPages = sGeometry.pagesPerSublk -
					sInfo.tocPagesPerBlock - sInfo.gc.data.totalValidPages;

				if (invalidPages == 0)
					invalidPages = 1;

				gcPageNum = sGeometry.pagesPerSublk * _numPages / invalidPages;
			}

			sInfo.gc.data.curZoneSize = 0;
			gcResetReadCache(&sInfo.gc.data.read_c);

			while (sInfo.gc.data.curZoneSize < gcPageNum &&
					sInfo.gc.data.btocIdx < sInfo.gc.data.dataPagesPerSublk &&
					sInfo.gc.data.numInvalidatedPages <
					sInfo.gc.data.totalValidPages) {
				uint32_t btocEntry =
					sInfo.gc.data.btoc[sInfo.gc.data.btocIdx];

				if (btocEntry != 0xFFFFFFFF) {
					if (sInfo.gc.data.read_c.span == 0 ||
							sInfo.gc.data.read_c.pageIndex != btocEntry) {
						uint32_t vpn = sInfo.gc.data.read_c.vpn;

						sInfo.gc.data.read_c.pageIndex = btocEntry;
						L2V_Search(&sInfo.gc.data.read_c);
						if (sInfo.gc.data.read_c.span == 0) {
							system_panic("YAFTL: gcFreeDataPages has called "
									"L2V_Search but span is still 0\r\n");
						}

						++sInfo.gc.data.read_c.pageIndex;
						--sInfo.gc.data.read_c.span;

						if (L2V_VPN_ISNORMAL(vpn)) {
							++sInfo.gc.data.read_c.vpn;
						} else if (vpn == L2V_VPN_MISS) {
							int ret = gcHandleVpnMiss(&sInfo.gc.data, _scrub);

							if (ret >= 0) {
								vpn = ret;
							} else {
								if (ret == -1)
									continueAfterWhile = FALSE;

								break;
							}
						} else if (vpn != L2V_VPN_SPECIAL) {
							system_panic("YAFTL: gcFreeDataPages doesn't know "
									"what vpn %X is\r\n", vpn);
						}

						if (vpn != L2V_VPN_SPECIAL) {
							uint32_t ourPage = sInfo.gc.data.chosenBlock
								* sGeometry.pagesPerSublk
								+ sInfo.gc.data.btocIdx;

							if (vpn == ourPage) {
								sInfo.gc.data.zone[
									sInfo.gc.data.curZoneSize++] = vpn;
								++sInfo.gc.data.numInvalidatedPages;
							}
						}
					}
				} // if (btocEntry != 0xFFFFFFFF)

				if (sInfo.gcPages - sInfo.latestUserBlk.usedPages %
						sGeometry.total_banks_ftl
						<= sInfo.gc.data.curZoneSize) {
					if (sInfo.blockArray[sInfo.gc.data.chosenBlock].
							validPagesDNo < sInfo.gc.data.curZoneSize) {
						system_panic("YAFTL: gcFreeDataPages tried to free "
								"more pages than it should\r\n");
					}

					if (gcReadZoneData(&sInfo.gc.data, 0, _scrub) == 0) {
						if (gcWriteZoneData(&sInfo.gc.data, _scrub) == 0) {
							sInfo.gc.data.curZoneSize = 0;
						} else {
							sInfo.gc.data.state = 4;
							continueAfterWhile = FALSE;
							break;
						}
					} else {
						if (sInfo.gc.data.state == 1)
							sInfo.gc.data.state = 0;

						continueAfterWhile = FALSE;
						break;
					}
				}

				++sInfo.gc.data.btocIdx;
			} // while

			if (!continueAfterWhile)
				break;

			// If there are still pages left in the zone, free them.
			if (sInfo.gc.data.curZoneSize > 0) {
				if (sInfo.blockArray[sInfo.gc.data.chosenBlock].validPagesDNo <
						sInfo.gc.data.curZoneSize) {
					system_panic("YAFTL: gcFreeDataPages had pages left in the"
							" zone, but there aren't valid pages at all\r\n");
				}

				if (gcReadZoneData(&sInfo.gc.data, 0, _scrub) == 0) {
					if (gcWriteZoneData(&sInfo.gc.data, _scrub) == 0) {
						if (sInfo.gc.data.dataPagesPerSublk <=
								sInfo.gc.data.btocIdx) {
							sanityCheckValid(&sInfo.gc.data);
						}

						return 0;
					} else {
						sInfo.gc.data.state = 4;
					}
				} else if (sInfo.gc.data.state == 1) {
					sInfo.gc.data.state = 0;
				}
			}
		} else if (sInfo.gc.data.state == 2) {
			// Claim this as a free block.
			uint32_t chosenBlock = sInfo.gc.data.chosenBlock;

			sanityCheckValid(&sInfo.gc.data);
			sInfo.blockArray[chosenBlock].unkn5 = 2;
			sInfo.blockArray[chosenBlock].readCount = 0;
			sInfo.blockArray[chosenBlock].validPagesINo = 0;
			sInfo.blockArray[chosenBlock].validPagesDNo = 0;
			sInfo.blockArray[chosenBlock].status = BLOCKSTATUS_FREE;
			++sInfo.blockStats.numFree;
			++sInfo.blockStats.numAvailable;
			--sInfo.blockStats.numAllocated;

			sInfo.gc.data.state = 0;
			if (sInfo.gc.data.list.head == sInfo.gc.data.list.tail)
				return 0;
		} else if (sInfo.gc.data.state == 3) {
			while (sInfo.gc.index.list.head != sInfo.gc.index.list.tail)
				gcFreeIndexPages(0xFFFFFFFF, _scrub);

			sInfo.gc.data.state = 0;
		} else if (sInfo.gc.data.state == 4) {
			sInfo.gc.data.state = 0;
		} else {
			return 0;
		}
	}

	return 0;
}

/* Externals */

error_t gcInit()
{
	uint32_t i;
	for (i = sGeometry.total_banks_ftl; i <= 0xF; i <<= 1);

	gcResetReadCache(&sInfo.gc.data.read_c);
	sInfo.gcPages = i;
	sInfo.gc.data.state = 0;
	sInfo.gc.data.victim = 0xFFFFFFFF;
	sInfo.gc.index.victim = 0xFFFFFFFF;

	// Allocate buffers
	bufzone_init(&sInfo.gcBufZone);

	sInfo.gc.data.btoc = (uint32_t*)bufzone_alloc(&sInfo.gcBufZone,
			sInfo.tocPagesPerBlock * sGeometry.bytesPerPage);
	sInfo.gc.data.pageBuffer1 = (uint8_t*)bufzone_alloc(&sInfo.gcBufZone,
			sGeometry.bytesPerPage);
	sInfo.gc.index.btoc = (uint32_t*)bufzone_alloc(&sInfo.gcBufZone,
			sInfo.tocPagesPerBlock * sGeometry.bytesPerPage);
	sInfo.gc.index.pageBuffer1 = (uint8_t*)bufzone_alloc(&sInfo.gcBufZone,
			sGeometry.bytesPerPage);
	// TODO: Verify that this is working. They use something else.
	sInfo.gcSpareBuffer = (SpareData*)bufzone_alloc(&sInfo.gcBufZone,
			sizeof(SpareData));
	sInfo.gc.data.pageBuffer2 = (uint8_t*)bufzone_alloc(&sInfo.gcBufZone,
			sInfo.gcPages * sGeometry.bytesPerPage);
	sInfo.gc.index.pageBuffer2 = (uint8_t*)bufzone_alloc(&sInfo.gcBufZone,
			sInfo.gcPages * sGeometry.bytesPerPage);
	sInfo.gc.data.spareArray = (SpareData*)bufzone_alloc(&sInfo.gcBufZone,
			sInfo.gcPages * sizeof(SpareData));
	sInfo.gc.index.spareArray = (SpareData*)bufzone_alloc(&sInfo.gcBufZone,
			sInfo.gcPages * sizeof(SpareData));

	bufzone_finished_allocs(&sInfo.gcBufZone);

	sInfo.gc.data.btoc = (uint32_t*)bufzone_rebase(&sInfo.gcBufZone,
			sInfo.gc.data.btoc);
	sInfo.gc.data.pageBuffer1 = (uint8_t*)bufzone_rebase(&sInfo.gcBufZone,
			sInfo.gc.data.pageBuffer1);
	sInfo.gc.index.btoc = (uint32_t*)bufzone_rebase(&sInfo.gcBufZone,
			sInfo.gc.index.btoc);
	sInfo.gc.index.pageBuffer1 = (uint8_t*)bufzone_rebase(&sInfo.gcBufZone,
			sInfo.gc.index.pageBuffer1);
	sInfo.gcSpareBuffer = (SpareData*)bufzone_rebase(&sInfo.gcBufZone,
			sInfo.gcSpareBuffer);
	sInfo.gc.data.pageBuffer2 = (uint8_t*)bufzone_rebase(&sInfo.gcBufZone,
			sInfo.gc.data.pageBuffer2);
	sInfo.gc.index.pageBuffer2 = (uint8_t*)bufzone_rebase(&sInfo.gcBufZone,
			sInfo.gc.index.pageBuffer2);
	sInfo.gc.data.spareArray = (SpareData*)bufzone_rebase(&sInfo.gcBufZone,
			sInfo.gc.data.spareArray);
	sInfo.gc.index.spareArray = (SpareData*)bufzone_rebase(&sInfo.gcBufZone,
			sInfo.gc.index.spareArray);

	bufzone_finished_rebases(&sInfo.gcBufZone);

	sInfo.gc.data.zone = (uint32_t*)yaftl_alloc(sInfo.gcPages
			* sizeof(uint32_t));
	sInfo.gc.index.zone = (uint32_t*)yaftl_alloc(sInfo.gcPages
			* sizeof(uint32_t));
	
	if (sInfo.gc.data.btoc && sInfo.gc.data.pageBuffer1 && sInfo.gc.index.btoc
			&& sInfo.gc.index.pageBuffer1 && sInfo.gcSpareBuffer
			&& sInfo.gc.data.pageBuffer2 && sInfo.gc.index.pageBuffer2
			&& sInfo.gc.data.spareArray && sInfo.gc.index.spareArray
			&& sInfo.gc.data.zone && sInfo.gc.index.zone) {
		return SUCCESS;
	} else {
		return ENOMEM;
	}
}

void gcResetReadCache(GCReadC* _readC)
{
	_readC->field_14 = 0xFFFFFFFF;
	_readC->node = NULL;
	_readC->span = 0;
}

void gcListPushBack(GCList* _list, uint32_t _block)
{
	uint32_t curr = _list->head;

	// Search if the block already exists.
	while (curr != _list->tail) {
		if (_list->block[curr] == _block)
			return;

		++curr;
		if (curr > GCLIST_MAX)
			curr = 0;
	}

	_list->block[_list->tail] = _block;
	++_list->tail;

	if (_list->tail > GCLIST_MAX)
		_list->tail = 0;

	if (_list->tail == _list->head)
		system_panic("YAFTL: gcListPushBack -- list is full\r\n");
}

void gcFreeBlock(uint32_t _block, uint8_t _scrub)
{
	uint32_t chosenBlock = sInfo.gc.data.chosenBlock;
	uint32_t currBlock = _block;

	if (sInfo.gc.data.state && chosenBlock != 0xFFFFFFFF &&
			sInfo.blockArray[chosenBlock].status == BLOCKSTATUS_CURRENT) {
		sInfo.blockArray[chosenBlock].status = BLOCKSTATUS_ALLOCATED;
	}

	sInfo.gc.data.state = FALSE;

	while (TRUE) {
		sInfo.gc.data.victim = currBlock;
		while (gcFreeDataPages(0, _scrub) == 0) {
			if (!sInfo.gc.data.state) {
				if (sInfo.gc.data.list.head == sInfo.gc.data.list.tail)
					return;

				currBlock = gcListPopFront(&sInfo.gc.data.list);
			}
		}
	}
}
