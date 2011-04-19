#include "openiboot.h"
#include "commands.h"
#include "util.h"
#include "h2fmi.h"
#include "a4/vfl.h"

enum {
	OldStyle = 0x64,
	NewStyle = 0x65,
};

enum {
	BUF_FREE = 0x10000000,
	BUF_ALLOCATED = 0x10000001,
	BUF_SPARE = 0x10000002,
};

typedef struct Buff {
	uint32_t* data;
	uint32_t* spare;
	uint32_t eStatus;
	uint32_t* buffer;
} Buff;

typedef struct BuffList {
	Buff pBuf[0xA];
} BuffList;


typedef struct BuffCxt {
	BuffList* pBuffList; // 0xA0
	uint32_t nFreeBuffers;
	uint32_t dwNumOfBuffers;
} BuffCxt;

BuffCxt* pBufCxt;
uint32_t wmr_inited = 0;
uint32_t dataSize;
uint32_t spareSize;
uint32_t FPart_inited = 0;
uint32_t specialBlockCache_unkvar;
uint32_t specialBlockCache_updated;
uint32_t* pSpecialBlockCache;

Buff* BUF_Get(uint32_t eStatus);
Buff* BUF_Release(Buff* _buff);

uint32_t* wmr_malloc(uint32_t size) {
	uint32_t* buffer = memalign(0x40, size);

	if(!buffer)
		system_panic("wmr_malloc: failed\r\n");

	memset(buffer, 0, size);

	return buffer;
}

uint32_t update_specialBlockCache(uint32_t ce, char* infoTypeName, uint32_t _arg) {
	uint32_t* tempBuffer[16];

	if(!infoTypeName || !*pSpecialBlockCache)
		return 0;

	if(ce > h2fmi_geometry.num_ce)
		return 0;

	memset(tempBuffer, 0xFF, 16);

	uint32_t i;
	for (i = 0; i < 5; i++) {
		if(!strncmp(infoTypeName, (char*)(*pSpecialBlockCache + (ce << 8) + (i << 5)), 16))
			break;
		if(!strncmp((char*)(*pSpecialBlockCache + (ce << 8) + (i << 5)), (char*)tempBuffer, 16))
			break;
		if(i == 4)
			return 0;
	}

	memcpy((uint32_t*)(*pSpecialBlockCache + (ce << 8) + (i << 5)), infoTypeName, 16);

	uint32_t j = 0;
	if(((uint32_t*)(*pSpecialBlockCache + (ce << 8) + (i << 5)))[5]) {
		if(!((uint32_t*)(*pSpecialBlockCache + (ce << 8) + (i << 5)))[6])
			j = 1;
		else
			return 0;
	}

	((uint32_t*)(*pSpecialBlockCache + (((i << 2) + j) << 3) + (ce << 8)))[4] = _arg;
	((uint32_t*)(*pSpecialBlockCache + (((i << 2) + j) << 3) + (ce << 8)))[5] = 1;

	return 1;
}

uint32_t sub_5FF2508C(uint32_t ce, uint32_t specialBlockNumber, uint32_t* headerBuffer, uint32_t* dataBuffer, uint32_t bytesToRead, char* infoTypeName, uint32_t nameSize, uint32_t zero1, uint32_t zero2) {
	Buff* buff = BUF_Get(0x10000000);
	if(!buff)
		return 0;

	if(!infoTypeName)
		return 0;

	uint32_t i;
	uint32_t page = 0;
	for (i = 0; i <= 2 && page <= h2fmi_geometry.pages_per_block; i++) {
		uint32_t specialPage = h2fmi_geometry.pages_per_block * ((h2fmi_geometry.unk14 * ((specialBlockNumber/h2fmi_geometry.blocks_per_bank) & 0xFFFF) + (specialBlockNumber % h2fmi_geometry.blocks_per_bank) ) & 0xFFFF) + page;
		uint32_t result = h2fmi_read_multi_ftl(ce, specialPage, (uint8_t*)(buff->data));
		if(result) {
			if(result != 1)
				i++;
			page++;
			continue;
		}
		if(strncmp(infoTypeName, (char*)(buff->data), nameSize))
			continue;

		if(!zero1)
			zero1 = specialBlockNumber;

		if(headerBuffer)
			memcpy(headerBuffer, buff->data, 0x38);

		if(!buff->data[13]) {
			BUF_Release(buff);
			if(zero2 == 1)
				update_specialBlockCache(ce, infoTypeName, specialBlockNumber);

			return 1;
		}

		uint32_t bytes_to_read = (bytesToRead >= buff->data[13]) ? buff->data[13] : bytesToRead;
		uint32_t bytes_read = (h2fmi_geometry.bbt_format << 10) - 0x38;

		if(bytes_to_read < (h2fmi_geometry.bbt_format << 10) - 0x38) {
			memcpy(dataBuffer, &(buff->data[14]), bytes_to_read);
			BUF_Release(buff);
			if(zero2 == 1)
				update_specialBlockCache(ce, infoTypeName, specialBlockNumber);

			return 1;
		}

		memcpy(dataBuffer, &(buff->data[14]), bytes_to_read);
		page++;

		for (bytes_read = (h2fmi_geometry.bbt_format << 10) - 0x38; bytes_read < bytes_to_read && page < h2fmi_geometry.pages_per_block;) {
			uint32_t specialPage = h2fmi_geometry.pages_per_block * ((h2fmi_geometry.unk14 * ((specialBlockNumber/h2fmi_geometry.blocks_per_bank) & 0xFFFF) + (specialBlockNumber % h2fmi_geometry.blocks_per_bank) ) & 0xFFFF) + page;
			result = h2fmi_read_multi_ftl(ce, specialPage, (uint8_t*)(buff->data));
			if(result)
				break;

			memcpy(dataBuffer+(bytes_read/sizeof(uint32_t)), buff->data, (bytesToRead-bytes_read >= (h2fmi_geometry.bbt_format<<10)) ? (h2fmi_geometry.bbt_format<<10) : (bytesToRead-bytes_read));
			bytes_read += (bytesToRead-bytes_read >= (h2fmi_geometry.bbt_format<<10)) ? (h2fmi_geometry.bbt_format<<10) : (bytesToRead-bytes_read);
		}

		if(bytes_read != bytes_to_read) {
			if(result != 1)
				i++;
			page++;
			continue;
		}

		BUF_Release(buff);
		if(zero2 == 1)
			update_specialBlockCache(ce, infoTypeName, specialBlockNumber);

		return 1;
	}

	BUF_Release(buff);

	return 0;
}

uint32_t get_scfg_info(uint32_t ce, uint32_t* headerBuffer, uint32_t* dataBuffer, uint32_t bytesToRead, char* infoTypeName, uint32_t nameSize, uint32_t zero1, uint32_t zero2) {
	uint32_t block;
	uint32_t done = 0;
	uint32_t specialBlockNumber;
	uint32_t* tempBuffer[16];
	uint32_t CE_cacheBlockNumber;
	memset(tempBuffer, 0, sizeof(tempBuffer));

	if(!infoTypeName)
		return 0;

	memcpy(tempBuffer, infoTypeName, 16);

	if(!zero1)
	{
		if(*pSpecialBlockCache)
		{
			if(ce <= h2fmi_geometry.num_ce)
			{
				CE_cacheBlockNumber = ce << 6;

				uint32_t i;
				for (i = 0; i < 5; i++) {
					if(!strncmp((char*)tempBuffer, (char*)(pSpecialBlockCache+CE_cacheBlockNumber+(i<<3)), 16))
						break;
				}

				if(i != 5)
				{
					uint32_t turns = 0;
					memcpy(tempBuffer, (char*)((*pSpecialBlockCache)+(ce<<8)), 32);
					if(!strncmp((char*)tempBuffer, "NANDDRIVERSIGN\0\0", 16))
						turns = 1;
					else
						turns = 2;

					for (i = 0; i < turns; i++) {
						if(*(pSpecialBlockCache+(5+2*i)) != 1)
							continue;

						if(!sub_5FF2508C(ce, *(pSpecialBlockCache+(4+2*turns)), headerBuffer, dataBuffer, bytesToRead, infoTypeName, nameSize, zero2, 0))
							continue;

						if(!*pSpecialBlockCache)
							continue;

						if(ce > h2fmi_geometry.num_ce)
							continue;

						uint32_t j;
						for(j = 0; j < 5; j++) {
							if(!strncmp(infoTypeName, (char*)((*pSpecialBlockCache)+CE_cacheBlockNumber+(j<<5)), 16)) {
								done = 1;
								break;
							}
						}

						if(!done)
							continue;

						if(i > 1)
							continue;

						uint32_t wtf = *((uint32_t*)((*pSpecialBlockCache)+CE_cacheBlockNumber+(((j<<2)+i)<<3)+20));
						if (wtf != 1)
							continue;

						*((uint32_t*)((*pSpecialBlockCache)+CE_cacheBlockNumber+(((j<<2)+i)<<3)+16)) = 0xFFFFFFFF;
						*((uint32_t*)((*pSpecialBlockCache)+CE_cacheBlockNumber+(((j<<2)+i)<<3)+20)) = 0;
					}
				}
			}
		}	

		block = ((uint32_t)((h2fmi_geometry.blocks_per_ce*0x2000)*0.96*0x2000)) & 0xFFFF;
		specialBlockNumber = h2fmi_geometry.blocks_per_ce;
	} else {
		block = 1;
		specialBlockNumber = 1;
	}

	done = 0;
	while(h2fmi_geometry.blocks_per_ce >= specialBlockNumber) {
		specialBlockNumber--;
		if(sub_5FF2508C(ce, specialBlockNumber, headerBuffer, dataBuffer, bytesToRead, infoTypeName, nameSize, zero2, 1)) {
			done = 1;
			break;
		}
	}
	if(!done)
		return 0;

	if(!strncmp((char*)tempBuffer, "DEVICEINFOBBT\0\0\0", 16))
		return 1;

	if(ce != 0)
		update_specialBlockCache(ce, "DEVICEINFOBBT\0\0\0", *(headerBuffer+6));
	else {
		update_specialBlockCache(ce, "DEVICEINFOBBT\0\0\0", *(headerBuffer+6));
		update_specialBlockCache(ce, "DEVICEUNIQUEINFO", *(headerBuffer+7));
		update_specialBlockCache(ce, "DEVICEUNIQUEINFO", *(headerBuffer+8));
		update_specialBlockCache(ce, "NANDDRIVERSIGN\0\0", *(headerBuffer+9));
		update_specialBlockCache(ce, "DIAGCONTROLINFO\0", *(headerBuffer+10));
		update_specialBlockCache(ce, "DIAGCONTROLINFO\0", *(headerBuffer+11));
		specialBlockCache_updated = 1;
	}

	return 1;
}

uint32_t specialBlockCacheInit(uint32_t _arg) {
	specialBlockCache_unkvar = 0xFA;
	if(!pSpecialBlockCache)
		pSpecialBlockCache = wmr_malloc(h2fmi_geometry.num_ce << 8);

	uint32_t ce;
	for (ce = 0; ce <= h2fmi_geometry.num_ce; ce++) {
		uint32_t i;
		for (i = 0; i <= 5; i++) {
			*(pSpecialBlockCache + (ce << 6) + (i << 3) + 5) = 0;
			*(pSpecialBlockCache + (ce << 6) + (i << 3) + 7) = 0;
			memset(pSpecialBlockCache + (ce << 6) + (i << 3), 0xFF, 4 * sizeof(uint32_t));
		}
	}

	return 0;
}

uint32_t FPart_Init(uint32_t signature_style, uint32_t fsys_start_block) {
	if(FPart_inited)
		return 0;

	uint32_t signature;
	if(signature_style & 0x200)
		signature = 2;
	else
		signature = 0;

	if(specialBlockCacheInit(((signature_style >> 8) & 1) | signature))
		return -1;

	return 0;
}

uint32_t BUF_Init(uint32_t bytes_per_page, uint32_t meta_per_logical_page, uint32_t ecc_bytes) {
	if(pBufCxt)
		return 0;

	dataSize = bytes_per_page;
	spareSize = meta_per_logical_page;

	pBufCxt = (BuffCxt*)wmr_malloc(0xC);

	pBufCxt->dwNumOfBuffers = ecc_bytes;
	pBufCxt->pBuffList = (BuffList*)wmr_malloc(pBufCxt->dwNumOfBuffers << 4);

	uint32_t i;
	for (i = 0; i < pBufCxt->dwNumOfBuffers; i++) {
		pBufCxt->pBuffList->pBuf[i].data = wmr_malloc(dataSize+spareSize);
		pBufCxt->pBuffList->pBuf[i].spare = pBufCxt->pBuffList->pBuf[i].data+(dataSize/sizeof(uint32_t));
		pBufCxt->pBuffList->pBuf[i].eStatus = BUF_FREE;
		pBufCxt->pBuffList->pBuf[i].buffer = pBufCxt->pBuffList->pBuf[i].data;
	}

	pBufCxt->nFreeBuffers = pBufCxt->dwNumOfBuffers;

	return 0;
}

Buff* BUF_Get(uint32_t eStatus) {
	if(!pBufCxt || !(pBufCxt->pBuffList) || !(pBufCxt->nFreeBuffers))
		return 0;

	uint32_t i;
	for (i = 0; i != pBufCxt->dwNumOfBuffers; i++) {
		if(pBufCxt->pBuffList->pBuf[i].eStatus != BUF_FREE)
			continue;
		if(eStatus == BUF_SPARE)
			pBufCxt->pBuffList->pBuf[i].data = NULL;
		else if(eStatus != BUF_ALLOCATED)
			pBufCxt->pBuffList->pBuf[i].spare = NULL;
		if(eStatus == BUF_ALLOCATED || eStatus == BUF_SPARE)
			memset(pBufCxt->pBuffList->pBuf[i].spare, 0xFF, spareSize);
		pBufCxt->pBuffList->pBuf[i].eStatus = BUF_ALLOCATED;
		pBufCxt->nFreeBuffers--;
		return &(pBufCxt->pBuffList->pBuf[i]);
	}

	return 0;
}

Buff* BUF_Release(Buff* _buff) {
	if(!_buff)
		return 0;

	uint32_t i;
	for (i = 0; i != pBufCxt->dwNumOfBuffers; i++) {
		if(&(pBufCxt->pBuffList->pBuf[i]) == _buff)
			break;
	}

	if(i == pBufCxt->dwNumOfBuffers)
		system_panic("vfl: wIdx > pBufCxt->dwNumOfBuffers\n");

	if(pBufCxt->pBuffList->pBuf[i].eStatus == BUF_FREE)
		return 0;

	if(!(pBufCxt->pBuffList->pBuf[i].eStatus == BUF_ALLOCATED))
		system_panic("vfl: pBuf->eStatus != BUF_ALLOCATED\n");

	pBufCxt->pBuffList->pBuf[i].data = pBufCxt->pBuffList->pBuf[i].buffer;
	pBufCxt->pBuffList->pBuf[i].spare = pBufCxt->pBuffList->pBuf[i].buffer + (dataSize/sizeof(uint32_t));
	pBufCxt->pBuffList->pBuf[i].eStatus = BUF_FREE;
	pBufCxt->nFreeBuffers++;

	if(pBufCxt->nFreeBuffers > pBufCxt->dwNumOfBuffers)
		system_panic("vfl: pBuf->nFreeBuffers > pBuf->dwNumOfBuffers\n");

	return &(pBufCxt->pBuffList->pBuf[i]);
}

uint8_t nand_get_epoch() {
	uint32_t result = GET_BITS(GET_REG(0xBF500000), 9, 7);
	if(!result)
		result = 1;

	result += 0x30;
	return (uint8_t)result;
}

uint32_t FTL_Setup(uint32_t _arg0, uint32_t _arg1, uint32_t signature_style, uint32_t fsys_start_block) {
	uint32_t* nand_driver_signature = malloc(0x108);
	if(!nand_driver_signature)
		return -1;

	uint32_t* FTL_Struct4 = malloc(0x108);
	if(!FTL_Struct4)
		return -1;

	memset(nand_driver_signature, 0, 0x108);
	memset(FTL_Struct4, 0, 0x108);

	if(wmr_inited)
		bufferPrintf("ftl: WMR_INIT already called\r\n");
	wmr_inited++;

	if(!h2fmi_geometry.num_ce) {
		bufferPrintf("ftl: No NAND attached!\r\n");
		return 0x80000001;
	}

	if(BUF_Init(h2fmi_geometry.bbt_format << 10, 0xC, 0xA))
		return 0x80000001;

	if(FPart_Init(signature_style, fsys_start_block))
		return 0x80000001;

	uint32_t style;
	if((signature_style >> 5) & 1)
		style = OldStyle;
	else
		style = NewStyle;

	if(style == OldStyle)
		VFL_ReadBlockZeroSign(nand_driver_signature, 0x108);
	else
		VFL_ReadBlockDriverSign(nand_driver_signature, 0x108);

	if(style == OldStyle)
		bufferPrintf("read old style signature 0x%08x\r\n", nand_driver_signature);
	else
		bufferPrintf("read new style signature 0x%08x\r\n", nand_driver_signature);

	if(*nand_driver_signature>>0x18 != 0x43 ||
			(GET_BITS(*nand_driver_signature, 16, 8) > 0x31) ||
			(GET_BITS(*nand_driver_signature, 8, 8) > 0x31) ||
			(GET_BITS(*nand_driver_signature, 0, 8) != 0x31) ||
			(GET_BITS(*(nand_driver_signature+1), 0, 8) > 6) ||
			nand_get_epoch() != GET_BITS(*nand_driver_signature, 0, 8)
			) {
		bufferPrintf("ftl: Incompatible signature.\r\n");
		return 0x80000002;
	}

/*	if(!(signature_style & 0x800) || (
			(*(nand_driver_signature+1) & 0x10000) &&
				!((signature_style >> 10) & 1) &&
				((!(*(nand_driver_signature+1) & 0x10000)) & ((signature_style >> 10) & 1))
			))*/
	if((signature_style & 0x800) && (
			!(*(nand_driver_signature+1) & 0x10000) ||
				((signature_style >> 10) & 1) ||
				!((!(*(nand_driver_signature+1) & 0x10000)) & ((signature_style >> 10) & 1))
			))
	{
		bufferPrintf("ftl: metadata whitening mismatch, please reformat.\r\n");
	}

	if(GET_BITS(*nand_driver_signature, 8, 8) == 0x31) {
		bufferPrintf("ftl: We're VSVFL!\r\n");
	} else if (GET_BITS(*nand_driver_signature, 8, 8) == 30) {
		bufferPrintf("ftl: We're VFL!\r\n");
	} else {
		bufferPrintf("ftl: No valid VFL Type found at signature 0x%08x\r\n", nand_driver_signature);
	}

// Not yet finished

	return 0;
}

void ftl_init() {
	uint32_t pointer1 = 0;
	uint32_t pointer2 = 0;
	FTL_Setup(pointer1, pointer2, 0, 1);
}
