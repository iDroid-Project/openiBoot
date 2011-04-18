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

int dma_setup();
signed int dma_init_channel(uint8_t direction, uint32_t channel, int segmentationSetting,
		uint32_t txrx_register, uint32_t size, uint32_t Setting1Index, uint32_t Setting2Index, void* handler);
void dma_continue_async(int channel);
int dma_set_aes(int channel, dmaAES* dmaAESInfo);
int dma_cancel(int channel);

#endif //_CDMA_H
