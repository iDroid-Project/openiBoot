#include "vfl/vfl.h"
#include "nand.h"
#include "util.h"
#include "commands.h"

typedef struct _vfl_vfl_context
{
	uint32_t usn_inc; // 0x000
	uint16_t control_block[3]; // 0x004
	uint8_t unk1[2]; // 0x00A
	uint32_t usn_dec; // 0x00C
	uint16_t active_context_block; // 0x010
	uint16_t next_context_page; // 0x012
	uint8_t unk2[2]; // 0x014
	uint16_t field_16; // 0x016
	uint16_t field_18; // 0x018
	uint16_t num_reserved_blocks; // 0x01A
	uint16_t reserved_block_pool_start; // 0x01C
	uint16_t total_reserved_blocks; // 0x01E
	uint16_t reserved_block_pool_map[820]; // 0x020
	uint8_t bad_block_table[282];			// 0x688
	uint16_t vfl_context_block[4];			// 0x7A2
	uint16_t remapping_schedule_start;		// 0x7AA
	uint8_t unk3[0x4C];				// 0x7AC
	uint32_t checksum1;				// 0x7F8
	uint32_t checksum2;				// 0x7FC
} vfl_vfl_context_t;

typedef struct _vfl_vfl_spare_data
{
	union
	{
		struct
		{
			uint32_t logicalPageNumber;
			uint32_t usn;
		} __attribute__ ((packed)) user;

		struct
		{
			uint32_t usnDec;
			uint16_t idx;
			uint8_t field_6;
			uint8_t field_7;
		} __attribute__ ((packed)) meta;
	};

	uint8_t type2;
	uint8_t type1;
	uint8_t eccMark;
	uint8_t field_B;
} __attribute__ ((packed)) vfl_vfl_spare_data_t;

static void virtual_page_number_to_virtual_address(vfl_vfl_device_t *_vfl, uint32_t _vpNum, uint16_t* _bank, uint16_t* _block, uint16_t* _page) {
	*_bank = _vpNum % _vfl->geometry.num_ce;
	*_block = (_vpNum / _vfl->geometry.num_ce) / _vfl->geometry.pages_per_block;
	*_page = (_vpNum / _vfl->geometry.num_ce) % _vfl->geometry.pages_per_block;
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
			if(pwDesPbn >= _vfl->geometry.blocks_per_ce)
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

static error_t vfl_vfl_read_single_page(vfl_device_t *_vfl, uint32_t dwVpn, uint8_t* buffer, uint8_t* spare, int empty_ok, int* refresh_page, uint32_t reserved)
{
	vfl_vfl_device_t *vfl = CONTAINER_OF(vfl_vfl_device_t, vfl, _vfl);

	if(refresh_page)
		*refresh_page = FALSE;

	//VFLData1.field_8++;
	//VFLData1.field_20++;

	dwVpn += vfl->geometry.pages_per_sublk * vfl->geometry.fs_start_block;
	if(dwVpn >= vfl->geometry.pages_total*vfl->geometry.pages_per_sublk)
	{
		bufferPrintf("ftl: No such virtual page %d.\r\n", dwVpn);
		return EINVAL;
	}

	uint16_t virtualBank;
	uint16_t virtualBlock;
	uint16_t virtualPage;
	uint16_t physicalBlock;

	virtual_page_number_to_virtual_address(vfl, dwVpn, &virtualBank, &virtualBlock, &virtualPage);
	physicalBlock = virtual_block_to_physical_block(vfl, virtualBank, virtualBlock);

	int ret = nand_device_read_single_page(vfl->device, virtualBank, physicalBlock + vfl->geometry.reserved_blocks, virtualPage, buffer, spare, 0);

	if(!empty_ok && ret == ENOENT)
		ret = EIO;

	if(ret == EINVAL || ret == EIO)
	{
		ret = nand_device_read_single_page(vfl->device, virtualBank, physicalBlock + vfl->geometry.reserved_blocks, virtualPage, buffer, spare, 0);
		if(!empty_ok && ret == ENOENT)
			return EIO;

		if(ret == EINVAL || ret == EIO)
			return ret;
	}

	if(ret == ENOENT && spare)
		memset(spare, 0xFF, vfl->geometry.bytes_per_spare);

	return ret;
}

static uint16_t* VFL_get_FTLCtrlBlock(vfl_vfl_device_t *_vfl)
{
	int bank = 0;
	int max = 0;
	uint16_t* FTLCtrlBlock = NULL;
	for(bank = 0; bank < _vfl->geometry.num_ce; bank++)
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

static inline error_t vfl_vfl_setup_geometry(vfl_vfl_device_t *_vfl)
{
#define nand_load(what, where) (nand_device_get_info(nand, (what), &_vfl->geometry.where, sizeof(_vfl->geometry.where)))

	nand_device_t *nand = _vfl->device;
	error_t ret = nand_load(diBlocksPerCE, blocks_per_ce);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("blocks per ce: 0x%08x\r\n", _vfl->geometry.blocks_per_ce);

	ret = nand_load(diBytesPerPage, bytes_per_page);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("bytes per page: 0x%08x\r\n", _vfl->geometry.bytes_per_page);

	ret = nand_load(diNumCE, num_ce);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("num ce: 0x%08x\r\n", _vfl->geometry.num_ce);

	ret = nand_load(diPagesPerBlock, pages_per_block);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("pages per block: 0x%08x\r\n", _vfl->geometry.pages_per_block);

	ret = nand_load(diECCBits, ecc_bits);
	if(FAILED(ret))
		return EINVAL;
	
	bufferPrintf("ecc bits: 0x%08x\r\n", _vfl->geometry.ecc_bits);

	uint16_t z = _vfl->geometry.blocks_per_ce;
	uint32_t mag = 1;
	while(z != 0 && mag < z) mag <<= 1;
	mag >>= 10;
	
	uint16_t a = (mag << 7) - (mag << 3) + mag;
	_vfl->geometry.some_page_mask = a;
	bufferPrintf("some_page_mask: 0x%08x\r\n", _vfl->geometry.some_page_mask);

	_vfl->geometry.pages_total = z * _vfl->geometry.pages_per_block * _vfl->geometry.num_ce;
	bufferPrintf("pages_total: 0x%08x\r\n", _vfl->geometry.pages_total);

	_vfl->geometry.pages_per_sublk = _vfl->geometry.pages_per_block * _vfl->geometry.num_ce;
	bufferPrintf("pages_per_sublk: 0x%08x\r\n", _vfl->geometry.pages_per_sublk);

	_vfl->geometry.some_sublk_mask = 
		_vfl->geometry.some_page_mask * _vfl->geometry.pages_per_sublk;
	bufferPrintf("some_sublk_mask: 0x%08x\r\n", _vfl->geometry.some_sublk_mask);
	
	ret = nand_load(diNumECCBytes, num_ecc_bytes);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("num_ecc_bytes: 0x%08x\r\n", _vfl->geometry.num_ecc_bytes);

	ret = nand_load(diMetaPerLogicalPage, bytes_per_spare);
	if(FAILED(ret))
		return EINVAL;
	
	bufferPrintf("bytes_per_spare: 0x%08x\r\n", _vfl->geometry.bytes_per_spare);

	ret = nand_load(diReturnOne, one);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("one: 0x%08x\r\n", _vfl->geometry.one);

	if(_vfl->geometry.num_ce != 1)
	{
		_vfl->geometry.some_crazy_val =	_vfl->geometry.blocks_per_ce
			- 27 - _vfl->geometry.reserved_blocks - _vfl->geometry.some_page_mask;
	}
	else
	{
		_vfl->geometry.some_crazy_val =	_vfl->geometry.blocks_per_ce - 27
			- _vfl->geometry.some_page_mask - (_vfl->geometry.reserved_blocks & 0xFFFF);
	}

	bufferPrintf("some_crazy_val: 0x%08x\r\n", _vfl->geometry.some_crazy_val);

	_vfl->geometry.vfl_blocks = _vfl->geometry.some_crazy_val + 4;
	bufferPrintf("vfl_blocks: 0x%08x\r\n", _vfl->geometry.vfl_blocks);

	_vfl->geometry.fs_start_block = _vfl->geometry.vfl_blocks + _vfl->geometry.reserved_blocks;
	bufferPrintf("fs_start_block: 0x%08x\r\n", _vfl->geometry.fs_start_block);

	uint32_t val = 1;
	nand_device_set_info(nand, diVendorType, &val, sizeof(val));

	val = 0x10001;
	nand_device_set_info(nand, diBanksPerCE_VFL, &val, sizeof(val));

#undef nand_load

	return SUCCESS;
}

static error_t vfl_vfl_open(vfl_device_t *_vfl, nand_device_t *_nand)
{
	vfl_vfl_device_t *vfl = CONTAINER_OF(vfl_vfl_device_t, vfl, _vfl);

	if(vfl->device || !_nand)
		return EINVAL;

	vfl->device = _nand;
	error_t ret = vfl_vfl_setup_geometry(vfl);
	if(FAILED(ret))
		return ret;

	bufferPrintf("vfl: Opening %p.\r\n", _nand);

	vfl->contexts = malloc(vfl->geometry.num_ce * sizeof(vfl_vfl_context_t));
	memset(vfl->contexts, 0, vfl->geometry.num_ce * sizeof(vfl_vfl_context_t));

	vfl->bbt = (uint8_t*) malloc(CEIL_DIVIDE(vfl->geometry.blocks_per_ce, 8));

	vfl->pageBuffer = (uint32_t*) malloc(vfl->geometry.pages_per_block * sizeof(uint32_t));
	vfl->chipBuffer = (uint16_t*) malloc(vfl->geometry.pages_per_block * sizeof(uint16_t));

	uint32_t bank = 0;
	for(bank = 0; bank < vfl->geometry.num_ce; bank++) {
		bufferPrintf("vfl: Checking bank %d.\r\n", bank);

		if(FAILED(nand_device_read_special_page(_nand, bank, "DEVICEINFOBBT\0\0\0",
						vfl->bbt, CEIL_DIVIDE(vfl->geometry.blocks_per_ce, 8))))
		{
			bufferPrintf("vfl: Failed to find DEVICEINFOBBT!\r\n");
			return EIO;
		}

		if(bank >= vfl->geometry.num_ce)
			return EIO;

		vfl_vfl_context_t *curVFLCxt = &vfl->contexts[bank];
		uint8_t* pageBuffer = malloc(vfl->geometry.bytes_per_page);
		uint8_t* spareBuffer = malloc(vfl->geometry.bytes_per_spare);
		if(pageBuffer == NULL || spareBuffer == NULL) {
			bufferPrintf("ftl: cannot allocate page and spare buffer\r\n");
			return -1;
		}

		// Any VFLCxt page will contain an up-to-date list of all blocks used to store VFLCxt pages. Find any such
		// page in the system area.

		int i;
		for(i = vfl->geometry.reserved_blocks; i < vfl->geometry.fs_start_block; i++) {
			// so pstBBTArea is a bit array of some sort
			if(!(vfl->bbt[i / 8] & (1 << (i  & 0x7))))
				continue;

			if(nand_device_read_single_page(vfl->device, bank, i, 0, pageBuffer, spareBuffer, 0) == 0)
			{
				memcpy(curVFLCxt->vfl_context_block, ((vfl_vfl_context_t*)pageBuffer)->vfl_context_block, sizeof(curVFLCxt->vfl_context_block));
				break;
			}
		}

		if(i == vfl->geometry.fs_start_block) {
			bufferPrintf("vfl: cannot find readable VFLCxtBlock\r\n");
			free(pageBuffer);
			free(spareBuffer);
			return EIO;
		}

		// Since VFLCxtBlock is a ringbuffer, if blockA.page0.spare.usnDec < blockB.page0.usnDec, then for any page a
	    // in blockA and any page b in blockB, a.spare.usNDec < b.spare.usnDec. Therefore, to begin finding the
		// page/VFLCxt with the lowest usnDec, we should just look at the first page of each block in the ring.
		int minUsn = 0xFFFFFFFF;
		int VFLCxtIdx = 4;
		for(i = 0; i < 4; i++) {
			uint16_t block = curVFLCxt->vfl_context_block[i];
			if(block == 0xFFFF)
				continue;

			if(nand_device_read_single_page(vfl->device, bank, block + vfl->geometry.reserved_blocks, 0, pageBuffer, spareBuffer, 0) != 0)
				continue;

			vfl_vfl_spare_data_t *spareData = (vfl_vfl_spare_data_t*)spareBuffer;
			if(spareData->meta.usnDec > 0 && spareData->meta.usnDec <= minUsn) {
				minUsn = spareData->meta.usnDec;
				VFLCxtIdx = i;
			}
		}

		if(VFLCxtIdx == 4) {
			bufferPrintf("vfl: cannot find readable VFLCxtBlock index in spares\r\n");
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
		for(page = 8; page < vfl->geometry.pages_per_block; page += 8) {
			if(nand_device_read_single_page(vfl->device, bank, curVFLCxt->vfl_context_block[VFLCxtIdx] + vfl->geometry.reserved_blocks,
						page, pageBuffer, spareBuffer, 0) != 0)
				break;
			
			last = page;
		}

		if(nand_device_read_single_page(vfl->device, bank, curVFLCxt->vfl_context_block[VFLCxtIdx] + vfl->geometry.reserved_blocks,
					last, pageBuffer, spareBuffer, 0) != 0)
		{
			bufferPrintf("vfl: cannot find readable VFLCxt\n");
			free(pageBuffer);
			free(spareBuffer);
			return -1;
		}

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
			bufferPrintf("vfl: VFLCxt has bad checksum.\r\n");
			return EIO;
		}
	} 

	// retrieve the FTL control blocks from the latest VFL across all banks.
	void* FTLCtrlBlock = VFL_get_FTLCtrlBlock(vfl);
	uint16_t buffer[3];

	// Need a buffer because eventually we'll copy over the source
	memcpy(buffer, FTLCtrlBlock, sizeof(buffer));

	// Then we update the VFLCxts on every bank with that information.
	for(bank = 0; bank < vfl->geometry.num_ce; bank++) {
		memcpy(vfl->contexts[bank].control_block, buffer, sizeof(buffer));
		vfl_gen_checksum(vfl, bank);
	}

	bufferPrintf("vfl: VFL successfully opened!\r\n");

	return SUCCESS;
}

static nand_device_t *vfl_vfl_get_device(vfl_device_t *_vfl)
{
	vfl_vfl_device_t *vfl = CONTAINER_OF(vfl_vfl_device_t, vfl, _vfl);
	return vfl->device;
}

error_t vfl_vfl_device_init(vfl_vfl_device_t *_vfl)
{
	memset(_vfl, 0, sizeof(*_vfl));

	//memset(&VFLData1, 0, sizeof(VFLData1));
	
	_vfl->current_version = 0;
	_vfl->vfl.open = vfl_vfl_open;

	_vfl->vfl.get_device = vfl_vfl_get_device;

	_vfl->vfl.read_single_page = vfl_vfl_read_single_page;

	memset(&_vfl->geometry, 0, sizeof(_vfl->geometry));

#ifdef CONFIG_A4
	_vfl->geometry.reserved_blocks = 16;
#else
	_vfl->geometry.reserved_blocks = 1;
#endif
	
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

vfl_vfl_device_t *vfl_vfl_device_allocate()
{
	vfl_vfl_device_t *ret = malloc(sizeof(*ret));
	vfl_vfl_device_init(ret);
	return ret;
}

