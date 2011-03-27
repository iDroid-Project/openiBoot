#include "vfl/vfl.h"
#include "nand.h"
#include "util.h"
#include "commands.h"

static void virtual_page_number_to_virtual_address(vfl_vfl_device_t *_vfl, uint32_t _vpNum, uint16_t* _bank, uint16_t* _block, uint16_t* _page) {
	*_bank = _vpNum % _vfl->geometry->num_ce;
	*_block = (_vpNum / _vfl->geometry->num_ce) / _vfl->geometry->pages_per_block;
	*_page = (_vpNum / _vfl->geometry->num_ce) % _vfl->geometry->pages_per_block;
}

static int vfl_is_good_block(uint8_t* badBlockTable, uint16_t virtualBlock) {
	int index = virtualBlock/8;
	return ((badBlockTable[index / 8] >> (7 - (index % 8))) & 0x1) == 0x1;
}

static uint16_t virtual_block_to_physical_block(vfl_vfl_device_t *_vfl, uint16_t virtualBank, uint16_t virtualBlock)
{
	if(vfl_is_good_block(_vfl->contexts[virtualBank].bad_block_table, virtualBlock))
		return virtualBlock;

	int pwDesPbn;
	for(pwDesPbn = 0; pwDesPbn < _vfl->contexts[virtualBank].num_reserved_blocks; pwDesPbn++)
	{
		if(_vfl->contexts[virtualBank].reserved_block_pool_map[pwDesPbn] == virtualBlock)
		{
			if(pwDesPbn >= _vfl->geometry->blocks_per_ce)
				bufferPrintf("ftl: Destination physical block for remapping is greater than number of blocks per bank!");

			return _vfl->contexts[virtualBank].reserved_block_pool_start + pwDesPbn;
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

static int vfl_gen_checksum(vfl_vfl_device_t *_vfl, int bank)
{
	vfl_checksum(&_vfl->contexts[bank], (uint32_t)&_vfl->contexts[bank].checksum1 - (uint32_t)&_vfl->contexts[bank],
		  	&_vfl->contexts[bank].checksum1, &_vfl->contexts[bank].checksum2);
	return TRUE;
}

static int vfl_check_checksum(vfl_vfl_device_t *_vfl, int bank)
{
	static int counter = 0;

	counter++;

	uint32_t checksum1;
	uint32_t checksum2;
	vfl_checksum(&_vfl->contexts[bank], (uint32_t)&_vfl->contexts[bank].checksum1 - (uint32_t)&_vfl->contexts[bank],
			&checksum1, &checksum2);

	// Yeah, this looks fail, but this is actually the logic they use
	if(checksum1 == _vfl->contexts[bank].checksum1)
		return TRUE;

	if(checksum2 != _vfl->contexts[bank].checksum2)
		return TRUE;

	return FALSE;
}

static int vfl_vfl_read_single_page(vfl_device_t *_vfl, uint32_t dwVpn, uint8_t* buffer, uint8_t* spare, int empty_ok, int* refresh_page)
{
	vfl_vfl_device_t *vfl = container_of(vfl_vfl_device_t, vfl, _vfl);

	if(refresh_page)
		*refresh_page = FALSE;

	//VFLData1.field_8++;
	//VFLData1.field_20++;
	
	uint32_t max_page = vfl->geometry->blocks_per_ce * vfl->geometry->pages_per_block * vfl->geometry->num_ce;
	if(dwVpn >= max_page)
	{
		bufferPrintf("ftl: No such virtual page %d (highest page %d).\r\n", dwVpn, max_page);
		return ERROR_ARG;
	}

	uint16_t virtualBank;
	uint16_t virtualBlock;
	uint16_t virtualPage;
	uint16_t physicalBlock;

	virtual_page_number_to_virtual_address(vfl, dwVpn, &virtualBank, &virtualBlock, &virtualPage);
	physicalBlock = virtual_block_to_physical_block(vfl, virtualBank, virtualBlock);

	int ret = nand_device_read_single_page(vfl->device, virtualBank, physicalBlock, virtualPage, buffer, spare);

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
		memset(spare, 0xFF, vfl->geometry->bytes_per_spare);

	return ret;
}

static int findDeviceInfoBBT(vfl_vfl_device_t *_vfl, int bank, void* deviceInfoBBT)
{
	uint8_t* buffer = malloc(_vfl->geometry->bytes_per_page);
	int lowestBlock = _vfl->geometry->blocks_per_ce - (_vfl->geometry->blocks_per_ce / 10);
	int block;
	for(block = _vfl->geometry->blocks_per_ce - 1; block >= lowestBlock; block--) {
		int page;
		int badBlockCount = 0;
		for(page = 0; page < _vfl->geometry->pages_per_block; page++) {
			if(badBlockCount > 2) {
				DebugPrintf("ftl: findDeviceInfoBBT - too many bad pages, skipping block %d\r\n", block);
				break;
			}

			int ret = nand_device_read_single_page(_vfl->device, bank, block, page, buffer, NULL);
			if(ret != 0) {
				if(ret == 1) {
					DebugPrintf("ftl: findDeviceInfoBBT - found 'badBlock' on bank %d, page %d\r\n", (block * _vfl->geometry->pages_per_block) + page);
					badBlockCount++;
				}

				DebugPrintf("ftl: findDeviceInfoBBT - skipping bank %d, page %d\r\n", (block * _vfl->geometry->pages_per_block) + page);
				continue;
			}

			if(memcmp(buffer, "DEVICEINFOBBT\0\0\0", 16) == 0) {
				if(deviceInfoBBT) {
					memcpy(deviceInfoBBT, buffer + 0x38, *((uint32_t*)(buffer + 0x34)));
				}

				free(buffer);
				return TRUE;
			} else {
				DebugPrintf("ftl: did not find signature on bank %d, page %d\r\n", (block * _vfl->geometry->pages_per_block) + page);
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
	for(bank = 0; bank < nand_geometry.blocks_per_ce; bank++)
	{
		good = findDeviceInfoBBT(bank, NULL);
		if(!good)
			return FALSE;
	}

	return good;
}*/

static uint16_t* VFL_get_FTLCtrlBlock(vfl_vfl_device_t *_vfl)
{
	int bank = 0;
	int max = 0;
	uint16_t* FTLCtrlBlock = NULL;
	for(bank = 0; bank < _vfl->geometry->num_ce; bank++)
	{
		int cur = _vfl->contexts[bank].usn_inc;
		if(max <= cur)
		{
			max = cur;
			FTLCtrlBlock = _vfl->contexts[bank].control_block;
		}
	}

	return FTLCtrlBlock;
}

static int vfl_vfl_open(vfl_device_t *_vfl, nand_device_t *_nand)
{
	vfl_vfl_device_t *vfl = container_of(vfl_vfl_device_t, vfl, _vfl);

	if(vfl->device)
		return -1;

	if(!_nand || !nand_device_get_geometry(_nand))
	{
		bufferPrintf("vfl: Cannot open empty NAND!\r\n.");
		return -1;
	}

	vfl->device = _nand;
	vfl->geometry = nand_device_get_geometry(_nand);

	bufferPrintf("vfl: Opening %p.\r\n", _nand);

	vfl->contexts = malloc(vfl->geometry->num_ce * sizeof(vfl_vfl_context_t));
	memset(vfl->contexts, 0, vfl->geometry->num_ce * sizeof(vfl_vfl_context_t));

	vfl->bbt = (uint8_t*) malloc(round_up(vfl->geometry->blocks_per_bank, 8));

	vfl->pageBuffer = (uint32_t*) malloc(vfl->geometry->pages_per_block * sizeof(uint32_t));
	vfl->chipBuffer = (uint16_t*) malloc(vfl->geometry->pages_per_block * sizeof(uint16_t));

	int bank = 0;
	for(bank = 0; bank < vfl->geometry->num_ce; bank++) {
		bufferPrintf("vfl: Checking bank %d.\r\n", bank);

		if(!findDeviceInfoBBT(vfl, bank, vfl->bbt))
		{
			bufferPrintf("ftl: findDeviceInfoBBT failed\r\n");
			return -1;
		}

		if(bank >= vfl->geometry->num_ce)
			return -1;

		vfl_vfl_context_t *curVFLCxt = &vfl->contexts[bank];
		uint8_t* pageBuffer = malloc(vfl->geometry->bytes_per_page);
		uint8_t* spareBuffer = malloc(vfl->geometry->bytes_per_spare);
		if(pageBuffer == NULL || spareBuffer == NULL) {
			bufferPrintf("ftl: cannot allocate page and spare buffer\r\n");
			return -1;
		}

		// Any VFLCxt page will contain an up-to-date list of all blocks used to store VFLCxt pages. Find any such
		// page in the system area.

		int vfl_start, vfl_size;
#ifdef CONFIG_A4
		vfl_start = 16;
		vfl_size = 1;
#else
		vfl_start = 1;
		vfl_size = 1;
#endif

		int i;
		for(i = 0; i < vfl_size /*FTLData->sysSuBlks*/; i++) {
			// so pstBBTArea is a bit array of some sort
			if(!(vfl->bbt[i / 8] & (1 << (i  & 0x7))))
				continue;

			if(nand_device_read_single_page(vfl->device, bank, i+vfl_start, 0, pageBuffer, spareBuffer) == 0)
			{
				memcpy(curVFLCxt->vfl_context_block, ((vfl_vfl_context_t*)pageBuffer)->vfl_context_block, sizeof(curVFLCxt->vfl_context_block));
				break;
			}
		}

		if(i == vfl_start+vfl_size) { //FTLData->sysSuBlks) {
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
		// read every 8th page, and nand_readvfl_cxt_page will try the 7 subsequent pages if the first was
		// no good. The last non-blank page will have the lowest spare.usnDec and highest usnInc for VFLCxt
		// in all the land (and is the newest).
		int page = 8;
		int last = 0;
		for(page = 8; page < vfl->geometry->pages_per_block; page += 8) {
			if(h2fmi_read_single_page(bank, (curVFLCxt->vfl_context_block[VFLCxtIdx] * vfl->geometry->pages_per_block) + page, pageBuffer, spareBuffer, NULL, NULL, 0) != 0) {
				break;
			}
			
			last = page;
		}

		if(h2fmi_read_single_page(bank, (curVFLCxt->vfl_context_block[VFLCxtIdx] * vfl->geometry->pages_per_block) + last, pageBuffer, spareBuffer, NULL, NULL, 0) != 0) {
			bufferPrintf("ftl: cannot find readable VFLCxt\n");
			free(pageBuffer);
			free(spareBuffer);
			return -1;
		}*/

		// Aha, so the upshot is that this finds the VFLCxt and copies it into vfl->contexts
		memcpy(&vfl->contexts[bank], pageBuffer, sizeof(vfl_vfl_context_t));

		// This is the newest VFLCxt across all banks
		if(curVFLCxt->usn_inc >= vfl->current_version) {
			vfl->current_version = curVFLCxt->usn_inc;
		}

		free(pageBuffer);
		free(spareBuffer);

		// Verify the checksum
		if(vfl_check_checksum(vfl, bank) == FALSE)
		{
			bufferPrintf("ftl: VFLCxt has bad checksum\n");
			return -1;
		}
	} 

	// retrieve the FTL control blocks from the latest VFL across all banks.
	void* FTLCtrlBlock = VFL_get_FTLCtrlBlock(vfl);
	uint16_t buffer[3];

	// Need a buffer because eventually we'll copy over the source
	memcpy(buffer, FTLCtrlBlock, sizeof(buffer));

	// Then we update the VFLCxts on every bank with that information.
	for(bank = 0; bank < vfl->geometry->num_ce; bank++) {
		memcpy(vfl->contexts[bank].control_block, buffer, sizeof(buffer));
		vfl_gen_checksum(vfl, bank);
	}

	return 0;
}

int vfl_vfl_device_init(vfl_vfl_device_t *_vfl)
{
	memset(_vfl, 0, sizeof(*_vfl));

	//memset(&VFLData1, 0, sizeof(VFLData1));
	
	_vfl->current_version = 0;
	_vfl->vfl.open = vfl_vfl_open;
	_vfl->vfl.read_single_page = vfl_vfl_read_single_page;
	return 0;
}

void vfl_vfl_device_cleanup(vfl_vfl_device_t *_vfl)
{
	if(_vfl->contexts)
		free(_vfl->contexts);

	if(_vfl->bbt)
		free(_vfl->bbt);

	if(_vfl->pageBuffer)
		free(_vfl->pageBuffer);

	if(_vfl->chipBuffer)
		free(_vfl->chipBuffer);

	memset(_vfl, 0, sizeof(*_vfl));
}

vfl_vfl_device_t *vfl_vfl_device_alllocate()
{
	vfl_vfl_device_t *ret = malloc(sizeof(*ret));
	vfl_vfl_device_init(ret);
	return ret;
}

