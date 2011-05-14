#ifndef  _CDMA_H
#define  _CDMA_H

typedef struct dmaAES {
	uint32_t inverse;
	uint32_t type;
	uint32_t *key;
	uint32_t dataSize;
	void (*ivGenerator)(uint32_t _param, uint32_t _segment, uint32_t* _iv);
	uint32_t ivParameter;
} dmaAES;

typedef struct DMASegmentInfo {
	uint32_t ptr;
	uint32_t size;
} DMASegmentInfo;

typedef struct dmaAES_CRYPTO {
	uint32_t unkn0;
	uint32_t unkn1;
	uint32_t *buffer;
	uint32_t size;
	uint32_t unkn4;
	uint32_t unkn5;
	uint32_t unkn6;
	uint32_t unkn7;
	uint32_t unkn8;
	uint32_t unkn9;
	uint32_t unkn10;
	uint32_t unkn11;
	uint32_t unkn12;
	uint32_t unkn13;
	uint32_t unkn14;
	uint32_t unkn15;
} dmaAES_CRYPTO;

int dma_setup();
signed int dma_init_channel(uint8_t direction, uint32_t channel, DMASegmentInfo* segmentsInfo,
		uint32_t txrx_register, uint32_t size, uint32_t Setting1Index, uint32_t Setting2Index, void* handler);
void dma_continue_async(int channel);
int dma_set_aes(int channel, dmaAES* dmaAESInfo);
int dma_cancel(int channel);
uint32_t aes_crypto_cmd(uint32_t _encrypt, void *_inBuf, void *_outBuf, uint32_t _size, uint32_t _type, void *_key, void *_iv);

#endif //_CDMA_H
