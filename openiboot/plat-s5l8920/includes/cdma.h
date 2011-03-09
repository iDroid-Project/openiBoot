#ifndef  _CDMA_H
#define  _CDMA_H

typedef struct dmaAES {
	uint32_t unkn0;
	uint32_t AESType;
	uint32_t *key;
	uint32_t dataSize;
	void (*handler)(uint32_t dataBuffer, uint32_t dmaAES_setting2, uint32_t* unknAESSetting1);
	uint32_t dataBuffer;
} dmaAES;

int dma_setup();
signed int dma_init_channel(uint8_t direction, uint32_t channel, int segmentationSetting,
		uint32_t txrx_register, uint32_t size, uint32_t Setting1Index, uint32_t Setting2Index, void* handler);
void dma_continue_async(int channel);
int dma_set_aes(int channel, dmaAES* dmaAESInfo);
int dma_cancel(int channel);

#endif //_CDMA_H
