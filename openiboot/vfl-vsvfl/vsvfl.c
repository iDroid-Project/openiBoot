#include "vfl/vsvfl.h"
#include "util.h"
#include "commands.h"

typedef struct _vfl_vsvfl_context
{
	uint32_t usn_inc; // 0x000
	uint32_t field_4; // 0x004
	uint32_t ftl_type; // 0x008
	uint32_t usn_dec; // 0x00C
	uint16_t active_context_block; // 0x010
	uint16_t next_context_page; // 0x012
	uint8_t unk2[2]; // 0x014
	uint16_t field_16; // 0x016
	uint16_t field_18; // 0x018
	uint16_t num_reserved_blocks; // 0x01A
	uint16_t field_1C; // 0x01C
	uint16_t total_reserved_blocks; // 0x01E
	uint8_t field_20[6]; // 0x020
	uint16_t reserved_block_pool_map[820]; // 0x026
	uint16_t vfl_context_block[4]; // 0x68E
	uint16_t usable_blocks_per_bank; // 0x696
	uint16_t reserved_block_pool_start; // 0x698
	uint16_t control_block[3]; // 0x69A
	uint8_t field_6A0[42]; // 0x6A0
	uint32_t field_6CA[4]; // 0x6CA
	uint32_t vendor_type; // 0x6DA
	uint8_t field_6DE[204]; // 0x6DE
	uint16_t remapping_schedule_start; // 0x7AA
	uint8_t unk3[0x48];				// 0x7AC
	uint32_t version;				// 0x7F4
	uint32_t checksum1;				// 0x7F8
	uint32_t checksum2;				// 0x7FC
} __attribute__((packed)) vfl_vsvfl_context_t;

typedef struct _vfl_vsvfl_spare_data
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
} __attribute__ ((packed)) vfl_vsvfl_spare_data_t;

static void virtual_to_physical_100014(vfl_vsvfl_device_t *_vfl, uint32_t _vBank, uint32_t _vPage, uint32_t *_pCE, uint32_t *_pPage)
{
	uint32_t pBank, pPage;

	pBank = _vBank / _vfl->geometry.num_ce;
	pPage = ((_vfl->geometry.pages_per_block - 1) & _vPage) | (2 * (~(_vfl->geometry.pages_per_block - 1) & _vPage));
	if (pBank & 1)
		pPage |= _vfl->geometry.pages_per_block;

	*_pCE = _vBank % _vfl->geometry.num_ce;
	*_pPage = pPage;
}

static void virtual_to_physical_150011(vfl_vsvfl_device_t *_vfl, uint32_t _vBank, uint32_t _vPage, uint32_t *_pCE, uint32_t *_pPage)
{
	uint32_t pBlock;

	pBlock = 2 * (_vPage / _vfl->geometry.pages_per_block);
	if(_vBank % (2 * _vfl->geometry.num_ce) >= _vfl->geometry.num_ce)
		pBlock++;

	*_pCE = _vBank % _vfl->geometry.num_ce;
	*_pPage = (_vfl->geometry.pages_per_block * pBlock) | (_vPage % 128);
}

static error_t virtual_block_to_physical_block(vfl_vsvfl_device_t *_vfl, uint32_t _vBank, uint32_t _vBlock, uint32_t *_pBlock)
{
	uint32_t pCE, pPage;

	if(!_vfl->virtual_to_physical) {
		bufferPrintf("vsvfl: virtual_to_physical hasn't been initialized yet!\r\n");
		return EINVAL;
	}

	_vfl->virtual_to_physical(_vfl, _vBank, _vfl->geometry.pages_per_block * _vBlock, &pCE, &pPage);
	*_pBlock = pPage / _vfl->geometry.pages_per_block;

	return SUCCESS;
}

static int vfl_is_good_block(uint8_t* badBlockTable, uint32_t block) {
	return (badBlockTable[block / 8] & (1 << (block % 8))) != 0;
}

static uint32_t remap_block(vfl_vsvfl_device_t *_vfl, uint32_t _ce, uint32_t _block, uint32_t *_isGood) {
	DebugPrintf("vsvfl: remap_block: CE %d, block %d\r\n", _ce, _block);

	if(vfl_is_good_block(_vfl->bbt[_ce], _block))
		return _block;

	DebugPrintf("vsvfl: remapping block...\r\n");

	if(_isGood)
		_isGood = 0;

	int pwDesPbn;
	for(pwDesPbn = 0; pwDesPbn < _vfl->geometry.blocks_per_ce - _vfl->contexts[_ce].reserved_block_pool_start * _vfl->geometry.banks_per_ce; pwDesPbn++)
	{
		if(_vfl->contexts[_ce].reserved_block_pool_map[pwDesPbn] == _block)
		{
			uint32_t vBank, vBlock, pBlock;

			/*
			if(pwDesPbn >= _vfl->geometry.blocks_per_ce)
				bufferPrintf("ftl: Destination physical block for remapping is greater than number of blocks per CE!");
			*/

			vBank = _ce + _vfl->geometry.num_ce * (pwDesPbn / (_vfl->geometry.blocks_per_bank_vfl - _vfl->contexts[_ce].reserved_block_pool_start));
			vBlock = _vfl->contexts[_ce].reserved_block_pool_start + (pwDesPbn % (_vfl->geometry.blocks_per_bank_vfl - _vfl->contexts[_ce].reserved_block_pool_start));

			if(FAILED(virtual_block_to_physical_block(_vfl, vBank, vBlock, &pBlock)))
				system_panic("vfl: failed to convert virtual reserved block to physical\r\n");

			return pBlock;
		}
	}

	bufferPrintf("vfl: failed to remap CE %d block 0x%04x\r\n", _ce, _block);
	return _block;
}

static error_t virtual_page_number_to_physical(vfl_vsvfl_device_t *_vfl, uint32_t _vpNum, uint32_t* _ce, uint32_t* _page) {
	uint32_t ce, vBank, ret, bank_offset, pBlock;

	vBank = _vpNum % _vfl->geometry.banks_total;
	ce = vBank % _vfl->geometry.num_ce;

	ret = virtual_block_to_physical_block(_vfl, vBank, _vpNum / _vfl->geometry.pages_per_sublk, &pBlock);

	if(FAILED(ret))
		return ret;

	pBlock = remap_block(_vfl, ce, pBlock, 0);

	bank_offset = _vfl->geometry.bank_address_space * (pBlock / _vfl->geometry.blocks_per_bank);

	*_ce = ce;
	*_page = _vfl->geometry.pages_per_block_2 * (bank_offset + (pBlock % _vfl->geometry.blocks_per_bank))
			+ ((_vpNum % _vfl->geometry.pages_per_sublk) / _vfl->geometry.banks_total);

	return SUCCESS;
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

static int vfl_gen_checksum(vfl_vsvfl_device_t *_vfl, int ce)
{
	vfl_checksum(&_vfl->contexts[ce], (uint32_t)&_vfl->contexts[ce].checksum1 - (uint32_t)&_vfl->contexts[ce],
			&_vfl->contexts[ce].checksum1, &_vfl->contexts[ce].checksum2);
	return TRUE;
}

static int vfl_check_checksum(vfl_vsvfl_device_t *_vfl, int ce)
{
	static int counter = 0;

	counter++;

	uint32_t checksum1;
	uint32_t checksum2;
	vfl_checksum(&_vfl->contexts[ce], (uint32_t)&_vfl->contexts[ce].checksum1 - (uint32_t)&_vfl->contexts[ce],
			&checksum1, &checksum2);

	// Yeah, this looks fail, but this is actually the logic they use
	if(checksum1 == _vfl->contexts[ce].checksum1)
		return TRUE;

	if(checksum2 != _vfl->contexts[ce].checksum2)
		return TRUE;

	return FALSE;
}

static error_t vfl_vsvfl_read_single_page(vfl_device_t *_vfl, uint32_t dwVpn, uint8_t* buffer, uint8_t* spare, int empty_ok, int* refresh_page, uint32_t disable_aes)
{
	vfl_vsvfl_device_t *vfl = CONTAINER_OF(vfl_vsvfl_device_t, vfl, _vfl);

	if(refresh_page)
		*refresh_page = FALSE;

	//VFLData1.field_8++;
	//VFLData1.field_20++;

	uint32_t pCE = 0, pPage = 0;
	int ret;

	ret = virtual_page_number_to_physical(vfl, dwVpn, &pCE, &pPage);

	// FIXME: Hack to get the h2fmi driver to read from the correct CE.
	uint32_t oldCE = pCE;

	pCE = (oldCE % (vfl->geometry.num_ce / 2)) * 2;
	if (oldCE >= vfl->geometry.num_ce / 2)
		pCE++;

	//bufferPrintf("vpn %d CE %d page %d\r\n", dwVpn, pCE, pPage);

	if(FAILED(ret)) {
		bufferPrintf("vfl_vsvfl_read_single_page: virtual_page_number_to_physical returned an error (dwVpn %d)!\r\n", dwVpn);
		return ret;
	}

	// Hack to get reading by absolute page number.
	ret = nand_device_read_single_page(vfl->device, pCE, 0, pPage, buffer, spare, disable_aes);

	if(!empty_ok && ret == ENOENT)
		ret = EIO;
	else if(empty_ok && ret == ENOENT)
		return 1;

	if(ret == EINVAL || ret == EIO)
	{
		ret = nand_device_read_single_page(vfl->device, pCE, 0, pPage, buffer, spare, disable_aes);
		if(!empty_ok && ret == ENOENT)
			return EIO;

		if(ret == EINVAL || ret == EIO)
			return ret;
	}

	if(ret == ENOENT && spare)
		memset(spare, 0xFF, vfl->geometry.bytes_per_spare);

	return ret;
}

static vfl_vsvfl_context_t* get_most_updated_context(vfl_vsvfl_device_t *vfl) {
	int ce = 0;
	int max = 0;
	vfl_vsvfl_context_t* cxt = NULL;

	for(ce = 0; ce < vfl->geometry.num_ce; ce++)
	{
		int cur = vfl->contexts[ce].usn_inc;
		if(max <= cur)
		{
			max = cur;
			cxt = &vfl->contexts[ce];
		}
	}

	return cxt;
}

static uint16_t* VFL_get_FTLCtrlBlock(vfl_device_t *_vfl)
{
	vfl_vsvfl_device_t *vfl = CONTAINER_OF(vfl_vsvfl_device_t, vfl, _vfl);

	vfl_vsvfl_context_t *cxt = get_most_updated_context(vfl);

	if(cxt)
		return cxt->control_block;
	else
		return NULL;
}

static inline error_t vfl_vsvfl_setup_geometry(vfl_vsvfl_device_t *_vfl)
{
#define nand_load(what, where) (nand_device_get_info(nand, (what), &_vfl->geometry.where, sizeof(_vfl->geometry.where)))

	nand_device_t *nand = _vfl->device;
	error_t ret = nand_load(diBlocksPerCE, blocks_per_ce);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("blocks per ce: 0x%08x\r\n", _vfl->geometry.blocks_per_ce);

	ret = nand_load(diBlocksPerBank_dw, blocks_per_bank);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("blocks per bank: 0x%0x\r\n", _vfl->geometry.blocks_per_bank);

	ret = nand_load(diBanksPerCE_dw, banks_per_ce);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("banks per ce: 0x%08x\r\n", _vfl->geometry.banks_per_ce);

	ret = nand_load(diBytesPerPage, bytes_per_page);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("bytes per page: 0x%08x\r\n", _vfl->geometry.bytes_per_page);

	ret = nand_load(diBankAddressSpace, bank_address_space);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("bank address space: 0x%08x\r\n", _vfl->geometry.bank_address_space);

	ret = nand_load(diNumCE, num_ce);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("num ce: 0x%08x\r\n", _vfl->geometry.num_ce);

	ret = nand_load(diPagesPerBlock, pages_per_block);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("pages per block: 0x%08x\r\n", _vfl->geometry.pages_per_block);

	ret = nand_load(diPagesPerBlock2, pages_per_block_2);
	if(FAILED(ret))
		return EINVAL;

	bufferPrintf("pages per block2: 0x%08x\r\n", _vfl->geometry.pages_per_block_2);

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

	_vfl->geometry.pages_per_sublk = _vfl->geometry.pages_per_block * _vfl->geometry.banks_per_ce * _vfl->geometry.num_ce;
	bufferPrintf("pages_per_sublk: 0x%08x\r\n", _vfl->geometry.pages_per_sublk);

	_vfl->geometry.some_sublk_mask =
		_vfl->geometry.some_page_mask * _vfl->geometry.pages_per_sublk;
	bufferPrintf("some_sublk_mask: 0x%08x\r\n", _vfl->geometry.some_sublk_mask);

	_vfl->geometry.banks_total = _vfl->geometry.num_ce * _vfl->geometry.banks_per_ce;
	bufferPrintf("banks_total: 0x%08x\r\n", _vfl->geometry.banks_total);

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

static error_t vfl_vsvfl_open(vfl_device_t *_vfl, nand_device_t *_nand)
{
	vfl_vsvfl_device_t *vfl = CONTAINER_OF(vfl_vsvfl_device_t, vfl, _vfl);

	if(vfl->device || !_nand)
		return EINVAL;

	vfl->device = _nand;
	error_t ret = vfl_vsvfl_setup_geometry(vfl);
	if(FAILED(ret))
		return ret;

	bufferPrintf("vsvfl: Opening %p.\r\n", _nand);

	vfl->contexts = malloc(vfl->geometry.num_ce * sizeof(vfl_vsvfl_context_t));
	memset(vfl->contexts, 0, vfl->geometry.num_ce * sizeof(vfl_vsvfl_context_t));

	vfl->pageBuffer = (uint32_t*) malloc(vfl->geometry.pages_per_block * sizeof(uint32_t));
	vfl->chipBuffer = (uint16_t*) malloc(vfl->geometry.pages_per_block * sizeof(uint16_t));

	uint32_t ce = 0;
	for(ce = 0; ce < vfl->geometry.num_ce; ce++) {
		vfl->bbt[ce] = (uint8_t*) malloc(CEIL_DIVIDE(vfl->geometry.blocks_per_ce, 8));

		bufferPrintf("vsvfl: Checking CE %d.\r\n", ce);

		if(FAILED(nand_device_read_special_page(_nand, ce, "DEVICEINFOBBT\0\0\0",
						vfl->bbt[ce], CEIL_DIVIDE(vfl->geometry.blocks_per_ce, 8))))
		{
			bufferPrintf("vsvfl: Failed to find DEVICEINFOBBT!\r\n");
			return EIO;
		}

		if(ce >= vfl->geometry.num_ce)
			return EIO;

		vfl_vsvfl_context_t *curVFLCxt = &vfl->contexts[ce];
		uint8_t* pageBuffer = malloc(vfl->geometry.bytes_per_page);
		uint8_t* spareBuffer = malloc(vfl->geometry.bytes_per_spare);
		if(pageBuffer == NULL || spareBuffer == NULL) {
			bufferPrintf("ftl: cannot allocate page and spare buffer\r\n");
			return ENOMEM;
		}

		// Any VFLCxt page will contain an up-to-date list of all blocks used to store VFLCxt pages. Find any such
		// page in the system area.

		int i;
		for(i = vfl->geometry.reserved_blocks; i < vfl->geometry.fs_start_block; i++) {
			// so pstBBTArea is a bit array of some sort
			if(!(vfl->bbt[ce][i / 8] & (1 << (i  & 0x7))))
				continue;

			if(SUCCEEDED(nand_device_read_single_page(vfl->device, ce, i, 0, pageBuffer, spareBuffer, 0)))
			{
				memcpy(curVFLCxt->vfl_context_block, ((vfl_vsvfl_context_t*)pageBuffer)->vfl_context_block,
						sizeof(curVFLCxt->vfl_context_block));
				break;
			}
		}

		if(i == vfl->geometry.fs_start_block) {
			bufferPrintf("vsvfl: cannot find readable VFLCxtBlock\r\n");
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

			if(FAILED(nand_device_read_single_page(vfl->device, ce, block, 0, pageBuffer, spareBuffer, 0)))
				continue;

			vfl_vsvfl_spare_data_t *spareData = (vfl_vsvfl_spare_data_t*)spareBuffer;

			if(spareData->meta.usnDec > 0 && spareData->meta.usnDec <= minUsn) {
				minUsn = spareData->meta.usnDec;
				VFLCxtIdx = i;
			}
		}

		if(VFLCxtIdx == 4) {
			bufferPrintf("vsvfl: cannot find readable VFLCxtBlock index in spares\r\n");
			free(pageBuffer);
			free(spareBuffer);
			return EIO;
		}

		// VFLCxts are stored in the block such that they are duplicated 8 times. Therefore, we only need to
		// read every 8th page, and nand_readvfl_cxt_page will try the 7 subsequent pages if the first was
		// no good. The last non-blank page will have the lowest spare.usnDec and highest usnInc for VFLCxt
		// in all the land (and is the newest).
		int page = 8;
		int last = 0;
		for(page = 8; page < vfl->geometry.pages_per_block; page += 8) {
			if(nand_device_read_single_page(vfl->device, ce, curVFLCxt->vfl_context_block[VFLCxtIdx], page, pageBuffer, spareBuffer, 0) != 0) {
				break;
			}

			last = page;
		}

		if(nand_device_read_single_page(vfl->device, ce, curVFLCxt->vfl_context_block[VFLCxtIdx], last, pageBuffer, spareBuffer, 0) != 0) {
			bufferPrintf("vsvfl: cannot find readable VFLCxt\n");
			free(pageBuffer);
			free(spareBuffer);
			return -1;
		}

		// Aha, so the upshot is that this finds the VFLCxt and copies it into vfl->contexts
		memcpy(&vfl->contexts[ce], pageBuffer, sizeof(vfl_vsvfl_context_t));

		// This is the newest VFLCxt across all CEs
		if(curVFLCxt->usn_inc >= vfl->current_version) {
			vfl->current_version = curVFLCxt->usn_inc;
		}

		free(pageBuffer);
		free(spareBuffer);

		// Verify the checksum
		if(vfl_check_checksum(vfl, ce) == FALSE)
		{
			bufferPrintf("vsvfl: VFLCxt has bad checksum.\r\n");
			return EIO;
		}
	}

	// retrieve some global parameters from the latest VFL across all CEs.
	vfl_vsvfl_context_t *latestCxt = get_most_updated_context(vfl);

	// Then we update the VFLCxts on every ce with that information.
	for(ce = 0; ce < vfl->geometry.num_ce; ce++) {
		// Don't copy over own data.
		if(&vfl->contexts[ce] != latestCxt) {
			// Copy the data, and generate the new checksum.
			memcpy(vfl->contexts[ce].control_block, latestCxt->control_block, sizeof(latestCxt->control_block));
			vfl->contexts[ce].usable_blocks_per_bank = latestCxt->usable_blocks_per_bank;
			vfl->contexts[ce].reserved_block_pool_start = latestCxt->reserved_block_pool_start;
			vfl->contexts[ce].ftl_type = latestCxt->ftl_type;
			memcpy(vfl->contexts[ce].field_6CA, latestCxt->field_6CA, sizeof(latestCxt->field_6CA));

			vfl_gen_checksum(vfl, ce);
		}
	}

	// Vendor-specific virtual-from/to-physical functions.
	// Note: support for some vendors is still missing.
	nand_device_t *nand = vfl->device;
	uint32_t vendorType = vfl->contexts[0].vendor_type;

	if(!vendorType)
		if(FAILED(nand_device_get_info(nand, diVendorType, &vendorType, sizeof(vendorType))))
			return EIO;

	switch(vendorType) {
	case 0x10001:
		vfl->geometry.banks_per_ce = 1;
		vfl->virtual_to_physical = virtual_to_physical_100014;
		break;
	
	case 0x100014:
	case 0x120014:
		vfl->geometry.banks_per_ce = 2;
		vfl->virtual_to_physical = virtual_to_physical_100014;
		break;

	case 0x150011:
		vfl->geometry.banks_per_ce = 2;
		vfl->virtual_to_physical = virtual_to_physical_150011;
		break;

	default:
		bufferPrintf("vsvfl: unsupported vendor 0x%06x\r\n", vendorType);
		return EIO;
	}

	if(FAILED(nand_device_set_info(nand, diVendorType, &vendorType, sizeof(vendorType))))
		return EIO;

	vfl->geometry.pages_per_sublk = vfl->geometry.pages_per_block * vfl->geometry.banks_per_ce * vfl->geometry.num_ce;
	vfl->geometry.banks_total = vfl->geometry.num_ce * vfl->geometry.banks_per_ce;
	vfl->geometry.blocks_per_bank_vfl = vfl->geometry.blocks_per_ce / vfl->geometry.banks_per_ce;

	uint32_t banksPerCE = vfl->geometry.banks_per_ce;
	if(FAILED(nand_device_set_info(nand, diBanksPerCE_VFL, &banksPerCE, sizeof(banksPerCE))))
		return EIO;

	bufferPrintf("vsvfl: detected chip vendor 0x%06x\r\n", vendorType);

	// Now, discard the old scfg bad-block table, and set it using the VFL context's reserved block pool map.
	uint32_t bank, i;
	uint32_t num_reserved = vfl->contexts[0].reserved_block_pool_start;
	uint32_t num_non_reserved = vfl->geometry.blocks_per_bank_vfl - num_reserved;

	for(ce = 0; ce < vfl->geometry.num_ce; ce++) {
		memset(vfl->bbt[ce], 0xFF, CEIL_DIVIDE(vfl->geometry.blocks_per_ce, 8));

		for(bank = 0; bank < banksPerCE; bank++) {
			for(i = 0; i < num_non_reserved; i++) {
				uint16_t mapEntry = vfl->contexts[ce].reserved_block_pool_map[bank * num_non_reserved + i];
				uint32_t pBlock;

				if(mapEntry == 0xFFF0)
					continue;

				if(mapEntry < vfl->geometry.blocks_per_ce) {
					pBlock = mapEntry;
				} else if(mapEntry > 0xFFF0) {
					virtual_block_to_physical_block(vfl, ce + bank * vfl->geometry.num_ce, num_reserved + i, &pBlock);
				} else {
					system_panic("vsvfl: bad map table: CE %d, entry %d, value 0x%08x\r\n",
						ce, bank * num_non_reserved + i, mapEntry);
				}

				vfl->bbt[ce][pBlock / 8] &= ~(1 << (pBlock % 8));
			}
		}
	}

	bufferPrintf("vsvfl: VFL successfully opened!\r\n");

	return SUCCESS;
}

static error_t vfl_vsvfl_get_info(vfl_device_t *_vfl, vfl_info_t _item, void *_result, size_t _sz)
{
	vfl_vsvfl_device_t *vfl = CONTAINER_OF(vfl_vsvfl_device_t, vfl, _vfl);
	nand_device_t *nand = vfl->device;

	if(_sz > 4 || _sz == 3) {
		return EINVAL;
	}

	switch(_item) {
	case diPagesPerBlockTotalBanks:
		auto_store(_result, _sz, vfl->geometry.pages_per_sublk);
		return SUCCESS;

	case diSomeThingFromVFLCXT:
		auto_store(_result, _sz, vfl->contexts[0].usable_blocks_per_bank);
		return SUCCESS;

	case diFTLType:
		auto_store(_result, _sz, vfl->contexts[0].ftl_type);
		return SUCCESS;

	case diBytesPerPageFTL:
		nand_device_get_info(nand, diBytesPerPage, _result, _sz);
		return SUCCESS;

	case diMetaBytes0xC:
		auto_store(_result, _sz, 0xC);
		return SUCCESS;

	case diUnkn20_1:
		auto_store(_result, _sz, 1);
		return SUCCESS;

	case diTotalBanks:
		nand_device_get_info(nand, diTotalBanks_VFL, _result, _sz);
		return SUCCESS;

	default:
		return ENOENT;
	}
}

static nand_device_t *vfl_vsvfl_get_device(vfl_device_t *_vfl)
{
	vfl_vsvfl_device_t *vfl = CONTAINER_OF(vfl_vsvfl_device_t, vfl, _vfl);
	return vfl->device;
}

error_t vfl_vsvfl_device_init(vfl_vsvfl_device_t *_vfl)
{
	memset(_vfl, 0, sizeof(*_vfl));

	//memset(&VFLData1, 0, sizeof(VFLData1));

	_vfl->current_version = 0;
	_vfl->vfl.open = vfl_vsvfl_open;

	_vfl->vfl.get_device = vfl_vsvfl_get_device;

	_vfl->vfl.read_single_page = vfl_vsvfl_read_single_page;

	_vfl->vfl.get_ftl_ctrl_block = VFL_get_FTLCtrlBlock;

	_vfl->vfl.get_info = vfl_vsvfl_get_info;

	memset(&_vfl->geometry, 0, sizeof(_vfl->geometry));

#if defined(CONFIG_A4) && !defined(CONFIG_IPAD_1G)
	_vfl->geometry.reserved_blocks = 16;
#else
	_vfl->geometry.reserved_blocks = 1;
#endif

	return 0;
}

void vfl_vsvfl_device_cleanup(vfl_vsvfl_device_t *_vfl)
{
	uint32_t i;

	if(_vfl->contexts)
		free(_vfl->contexts);

	for (i = 0; i < sizeof(_vfl->bbt) / sizeof(void*); i++) {
		if(_vfl->bbt[i])
			free(_vfl->bbt[i]);
	}

	if(_vfl->pageBuffer)
		free(_vfl->pageBuffer);

	if(_vfl->chipBuffer)
		free(_vfl->chipBuffer);

	memset(_vfl, 0, sizeof(*_vfl));
}

vfl_vsvfl_device_t *vfl_vsvfl_device_allocate()
{
	vfl_vsvfl_device_t *ret = malloc(sizeof(*ret));
	vfl_vsvfl_device_init(ret);
	return ret;
}

