#include "vfl/vfl.h"
#include "nand.h"
#include "util.h"
#include "commands.h"

// TODO: Fix this. -- Ricky26

#if 0
static int VFL_Init_Done = 0;
//static VFLData1Type VFLData1;
static vfl_context_t* pstVFLCxt = NULL;
static uint8_t* pstBBTArea = NULL;
static uint32_t* ScatteredPageNumberBuffer = NULL;
static uint16_t* ScatteredBankNumberBuffer = NULL;
static int curVFLusnInc = 0;

static void virtual_page_number_to_virtual_address(uint32_t _vpNum, uint16_t* _bank, uint16_t* _block, uint16_t* _page) {
	*_bank = _vpNum % nand_geometry.num_ce;
	*_block = (_vpNum / nand_geometry.num_ce) / nand_geometry.pages_per_block;
	*_page = (_vpNum / nand_geometry.num_ce) % nand_geometry.pages_per_block;
}

static int vfl_is_good_block(uint8_t* badBlockTable, uint16_t virtualBlock) {
	int index = virtualBlock/8;
	return ((badBlockTable[index / 8] >> (7 - (index % 8))) & 0x1) == 0x1;
}

static uint16_t virtual_block_to_physical_block(uint16_t virtualBank, uint16_t virtualBlock) {
	if(vfl_is_good_block(pstVFLCxt[virtualBank].bad_block_table, virtualBlock))
		return virtualBlock;

	int pwDesPbn;
	for(pwDesPbn = 0; pwDesPbn < pstVFLCxt[virtualBank].num_reserved_blocks; pwDesPbn++) {
		if(pstVFLCxt[virtualBank].reserved_block_pool_map[pwDesPbn] == virtualBlock) {
			if(pwDesPbn >= nand_geometry.blocks_per_ce)
				bufferPrintf("ftl: Destination physical block for remapping is greater than number of blocks per bank!");

			return pstVFLCxt[virtualBank].reserved_block_pool_start + pwDesPbn;
		}
	}

	return virtualBlock;
}

static void vfl_checksum(void* data, int size, uint32_t* a, uint32_t* b)
{
	int i;
	uint32_t* buffer = (uint32_t*) data;
	uint32_t x = 0;
	uint32_t y = 0;
	for(i = 0; i < (size / 4); i++) {
		x += buffer[i];
		y ^= buffer[i];
	}

	*a = x + 0xAABBCCDD;
	*b = y ^ 0xAABBCCDD;
}

static int vfl_gen_checksum(int bank)
{
	vfl_checksum(&pstVFLCxt[bank], (uint32_t)&pstVFLCxt[bank].checksum1 - (uint32_t)&pstVFLCxt[bank], &pstVFLCxt[bank].checksum1, &pstVFLCxt[bank].checksum2);
	return TRUE;
}

static int vfl_check_checksum(int bank)
{
	static int counter = 0;

	counter++;

	uint32_t checksum1;
	uint32_t checksum2;
	vfl_checksum(&pstVFLCxt[bank], (uint32_t)&pstVFLCxt[bank].checksum1 - (uint32_t)&pstVFLCxt[bank], &checksum1, &checksum2);

	// Yeah, this looks fail, but this is actually the logic they use
	if(checksum1 == pstVFLCxt[bank].checksum1)
		return TRUE;

	if(checksum2 != pstVFLCxt[bank].checksum2)
		return TRUE;

	return FALSE;
}

int vfl_read(uint32_t dwVpn, uint8_t* buffer, uint8_t* spare, int empty_ok, int* refresh_page, int aes)
{
	if(refresh_page)
		*refresh_page = FALSE;

	//VFLData1.field_8++;
	//VFLData1.field_20++;

	if(dwVpn >= nand_geometry.pages_per_ce * nand_geometry.num_ce)
	{
		bufferPrintf("ftl: No such virtual page %d.\r\n", dwVpn);
		return ERROR_ARG;
	}

	uint16_t virtualBank;
	uint16_t virtualBlock;
	uint16_t virtualPage;
	uint16_t physicalBlock;

	virtual_page_number_to_virtual_address(dwVpn, &virtualBank, &virtualBlock, &virtualPage);
	physicalBlock = virtual_block_to_physical_block(virtualBank, virtualBlock);

	int page = physicalBlock * nand_geometry.pages_per_block + virtualPage;

	int ret = h2fmi_read_single_page(virtualBank, page, buffer, spare, NULL, NULL, !aes);

	if(!empty_ok && ret == ERROR_EMPTY)
		ret = ERROR_NAND;

	/*if(refresh_page) {
		if((Geometry->field_2F <= 0 && ret == 0) || ret == ERROR_NAND) {
			bufferPrintf("ftl: setting refresh_page to TRUE due to the following factors: Geometry->field_2F = %x, ret = %d\r\n", Geometry->field_2F, ret);
			*refresh_page = TRUE;
		}
	}*/

	/*if(ret == ERROR_ARG || ret == ERROR_NAND) {
		nand_bank_reset(virtualBank, 100);
		ret = nand_read(virtualBank, page, buffer, spare, TRUE, TRUE);
		if(!empty_ok && ret == ERROR_EMPTY) {
			return ERROR_NAND;
		}

		if(ret == ERROR_ARG || ret == ERROR_NAND)
			return ret;
	}*/

	if(ret == ERROR_EMPTY && spare)
		memset(spare, 0xFF, nand_geometry.bytes_per_spare);

	return ret;
}

static int findDeviceInfoBBT(int bank, void* deviceInfoBBT)
{
	uint8_t* buffer = malloc(nand_geometry.bytes_per_page);
	int lowestBlock = nand_geometry.blocks_per_ce - (nand_geometry.blocks_per_ce / 10);
	int block;
	for(block = nand_geometry.blocks_per_ce - 1; block >= lowestBlock; block--) {
		int page;
		int badBlockCount = 0;
		for(page = 0; page < nand_geometry.pages_per_block; page++) {
			if(badBlockCount > 2) {
				DebugPrintf("ftl: findDeviceInfoBBT - too many bad pages, skipping block %d\r\n", block);
				break;
			}

			int ret = h2fmi_read_single_page(bank, (block * nand_geometry.pages_per_block) + page, buffer, NULL, NULL, NULL, 0);
			if(ret != 0) {
				if(ret == 1) {
					DebugPrintf("ftl: findDeviceInfoBBT - found 'badBlock' on bank %d, page %d\r\n", (block * nand_geometry.pages_per_block) + page);
					badBlockCount++;
				}

				DebugPrintf("ftl: findDeviceInfoBBT - skipping bank %d, page %d\r\n", (block * nand_geometry.pages_per_block) + page);
				continue;
			}

			if(memcmp(buffer, "DEVICEINFOBBT\0\0\0", 16) == 0) {
				if(deviceInfoBBT) {
					memcpy(deviceInfoBBT, buffer + 0x38, *((uint32_t*)(buffer + 0x34)));
				}

				free(buffer);
				return TRUE;
			} else {
				DebugPrintf("ftl: did not find signature on bank %d, page %d\r\n", (block * nand_geometry.pages_per_block) + page);
			}
		}
	}

	free(buffer);
	return FALSE;
}

/*static int hasDeviceInfoBBT()
{
	int bank;
	int good = TRUE;
	for(bank = 0; bank < nand_geometry.banks_per_ce; bank++)
	{
		good = findDeviceInfoBBT(bank, NULL);
		if(!good)
			return FALSE;
	}

	return good;
}*/

static uint16_t* VFL_get_FTLCtrlBlock()
{
	int bank = 0;
	int max = 0;
	uint16_t* FTLCtrlBlock = NULL;
	for(bank = 0; bank < nand_geometry.banks_per_ce; bank++)
	{
		int cur = pstVFLCxt[bank].usn_inc;
		if(max <= cur)
		{
			max = cur;
			FTLCtrlBlock = pstVFLCxt[bank].control_block;
		}
	}

	return FTLCtrlBlock;
}

static int VFL_Open() {
	int bank = 0;
	for(bank = 0; bank < nand_geometry.num_ce; bank++) {
		if(!findDeviceInfoBBT(bank, pstBBTArea)) {
			bufferPrintf("ftl: findDeviceInfoBBT failed\r\n");
			return -1;
		}

		if(bank >= nand_geometry.num_ce) {
			return -1;
		}


		vfl_context_t *curVFLCxt = &pstVFLCxt[bank];
		uint8_t* pageBuffer = malloc(nand_geometry.bytes_per_page);
		uint8_t* spareBuffer = malloc(nand_geometry.bytes_per_spare);
		if(pageBuffer == NULL || spareBuffer == NULL) {
			bufferPrintf("ftl: cannot allocate page and spare buffer\r\n");
			return -1;
		}

		// Any VFLCxt page will contain an up-to-date list of all blocks used to store VFLCxt pages. Find any such
		// page in the system area.

		int i;
		for(i = 128; i < 256 /*FTLData->sysSuBlks*/; i++) {
			// so pstBBTArea is a bit array of some sort
			if(!(pstBBTArea[i / 8] & (1 << (i  & 0x7))))
				continue;

			if(h2fmi_read_single_page(bank, i, pageBuffer, spareBuffer, NULL, NULL, 0) == 0) {
				memcpy(curVFLCxt->vfl_context_block, ((vfl_context_t*)pageBuffer)->vfl_context_block, sizeof(curVFLCxt->vfl_context_block));
				break;
			}
		}

		if(i == 256) { //FTLData->sysSuBlks) {
			bufferPrintf("ftl: cannot find readable VFLCxtBlock\r\n");
			free(pageBuffer);
			free(spareBuffer);
			return -1;
		}

		// Since VFLCxtBlock is a ringbuffer, if blockA.page0.spare.usnDec < blockB.page0.usnDec, then for any page a
	        // in blockA and any page b in blockB, a.spare.usNDec < b.spare.usnDec. Therefore, to begin finding the
		// page/VFLCxt with the lowest usnDec, we should just look at the first page of each block in the ring.
		/*int minUsn = 0xFFFFFFFF;
		int VFLCxtIdx = 4;
		for(i = 0; i < 4; i++) {
			uint16_t block = curVFLCxt->vfl_context_block[i];
			if(block == 0xFFFF)
				continue;

			if(h2fmi_read_single_page(bank, block, pageBuffer, spareBuffer, NULL, NULL, 0) != 0)
				continue;

			vfl_spare_data_t *spareData = (vfl_spare_data_t*)spareBuffer;
			if(spareData->meta.usnDec > 0 && spareData->meta.usnDec <= minUsn) {
				minUsn = spareData->meta.usnDec;
				VFLCxtIdx = i;
			}
		}

		if(VFLCxtIdx == 4) {
			bufferPrintf("ftl: cannot find readable VFLCxtBlock index in spares\r\n");
			free(pageBuffer);
			free(spareBuffer);
			return -1;
		}

		// VFLCxts are stored in the block such that they are duplicated 8 times. Therefore, we only need to
		// read every 8th page, and nand_read_vfl_cxt_page will try the 7 subsequent pages if the first was
		// no good. The last non-blank page will have the lowest spare.usnDec and highest usnInc for VFLCxt
		// in all the land (and is the newest).
		int page = 8;
		int last = 0;
		for(page = 8; page < nand_geometry.pages_per_block; page += 8) {
			if(h2fmi_read_single_page(bank, (curVFLCxt->vfl_context_block[VFLCxtIdx] * nand_geometry.pages_per_block) + page, pageBuffer, spareBuffer, NULL, NULL, 0) != 0) {
				break;
			}
			
			last = page;
		}

		if(h2fmi_read_single_page(bank, (curVFLCxt->vfl_context_block[VFLCxtIdx] * nand_geometry.pages_per_block) + last, pageBuffer, spareBuffer, NULL, NULL, 0) != 0) {
			bufferPrintf("ftl: cannot find readable VFLCxt\n");
			free(pageBuffer);
			free(spareBuffer);
			return -1;
		}*/

		// Aha, so the upshot is that this finds the VFLCxt and copies it into pstVFLCxt
		memcpy(&pstVFLCxt[bank], pageBuffer, sizeof(vfl_context_t));

		// This is the newest VFLCxt across all banks
		if(curVFLCxt->usn_inc >= curVFLusnInc) {
			curVFLusnInc = curVFLCxt->usn_inc;
		}

		free(pageBuffer);
		free(spareBuffer);

		// Verify the checksum
		if(vfl_check_checksum(bank) == FALSE) {
			bufferPrintf("ftl: VFLCxt has bad checksum\n");
			return -1;
		}
	} 

	// retrieve the FTL control blocks from the latest VFL across all banks.
	void* FTLCtrlBlock = VFL_get_FTLCtrlBlock();
	uint16_t buffer[3];

	// Need a buffer because eventually we'll copy over the source
	memcpy(buffer, FTLCtrlBlock, sizeof(buffer));

	// Then we update the VFLCxts on every bank with that information.
	for(bank = 0; bank < nand_geometry.banks_per_ce; bank++) {
		memcpy(pstVFLCxt[bank].control_block, buffer, sizeof(buffer));
		vfl_gen_checksum(bank);
	}

	return 0;
}

int VFL_Init()
{
	if(VFL_Init_Done)
		return 0;

	//memset(&VFLData1, 0, sizeof(VFLData1));
	if(pstVFLCxt == NULL) {
		pstVFLCxt = malloc(nand_geometry.num_ce * sizeof(vfl_context_t));
		memset(pstVFLCxt, 0, nand_geometry.num_ce * sizeof(vfl_context_t));
		if(pstVFLCxt == NULL)
			return -1;
	}

	if(pstBBTArea == NULL) {
		pstBBTArea = (uint8_t*) malloc(round_up(nand_geometry.blocks_per_bank, 8));
		if(pstBBTArea == NULL)
			return -1;
	}

	if(ScatteredPageNumberBuffer == NULL && ScatteredBankNumberBuffer == NULL) {
		ScatteredPageNumberBuffer = (uint32_t*) malloc(nand_geometry.pages_per_block * sizeof(uint32_t));
		ScatteredBankNumberBuffer = (uint16_t*) malloc(nand_geometry.pages_per_block * sizeof(uint16_t));
		if(ScatteredPageNumberBuffer == NULL || ScatteredBankNumberBuffer == NULL)
			return -1;
	}

	curVFLusnInc = 0;
	VFL_Init_Done = 1;
	return 0;
}

static void vfl_bootstrap()
{
	if(VFL_Init() != 0)
	{
		bufferPrintf("vfl: Failed to initialize VFL!\r\n");
		return;
	}

	/*if(VFL_Open() != 0)
	{
		bufferPrintf("vfl: Failed to open VFL!\r\n");
		return;
	}*/
}
MODULE_INIT(vfl_bootstrap);

static void cmd_vfl_open(int argc, char** argv)
{
	VFL_Open();
}
COMMAND("vfl_open", "H2FMI NAND test", cmd_vfl_open);

static void cmd_vfl_read(int argc, char** argv)
{
	if(argc < 7)
	{
		bufferPrintf("Usage: %s [page] [data] [metadata] [buf1] [buf2] [flag]\r\n", argv[0]);
		return;
	}
	
	uint32_t page = parseNumber(argv[1]);
	uint32_t data = parseNumber(argv[2]);
	uint32_t meta = parseNumber(argv[3]);
	uint32_t empty_ok = parseNumber(argv[4]);
	uint32_t refresh = parseNumber(argv[5]);
	uint32_t flag = parseNumber(argv[6]);

	uint32_t ret = vfl_read(page,
			(uint8_t*)data, (uint8_t*)meta, empty_ok, (int32_t*)refresh,
			flag);

	bufferPrintf("vfl: Command completed with result 0x%08x.\r\n", ret);
}
COMMAND("vfl_read", "H2FMI NAND test", cmd_vfl_read);
#endif
