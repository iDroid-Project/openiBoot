// This won't compile. But you get the feeling for what it does. :]

uint32_t yaFTL_inited = 0;

typedef struct
{
  uint32_t buffer;
  uint32_t endOfBuffer;
  uint32_t size;
  uint32_t numAllocs;
  uint32_t numRebases;
  uint32_t paddingsSize;
  uint32_t state;
} WMR_BufZone_t;

void WMR_BufZone_Init(WMR_BufZone_t *_zone)
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
uint32_t WMR_Buf_Alloc_ForDMA(WMR_BufZone_t *_zone, uint32_t size)
{
	uint32_t oldSizeRounded;

	if (_zone->state != 1)
		system_panic("WMR_Buf_Alloc_ForDMA: bad state\r\n");

	oldSizeRounded = ROUND_UP(_zone->size, 64);
	_zone->paddingsSize = _zone->paddingsSize + (oldSizeRounded - _zone->size);
	_zone->size = oldSizeRounded + size;
	_zone->numAllocs++;

	return oldSizeRounded;
}

void WMR_BufZone_FinishedAllocs(WMR_BufZone_t *_zone)
{
	void* buff;

	if (_zone->state != 1)
		system_panic("WMR_BufZone_FinishedAllocs: bad state\r\n");

	_zone->size = ROUND_UP(_zone->size, 64);

	buff = wmr_malloc(_zone->size);

	if(!buff)
		system_panic("WMR_BufZone_FinishedAllocs: No buffer.\r\n");

	_zone->buffer = buff;
	_zone->endOfBuffer = (buff + _zone->size);
	_zone->state = 2;
}

void WMR_BufZone_Rebase(WMR_BufZone_t *_zone, uint32_t* ppBuffer)
{
	*ppBuffer = *ppBuffer + _zone->buffer;
	_zone->numRebases++;
}

void WMR_BufZone_FinishedRebases(WMR_BufZone_t *_zone)
{
	if (_zone->numAllocs != _zone->numRebases)
		system_panic("WMR_BufZone_FinishedRebases: _zone->numAllocs != _zone->numRebases\r\n");
}

typedef struct {
	uint32_t unkn0;
	uint16_t unkn1;
	uint16_t unkn2;
	uint16_t unkn3;
	uint8_t unkn4;
	uint8_t unkn5;
} UNKNBUFFERSTRUCT;

typedef struct {
	WMR_BufZone_t zone;
	WMR_BufZone_t segment_info_temp;
	uint16_t unkFactor_0x1; // 38
	uint16_t unkn3A_0x800; // 3A
	uint32_t unknCalculatedValue0; // 3C
	uint32_t unknCalculatedValue1; // 40
	uint32_t total_pages_ftl; // 44
	uint16_t unknCalculatedValue3; // 48
	uint16_t unkn_0x2A; // 4A
	uint32_t unkn_0x2C; // 4C
	uint32_t* ftl2_buffer_x;
	uint16_t unkn_0x34; // 54
	uint32_t unkn_0x3C; // 5C
	uint32_t* ftl2_buffer_x2000;
	uint32_t unk64; // 64
	uint32_t unk6C; // 6C
	uint32_t unk74_4;
	uint32_t unk78_counter;
	uint32_t unk7C_byteMask;
	uint32_t unk80_3;
	uint32_t* unk84_buffer;
	uint32_t* unk88_buffer;
	uint32_t* unk8c_buffer;
	WMR_BufZone_t ftl_buffer2;
	uint32_t unkAC_2;
	uint32_t unkB0_1;
	uint32_t* unkB4_buffer;
	uint32_t* unkB8_buffer;
	uint32_t unkStruct_ftl[10]; // BC
	uint32_t unkC4;
	uint32_t unkD0;
	uint32_t* indexPageBuf; // EC
	UNKNBUFFERSTRUCT* unknBuffer4_ftl; // F0
	UNKNBUFFERSTRUCT* unknBuffer2_ftl; // F4
	uint32_t* pageBuffer2;
	uint32_t* buffer3;
	uint32_t* buffer4;
	uint8_t* buffer5;
	uint32_t* buffer6;
	uint32_t* buffer7;
	uint32_t* buffer8;
	uint32_t* buffer9;
	uint32_t* buffer10;
	uint32_t* buffer11;
	uint32_t* buffer12;
	uint32_t* buffer13;
	uint32_t* buffer14;
	uint32_t* buffer15;
	uint32_t* buffer16;
	uint32_t* buffer17;
	uint32_t* pageBuffer0;
	uint32_t* pageBuffer1;
	uint32_t unk140_n1;
	uint32_t* buffer19;
	uint32_t* unknBuffer3_ftl; // 148
	uint32_t* buffer20;
	uint32_t* buffer18;
	uint32_t unkn_pageOffset;
	uint32_t unknCalculatedValue4;
	uint32_t unknCalculatedValue5;
	uint32_t unknCalculatedValue6;
	uint32_t unknCalculatedValue7;
	uint32_t unknCalculatedValue8;
	uint32_t unknCalculatedValue9;
	uint32_t unk170_n1;
	uint16_t unkn_vflcxt69a;
	uint16_t unkn_vflcxt69c;
	uint16_t unkn_vflcxt69e;
	uint16_t unkn_1;
	uint32_t unk17C_0x0;
	uint32_t unk180_n1;
	uint32_t unk184_0xA;
	uint8_t unk188_0x63;
} YAFTL_INFO;
YAFTL_INFO yaftl_info;

typedef struct {
	uint16_t pages_per_block_total_banks;
	uint16_t usable_blocks_per_bank;
	uint16_t bytes_per_page_ftl;
	uint16_t meta_struct_size_0xC;
	uint16_t total_banks;
	uint32_t total_usable_pages;
} NAND_GEOMETRY_FTL;
NAND_GEOMETRY_FTL nand_geometry_ftl;

typedef struct {
	char[4] version; // 0
	uint32_t unknCalculatedValue0; // 4
	uint32_t total_pages_ftl; // 8
	uint32_t unkn_0x2C; // C
	uint32_t cxt_unkn0; // 10 // placeholder
	uint32_t unkn_0x3C; // 14
	uint32_t unk6C; // 18
	uint32_t unkStruct_ftl_1; // 1C
	uint32_t unkStruct_ftl_4; // 20
	uint32_t unkStruct_ftl_0; // 24
	uint32_t unkStruct_ftl_3; // 28
	uint32_t unk184_0xA; // 2C
	uint32_t cxt_unkn1[10]; // placeholder
	uint16_t unknCalculatedValue3; // 5C
	uint16_t unkFactor_0x1; // 5E
	uint16_t unkn3A_0x800; // 60
	uint16_t unkn_0x2A; // 62
	uint16_t unkn_0x34; // 66
	uint16_t unk64; // 68
	uint32_t cxt_unkn2[11]; // placeholder
	uint8_t unk188_0x63; // 94
} YAFTL_CXT;

uint32_t BTOC_Init() {
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
		yaftl_info.unkB8_buffer[i] = wmr_malloc(nand_geometry_ftl.pages_per_block_total_banks<<2);
		memset(yaftl_info.unkB8_buffer[i], 0xFF, nand_geometry_ftl.pages_per_block_total_banks<<2);
	}

	yaftl_info.unk7C_byteMask = (1 << yaftl_info.unk74_4) - 1;
	yaftl_info.unk80_3 = 3;

	WMR_BufZone_Init(&yaftl_info.ftl_buffer2);

	for(i = 0; i < yaftl_info.unk74_4; i++) {
		yaftl_info.unk84_buffer[i] = WMR_Buf_Alloc_ForDMA(&yaftl_info.ftl_buffer2, nand_geometry_ftl.bytes_per_page_ftl * unknFactor_0x1);
	}

	if(!WMR_BufZone_FinishedAllocs(&yaftl_info.ftl_buffer2))
		return -1;

	for(i = 0; i < yaftl_info.unk74_4; i++) {
		WMR_BufZone_Rebase(&yaftl_info.ftl_buffer2, &yaftl_info.unk84_buffer[i]);
	}

	WMR_BufZone_FinishedRebases(&yaftl_info.ftl_buffer2);

	return 0;
}

uint32_t BTOC_Alloc(uint32_t _arg0, uint32_t _unused_arg1) {
	int32_t inited = 0x7FFFFFFF;
	yaftl_info.unkB0_1 = (yaftl_info.unkB0_1 + 1) % yaftl_info.unkAC_2;
	yaftl_info.unkB4_buffer[yaftl_info.unkB0_1] = _arg0;

	memset(yaftl_info.unkB8_buffer[yaftl_info.unkB0_1], 0xFF, nand_geometry_ftl.pages_per_block_total_banks<<2);

	yaftl_info.unk78_counter++;

	uint32_t found = 0;
	uint32_t i;
	for(i = 0; i < yaftl_info.unk74_4; i++) {
		if((yaftl_info.unk80_3 & 1<<i) && (yaftl_info.unk7C_byteMask & 1<<i) && (inited > (int32_t)yaftl_info.unk8c_buffer[i])) {
			found = i;
			inited = yaftl_info.unk8c_buffer[i];
		}
	}

	if(!found)
		system_panic("BTOC_Alloc: Couldn't allocate a BTOC.\r\n");

	yaftl_info.unk7C_byteMask &= (~(1<<found));
	yaftl_info.unk88_buffer[found] = _arg0;
	yaftl_info.unk8c_buffer[found] = yaftl_info.unk78_counter;
	return yaftl_info.unk84_buffer[found];
}

void yaftl_loop_thingy() {
	uint32_t i;
	for (i = 0; i < yaftl_info.unk74_4; i++) {
		if(yaftl_info.unk88_buffer[i] == ~3)
			yaftl_info.unk88_buffer[i] = yaftl_info.unkn_0x2C;
		if(yaftl_info.unk88_buffer[i] == ~2)
			yaftl_info.unk88_buffer[i] = yaftl_info.unkn_0x3C;
	}
}

uint32_t YAFTL_readCxtInfo(uint32_t _page, uint32_t* _ptr, uint32_t _arg2, uint32_t arg3) {
	uint32_t readPage;

	if(YAFTL_readPage(_page, _ptr, yaftl_info.buffer5, 0, 1, _arg3))
		return ERROR_ARG;

	if(!(yaftl_info.buffer5[9] & 0x20))
		return ERROR_ARG;

	YAFTL_CXT yaftl_cxt = (YAFTL_CXT*)_ptr;

	if(strncmp(yaftl_cxt->version, "CX01", 4)) {
		bufferPrintf("YAFTL: Wrong version of CXT.\r\n");
		return ERROR_ARG;
	}

	// More to come.

	if(_arg3) {
		yaftl_info.unkFactor_0x1 = yaftl_cxt->unkFactor_0x1;
		yaftl_info.unkn3A_0x800 = yaftl_cxt->unkn3A_0x800;
		yaftl_info.unknCalculatedValue0 = yaftl_cxt->unknCalculatedvalue0;
		yaftl_info.unknCalculatedValue3 = yaftl_cxt->unknCalculatedValue3;
		yaftl_info.total_pages_ftl = yaftl_cxt->total_pages_ftl;
		yaftl_info.unkn_0x2A = yaftl_cxt->unkn_0x2A;
		yaftl_info.unkn_0x2C = yaftl_cxt->unkn_0x2C;
		yaftl_info.unkn_0x34 = yaftl_cxt->unkn_0x34;
		yaftl_info.unkn_0x3C = yaftl_cxt->unkn_0x3C;
		yaftl_info.unkStruct_ftl[0] = yaftl_cxt->unkStruct_ftl_0;
		yaftl_info.unkStruct_ftl[1] = yaftl_cxt->unkStruct_ftl_1;
		yaftl_info.unkStruct_ftl[3] = yaftl_cxt->unkStruct_ftl_3;
		yaftl_info.unkStruct_ftl[4] = yaftl_cxt->unkStruct_ftl_4;
		yaftl_info.unk64 = yaftl_cxt->unk64;
		yaftl_info.unk6C = yaftl_cxt->unk6C;
		yaftl_info.unk184_0xA = yaftl_cxt->unk184_0xA;
		yaftl_info.unk188_0x63 = yaftl_cxt->unk188_0x63;

		readPage = _page;
		uint32_t i;
		for (i = 0; i < yaftl_info.unkFactor_0x1; i++) {
			if(FAILED(YAFTL_readPage(++readPage, (uint32_t*)(yaftl_info.bytes_per_page_ftl * i + yaftl_info.ftl2_buffer_x), yaftl_info.buffer5, 0, 1, 0)) ||
					!(yaftl_info.buffer5[9] & 0x20))
				return ERROR_ARG;
		}
		for (i = 0; i < yaftl_info.unkFactor_0x1; i++) {
			if(FAILED(YAFTL_readPage(++readPage, (uint32_t*)(yaftl_info.bytes_per_page_ftl * i + yaftl_info.ftl2_buffer_x2000), yaftl_info.buffer5, 0, 1, 0)) ||
					!(yaftl_info.buffer5[9] & 0x20))
				return ERROR_ARG;
		}
		readPage+= (yaftl_info.unkFactor_0x1+1);
		uint32_t unkncalcv3 = yaftl_info.unknCalculatedValue3;
		for (i = 0; i < yaftl_info_unknCalculatedValue4; i++) {
			if(FAILED(YAFTL_readPage(readPage+i, _ptr, yaftl_info.buffer5, 0, 1, 0)) ||
					!(yaftl_info.buffer5[9] & 0x20))
				return ERROR_ARG;

			uint32_t j = 0;
			uint32_t indexPageStart = (i * (yaftl_info.bytes_per_page_ftl>>2))/sizeof(uint32_t);
			while(j < unkncalcv3 && j <= (yaftl_info.bytes_per_page_ftl>>2)) {
				yaftl_info.indexPageBuf[indexPageStart+j*2] = _ptr[j++];
			}
			unkncalcv3 -= yaftl_info.bytes_per_page_ftl>>2;
		}
		readPage+=yaftl_info.unknCalculatedValue4;
		uint32_t usablblpb = yaftl_info.usable_blocks_per_bank;
		for (i = 0; i < yaftl_info.unknCalculatedValue5; i++) {
			if(FAILED(YAFTL_readPage(readPage+i, _ptr, yaftl_info.buffer5, 0, 1, 0)) ||
					!(yaftl_info.buffer5[9] & 0x20))
				return ERROR_ARG;

			uint32_t j = 0;
			while(j < usablblpb && j <= yaftl_info.bytes_per_page_ftl) {
				yaftl_info.unknBuffer2_ftl[(i*yaftl_info.bytes_per_page_ftl)/sizeof(UNKNBUFFERSTRUCT)+j].unkn4 = ((uint8_t*)_ptr)[j++];
			}
			usablblpb -= yaftl_info.bytes_per_page_ftl;
		}
	} else {
		readPage = _page + yaftl_info.unknCaclculatedValue4+2 + yaftl_info.unkFactor_0x1*2
	}
	readPage+=yaftl_info.unknCalculatedValue5;
	for (i = 0; i < yaftl_info.unknCalculatedValue6; i++) {
		if(FAILED(YAFTL_readPage(readPage+i, _ptr, yaftl_info.buffer5, 0, 1, 0)) ||
				!(yaftl_info.buffer5[9] & 0x20))
			return ERROR_ARG;

		uint32_t j = 0;
		uint32_t usablblpb = yaftl_info.usable_blocks_per_bank;
		while(j < usablblpb && j <= (yaftl_info.bytes_per_page_ftl>>1)) {
			yaftl_info.unknBuffer2_ftl[(i*(yaftl_info.bytes_per_page_ftl>>1))/sizeof(UNKNBUFFERSTRUCT)+j].unkn3 = ((uint16_t*)_ptr)[j++];
			usablblpb -= yaftl_info.bytes_per_page_ftl;
		}
		usablblpb -= yaftl_info.bytes_per_page_ftl>>1;
	}
	readPage+=yaftl_info.unknCalculatedValue6;
	for (i = 0; i < yaftl_info.unknCalculatedValue7; i++) {
		if(FAILED(YAFTL_readPage(readPage+i, _ptr, yaftl_info.buffer5, 0, 1, 0)) ||
				!(yaftl_info.buffer5[9] & 0x20))
			return ERROR_ARG;

		uint32_t j = 0;
		uint32_t usablblpb = yaftl_info.usable_blocks_per_bank;
		while(j < usablblpb && j <= (yaftl_info.bytes_per_page_ftl>>2)) {
			yaftl_info.unknBuffer2_ftl[(i*(yaftl_info.bytes_per_page_ftl>>2))/sizeof(UNKNBUFFERSTRUCT)+j].unkn0 = _ptr[j++];
		}
		usablblpb -= yaftl_info.bytes_per_page_ftl>>2;
	}
	readPage+=yaftl_info.unknCalculatedValue7;
	if(_arg2) {
		for (i = 0; i < yaftl_info.unknCalculatedValue9; i++) {
			if(FAILED(YAFTL_readPage(readPage+i, _ptr, yaftl_info.buffer5, 0, 1, 0)) ||
					!(yaftl_info.buffer5[9] & 0x20))
				return ERROR_ARG;

			uint32_t usablblpb = yaftl_info.usable_blocks_per_bank;
			while(j < usablblpb && j <= (yaftl_info.bytes_per_page_ftl>>1)) {
				yaftl_info.unknBuffer2_ftl[(i*(yaftl_info.bytes_per_page_ftl>>1))/sizeof(UNKNBUFFERSTRUCT)+j].unkn2 = ((uint16_t*)_ptr)[j];
				yaftl_info.unkD0 += yaftl_info.unknBuffer2_ftl[(i*(yaftl_info.bytes_per_page_ftl>>1))/sizeof(UNKNBUFFERSTRUCT)+j].unkn2;
				j++;
			}
			usablblpb -= yaftl_info.bytes_per_page_ftl>>1;
		}
		readPage+=yaftl_info_unknCalculatedValue9;
		for (i = 0; i < yaftl_info.unknCalculatedValue8; i++) {
			if(FAILED(YAFTL_readPage(readPage+i, _ptr, yaftl_info.buffer5, 0, 1, 0)) ||
					!(yaftl_info.buffer5[9] & 0x20))
				return ERROR_ARG;

			uint32_t usablblpb = yaftl_info.usable_blocks_per_bank;
			while(j < usablblpb && j <= (yaftl_info.bytes_per_page_ftl>>1)) {
				yaftl_info.unknBuffer2_ftl[(i*(yaftl_info.bytes_per_page_ftl>>1))/sizeof(UNKNBUFFERSTRUCT)+j].unkn1 = ((uint16_t*)_ptr)[j];
				yaftl_info.unkC4 += yaftl_info.unknBuffer2_ftl[(i*(yaftl_info.bytes_per_page_ftl>>1))/sizeof(UNKNBUFFERSTRUCT)+j].unkn1;
				j++;
			}
	}
	return 0;
}

uint32_t YAFTL_readPage(uint32_t _page, uint32_t* _ptr, uint32_t* _unkn_ptr, uint32_t _arg0, uint32_t _arg1, uint32_t _arg2) {
	uint32_t unk1 = 0;
	uint32_t* data_array[2] = { _ptr, (_unkn_ptr ? _unkn_ptr : yaftl_info.buffer18) };
	if(FAILED(vfl_vsvfl_read_single_page(_page, data_array, _arg1, _arg2, &unk1, 0, _arg0))) {
		bufferPrintf("YAFTL_readPage: We got read failure.\r\n");
		return ERROR_ARG;
	}

	yaftl_info.unknBuffer2_ftl[page / nand_geometry_ftl.pages_per_block_total_banks].unkn3++;
	return 0;
}

void YAFTL_Init() {
	if(yaFTL_inited)
		system_panic("Oh shit, yaFTL already inited!\r\n");

	memset(yaftl_info, 0, sizeof(yaftl_info)); // 0x358

	yaftl_info.unkn140_n1 = -1;
	yaftl_info.unkn170_n1 = -1;
	yaftl_info.unkn184_0xA = 0xA;
	yaftl_info.unkn188_0x63 = 0x63;

	memcpy(pVSVFL_fn_table, VSVFL_fn_table, 0x2C);

	nand_geometry_ftl.pages_per_block_total_banks = vfl_device_get_info(_dev, diPagesPerBlockTotalBanks);
	nand_geometry_ftl.usable_blocks_per_bank = vfl_device_get_info(_dev, diSomeThingFromVFLCXT);
	nand_geometry_ftl.bytes_per_page_ftl = vfl_device_get_info(_dev, diUnkn140x2000);
	nand_geometry_ftl.meta_struct_size_0xC = vfl_device_get_info(_dev, diMetaBytes0xC) * vfl_device_get_info(_dev, diUnkn20_1);
	nand_geometry_ftl.total_banks_ftl = vfl_device_get_info(_dev, diTotalBanks);
	nand_geometry_ftl.total_usable_pages = nand_geometry_ftl.pages_per_block_total_banks * nand_geometry_ftl.usable_blocks_per_bank;

	if(nand_geometry_ftl.meta_struct_size_0xC != 0xC)
		system_panic("MetaStructSize not equal 0xC!\r\n");

	uint32_t i = 1;
	while(((i * nand_geometry_ftl.bytes_per_page_ftl) - (i << 2)) < ((nand_geometry_ftl.pages_per_block_total_banks - i) << 2)) { i++; };
	yaftl_info.unkFactor_0x1 = i;
	yaftl_info.unkn3A_0x800 = nand_geometry_ftl.bytes_per_page_ftl >> 2;
	WMR_BufZone_Init(&yaftl_info.zone);
	yaftl_info.pageBuffer0 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, nand_geometry_ftl.bytes_per_page_ftl);
	yaftl_info.pageBuffer1 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, nand_geometry_ftl.bytes_per_page_ftl);
	yaftl_info.pageBuffer2 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, nand_geometry_ftl.bytes_per_page_ftl * yaftl_info.unkFactor_0x1);
	yaftl_info.buffer3 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer4 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer5 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer6 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer7 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer8 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer9 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer10 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer11 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer12 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer13 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer14 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer15 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer16 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer17 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer18 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, 0xC);
	yaftl_info.buffer19 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, nand_geometry_ftl.total_banks_ftl * 0x180);
	yaftl_info.buffer20 = WMR_Buf_Alloc_ForDMA(&yaftl_info.zone, nand_geometry_ftl.pages_per_block_total_banks * 0xC);
	WMR_BufZone_FinishedAllocs(&yaftl_info.zone);

	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.pageBuffer0);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.pageBuffer1);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.pageBuffer2);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer3);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer4);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer5);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer6);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer7);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer8);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer9);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer10);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer11);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer12);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer13);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer14);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer15);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer16);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer17);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer18);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer19);
	WMR_BufZone_Rebase(&yaftl_info.zone, yaftl_info.buffer20);
	WMR_BufZone_FinishedRebases(&yaftl_info.zone);

	yaftl_info.unknCalculatedValue0 = (((nand_geometry_ftl.pages_per_block_total_banks - yaftl_info.unkFactor_0x1) - (nand_geometry_ftl.usable_blocks_per_bank - 8)) / ((nand_geometry_ftl.pages_per_block_total_banks - yaftl_info.unkFactor_0x1) * yaftl_info.unkn3A_0x800) * 3);
	if(((nand_geometry_ftl.pages_per_block_total_banks - yaftl_info.unkFactor_0x1) - (nand_geometry_ftl.usable_blocks_per_bank - 8)) % ((nand_geometry_ftl.pages_per_block_total_banks - yaftl_info.unkFactor_0x1) * yaftl_info.unkn3A_0x800))
		yaftl_info.unknCalculatedValue0 = yaftl_info.unknCalculatedValue0+3;

	yaftl_info.unknCalculatedValue1 = nand_geometry_ftl.usable_blocks_per_bank - yaftl_info.unknCalculatedValue0 - 3;

	yaftl_info.total_pages_ftl = ((nand_geometry_ftl.pages_per_block_total_banks - yaftl_info.unkFactor_0x1) - (nand_geometry_ftl.usable_blocks_per_bank - 8)) - (nand_geometry_ftl.usable_blocks_per_bank * nand_geometry_ftl.pages_per_block_total_banks);

	yaftl_info.unknCalculatedValue3 = ((nand_geometry_ftl.pages_per_block_total_banks * nand_geometry_ftl.usable_blocks_per_bank) << 2) / nand_geometry_ftl.bytes_per_page_ftl;
	yaftl_info.indexPageBuf = wmr_malloc(yaftl_info.unknCalculatedValue3 << 3);
	if(!yaftl_info.indexPageBuf)
		system_panic("No buffer.\r\n");

	yaftl_info.unknBuffer2_ftl = wmr_malloc(nand_geometry_ftl.usable_blocks_per_bank * 0xC);
	if(!yaftl_info.unknBuffer2_ftl)
		system_panic("No buffer.\r\n");

	yaftl_info.unknBuffer3_ftl = wmr_malloc(nand_geometry_ftl.pages_per_block_total_banks << 2);
	if(!yaftl_info.unknBuffer3_ftl)
		system_panic("No buffer.\r\n");

	yaftl_info.unknCalculatedValue4 = (yaftl_info.unknCalculatedValue3 << 2) / nand_geometry_ftl.bytes_per_page_ftl;
	if((yaftl_info.unknCalculatedValue3 << 2) % nand_geometry_ftl.bytes_per_page_ftl)
		yaftl_info.unknCalculatedValue4++;

	yaftl_info.unknCalculatedValue5 = (yaftl_info.usable_blocks_per_bank / nand_geometry_ftl.bytes_per_page_ftl) & 0xFFFF;
	if((yaftl_info.usable_blocks_per_bank % nand_geometry_ftl.bytes_per_page_ftl) & 0xFFFF)
		yaftl_info.unknCalculatedValue5++;

	yaftl_info.unknCalculatedValue6 = (yaftl_info.usable_blocks_per_bank << 1) / nand_geometry_ftl.bytes_per_page_ftl;
	yaftl_info.unknCalculatedValue8 = yaftl_info.unknCalculatedValue6;
	yaftl_info.unknCalculatedValue9 = yaftl_info.unknCalculatedValue6;
	uint32_t unknv6mod = (yaftl_info.usable_blocks_per_bank << 1) % nand_geometry_ftl.bytes_per_page_ftl;
	if(unknv6mod) {
		yaftl_info.unknCalculatedValue6++;
		yaftl_info.unknCalculatedValue8++;
		yaftl_info.unknCalculatedValue9++;
	}

	yaftl_info.unknCalculatedValue7 =  (yaftl_info.usable_blocks_per_bank << 2) / nand_geometry_ftl.bytes_per_page_ftl;
	if((yaftl_info.usable_blocks_per_bank << 2) % nand_geometry_ftl.bytes_per_page_ftl)
		yaftl_info.unknCalculatedValue7++;

	yaftl_info.unkn_pageOffset = yaftl_info.unknCalculatedValue4 + yaftl_info.unknCalculatedValue5 + yaftl_info.unknCalculatedValue6 + yaftl_info.unknCalculatedValue7 + yaftl_info.unknCalculatedValue8 + yaftl_info.unknCalculatedValue9 + ((yaftl_info.unkFactor_0x1 << 1)+1);

	memset(yaftl_info.unkStruct_ftl, 0, 0x20);

	if(BTOC_Init())
		system_panic("Actually (and before) it should free something, but,.. hey, we don't fuck up, do we?\r\n");

	yaftl_info.ftl2_buffer_x = BTOC_Alloc(~2, 1);
	yaftl_info.ftl2_buffer_x2000 = BTOC_Alloc(~1, 0);

	yaFTL_inited = 1;

	return 0;
}

uint32_t YAFTL_Open(uint32_t* pagesAvailable, uint32_t* bytesPerPage, uint32_t signature_bit) {
	uint16_t vfl_bytes[3];
	uint32_t unkn1 = 1;

	memset(yaftl_info.indexPageBuf, 0xFF, yaft_info.unknCalculatedValue3<<3);
	memset(yaftl_info.unknBuffer2_ftl, 0xFF, nand_geometry_ftl.usable_blocks_per_bank * 0xC);

	uint32_t i;
	for (i = 0; i <= nand_geometry_ftl.usable_blocks_per_bank) {
		yaftl_info.unknBuffer2_ftl[i].unkn0 = 0;
		yaftl_info.unknBuffer2_ftl[i].unkn1 = 0;
		yaftl_info.unknBuffer2_ftl[i].unkn2 = 0;
		yaftl_info.unknBuffer2_ftl[i].unkn3 = 0;
		yaftl_info.unknBuffer2_ftl[i].unkn4 = 0;
		yaftl_info.unknBuffer2_ftl[i].unkn5 = 0;
	}

	memcpy(vfl_bytes, some_vsvfl_sub_to_get_most_recent_vflcxt_offset_0x69a(), sizeof(vfl_bytes));
	if (vfl_bytes[0] != 0xFFFF) { // !restoreMount
		// mark first 3 blocks as dead
		yaftl_info.unkn_vflcxt69a = vfl_bytes[0];
		yaftl_info.unkn_vflcxt69c = vfl_bytes[1];
		yaftl_info.unkn_vflcxt69e = vfl_bytes[2];

		yaftl_info.unknBuffer2_ftl[yaftl_info.unkn_vflcxt69a].unkn4 = 2;
		yaftl_info.unknBuffer2_ftl[yaftl_info.unkn_vflcxt69c].unkn4 = 2;
		yaftl_info.unknBuffer2_ftl[yaftl_info.unkn_vflcxt69e].unkn4 = 2;

		if(!yaftl_info.pageBuffer0)
			system_panic("This can't happen. This shouldn't happen. Whatever, it's doing something else then.\r\n");
		else {
			// wtf has been done before
			yaftl_info.unknBuffer2_ftl[vfl_bytes[0]].unkn4 = 2;
			yaftl_info.unknBuffer2_ftl[vfl_bytes[1]].unkn4 = 2;
			yaftl_info.unknBuffer2_ftl[vfl_bytes[2]].unkn4 = 2;

			// find valid ftlcxt page
			uint32_t sth1 = 0;
			uint32_t sth2 = 0;
			for (i = 0; i < 3; i++) {
				if(!YAFTL_readPage(vfl_bytes[i] * yaftl_info.pages_per_block_total_banks, yaftl_info.pageBuffer0, yaftl_info.buffer6, 0, 1, 0)) {
					if(yaftl_info.buffer6[1] != 0xFFFFFFFF && yaftl_info.buffer6[1] > sth1) {
						sth1 = yaftl_info.buffer6[1];
						sth2 = vfl_bytes[i];
					}
				}
			}
			uint32_t some_val;
			if(sth1) {
				yaftl_info.unknBuffer2_ftl[sth2].unkn4 = 16;
				i = 0;
				while(1) {
					if(((uint16_t)yaftl_info.unkn_pageOffset)+i < nand_geometry_ftl.pages_per_block_total_banks
							&& !YAFTL_readPage(yaftl_info.pages_per_block_total_banks * sth2 + i, yaftl_info.pageBuffer0, yaftl_info.buffer6, 0, 1, 0)) {
						if(YAFTL_readPage(yaftl_info.pages_per_block_total_banks*sth2+yaftl_info.unkn_pageOffset+i, yaftl_info.pageBuffer0, yaftl_info.buffer6, 0, 1, 0) == 1) {
							yaftl_info.unkn170_n1 = yaftl_info.pages_per_block_total_banks*sth2+i;
							if(YAFTL_readCxtInfo(yaftl_info.pages_per_block_total_banks*sth2+i, yaftl_info.pageBuffer0, 1, &unkn1))
								some_val = 5;
							else
								some_val = 0;
							break;
						}
					} else {
						YAFTL_readCxtInfo(yaftl_info.pages_per_block_total_banks*sth2+(~yaftl_info.unkn_pageOffset)+i, yaftl_info.pageBuffer0, 0, &unkn1);
						some_val = 5;
					}
				}
			} else {
				yaftl_info.unknBuffer2_ftl[vfl_bytes[0]] = 16;
				some_val = 5;
			}

			if(unkn1 != 1) {
				bufferPrintf("YAFTL_Open: unsupported low-level format version.\r\n");
				return -1;
			}

			if(some_val || signature_bit) {
				bufferPrintf("Ach, fu, 0x80000001\r\n");
				yaftl_info.unk184_0xA *= 100;
			}

			yaftl_info.unknBuffer4_ftl = wmr_malloc(yaftl_info.unk184_0xA*0xC);
			if(!yaftl_info.unknBuffer4_ftl) {
				bufferPrintf("Ach, fu, 0x80000001\r\n");
				return -1;
			}

			WMR_BufZone_Init(&yaftl_info.segment_info_temp);
			for(i = 0; i < yaftl_info.unk184_0xA; i++) {
				yaftl_info.unknBuffer4_ftl.unkn0 = WMR_Buf_Alloc_ForDMA(&yaftl_info.segment_info_temp, nand_geometry_ftl.bytes_per_page_ftl);
			}
			if(!WMR_BufZone_FinishedAllocs(&yaftl_info.segment_info_temp))
				return -1;
			for(i = 0; i < yaftl_info.unk184_0xA; i++) {
				WMR_BufZone_Rebase(&yaftl_info.segment_info_temp, yaftl_info.unknBuffer4_ftl[i]);
				yaftl_info.unknBuffer4_ftl[i].unkn1 = -1;
				yaftl_info.unknBuffer4_ftl[i].unkn2 = 0;
				yaftl_info.unknBuffer4_ftl[i].unkn3 = 0;
				yaftl_info.unknBuffer4_ftl[i].unkn4 = -1;
				yaftl_info.unknBuffer4_ftl[i].unkn5 = -1;
				memset(yaftl_info.unknBuffer4_ftl[i].unkn0, 0xFF, nand_geometry_ftl.bytes_per_page_ftl);
			}
			WMR_BufZone_FinishedRebases(&yaftl_info.segment_info_temp);

			pagesAvailable = yaftl_info.total_pages_ftl*0.99;
			bytesPerPage = nand_geometry_ftl.bytes_per_page_ftl;
			yaftl_info.unkn17C_0x0 = 0;
			yaftl_info.unkn180_n1 = -1;
			for (i = 0; i <= nand_geometry_ftl.usable_blocks_per_bank; i++) {
				if(yaftl_info.unknBuffer2_ftl[i].unkn4 != 16 && yaftl_info.unknBuffer2_ftl[i].unkn4 != 2) {
					if(yaftl_info.unknBuffer2_ftl[i].unkn0 > yaftl_info.unkn17C_0x0)
						yaftl_info.unkn17C_0x0 = yaftl_info.unknBuffer2_ftl[i].unkn0;
					if(yaftl_info.unknBuffer2_ftl[i].unkn0 < yaftl_info.unkn180_n1)
						yaftl_info.unkn180_n1 = yaftl_info.unknBuffer2_ftl[i].unkn0;
				}
			}
			SetupFreeAndAllocd();
			yaftl_loop_thingy();
			return 0;
		}
	} else {
		// do restore
		// NO! Better not! Not yet...
	}
}
