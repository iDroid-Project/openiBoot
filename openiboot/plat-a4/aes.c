#include "aes.h"
#include "arm/arm.h"
#include "cdma.h"
#include "clock.h"
#include "hardware/dma.h"
#include "tasks.h"
#include "util.h"

// Placeholders

void aes_img2verify_encrypt(void* data, int size, const void* iv)
{
}

void aes_img2verify_decrypt(void* data, int size, const void* iv)
{
}

void aes_836_encrypt(void* data, int size, const void* iv)
{
}

void aes_836_decrypt(void* data, int size, const void* iv)
{
}

void aes_838_encrypt(void* data, int size, const void* iv)
{
}

void aes_838_decrypt(void* data, int size, const void* iv)
{
}

void aes_encrypt(void* data, int size, AESKeyType keyType, const void* key, const void* iv)
{
}

void aes_decrypt(void* data, int size, AESKeyType keyType, const void* key, const void* iv)
{
}

typedef struct AES_CRYPTO {
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
} AES_CRYPTO;

AES_CRYPTO aes_crypto[3]; // first one is not used. lol.

uint32_t aes_hw_crypto_operation(uint32_t _arg0, uint32_t _channel, uint32_t *_buffer, uint32_t _arg3, uint32_t _size, uint32_t _arg5, uint32_t _arg6, uint32_t _arg7) {
	uint32_t unkn0;
	uint32_t value = 0;
	uint32_t channel_reg = _channel << 12;
	SET_REG(DMA + channel_reg, 2);

	aes_crypto[_channel].unkn1 = (_arg7 ? 0x30103 : 0x103);
	aes_crypto[_channel].buffer = _buffer;
	aes_crypto[_channel].size = _size;
	aes_crypto[_channel].unkn9 = 0;

	if(_arg0) {
		switch(_arg5)
		{
			case 1:
				value |= (0 << 2);
				break;
			case 2:
				value |= (1 << 2);
				break;
			case 4:
				value |= (2 << 2);
				break;
			default:
				return -1;
		}
		if((_arg6 - 1) > 31)
			return -1;
		switch(_arg6)
		{
			case 2:
				value |= (0 << 4);
				break;
			case 3:
				value |= (1 << 4);
				break;
			case 5:
				value |= (2 << 4);
				break;
			case 9:
				value |= (3 << 4);
				break;
			case 17:
				value |= (4 << 4);
				break;
			case 33:
				value |= (5 << 4);
				break;
			default:
				return -1;
		}
		if(_arg0 == 1)
			value |= 2;
		unkn0 = 0;
		SET_REG(DMA + channel_reg + DMA_SETTINGS, value);
		SET_REG(DMA + channel_reg + DMA_TXRX_REGISTER, _arg3);
	} else {
		unkn0 = 128;
	}
	DataCacheOperation(1, (uint32_t)&aes_crypto[_channel], 0x20);
	SET_REG(DMA + channel_reg + DMA_SIZE, _size);
	SET_REG(DMA + channel_reg + 0x14, (uint32_t)&aes_crypto[_channel]);
	SET_REG(DMA + channel_reg, unkn0 | (_arg7 << 8) | 0x1C0000);
	return 0;
}

void wait_for_dma_channel(uint32_t channel) {
	uint32_t channel_reg = channel << 12;
	while(GET_BITS(GET_REG(DMA + channel_reg), 16, 2) == 1)
		task_yield();
}

uint32_t aes_hw_crypto_cmd(uint32_t _encrypt, uint32_t *_inBuf, uint32_t *_outBuf, uint32_t _size, uint32_t _type, uint32_t* _key, uint32_t *_iv) {
	uint32_t value = 0;

	clock_gate_switch(0x14, ON);
	dma_channel_activate(1, 1);
	dma_channel_activate(2, 1);

	uint32_t channel = 1;
	uint32_t channel_reg = channel << 12;
	value = (channel & 0xFF) << 8;

	if(_size & 0xF)
		system_panic("aes_hw_crypto_cmd: bad arguments\r\n");

	if(_encrypt & 0xF0) {
		if((_encrypt & 0xF0) != 0x10)
			system_panic("aes_hw_crypto_cmd: bad arguments\r\n");
		value |= 0x20000;
	}

	if(_encrypt & 0xF) {
		if((_encrypt & 0xF) != 1)
			system_panic("aes_hw_crypto_cmd: bad arguments\r\n");
		value |= 0x10000;
	}

	if(_iv) {
		SET_REG(0x87801010, _iv[0]);
		SET_REG(0x87801014, _iv[1]);
		SET_REG(0x87801018, _iv[2]);
		SET_REG(0x8780101C, _iv[3]);
	} else {
		SET_REG(0x87801010, 0);
		SET_REG(0x87801014, 0);
		SET_REG(0x87801018, 0);
		SET_REG(0x8780101C, 0);
	}

	switch(GET_BITS(_type, 28, 4))
	{
		case 2:	// AES 256
			value |= 0x80000;
			break;

		case 1:	// AES 192
			value |= 0x40000;
			break;

		case 0:	// AES 128
			break;

		default:// Fail
			system_panic("aes_hw_crypto_cmd: bad arguments\r\n");
	}

	uint32_t key_shift = 0;
	uint32_t key_set = 0;
	if ((_type & 0xFFF) == 0) {
		switch(GET_BITS(_type, 28, 4)) {
			case 2:				// AES 256
				SET_REG(DMA + DMA_AES + DMA_AES_KEY_7 + channel_reg, _key[7]);
				SET_REG(DMA + DMA_AES + DMA_AES_KEY_6 + channel_reg, _key[6]);
			case 1:				// AES 192
				SET_REG(DMA + DMA_AES + DMA_AES_KEY_5 + channel_reg, _key[5]);
				SET_REG(DMA + DMA_AES + DMA_AES_KEY_4 + channel_reg, _key[4]);
			case 0:				// AES 128
				SET_REG(DMA + DMA_AES + DMA_AES_KEY_3 + channel_reg, _key[3]);
				SET_REG(DMA + DMA_AES + DMA_AES_KEY_2 + channel_reg, _key[2]);
				SET_REG(DMA + DMA_AES + DMA_AES_KEY_1 + channel_reg, _key[1]);
				SET_REG(DMA + DMA_AES + DMA_AES_KEY_0 + channel_reg, _key[0]);
				value |= 0x100000;
				break;
			default:			// Fail
				system_panic("aes_hw_crypto_cmd: bad arguments\r\n");
		}
		key_set = 1;
	} else {
		if ((_type & 0xFFF) == 512) {
			key_shift = 1;
			value |= 0x200000;
		} else if ((_type & 0xFFF) == 513) {
			key_shift = 2;
			value |= 0x400000;
		} else {
			key_shift = 0;
		}
		// Key deactivated?
		if(GET_REG(DMA + DMA_AES) & (1 << key_shift))
			return -1;
	}

	SET_REG(DMA + DMA_AES + channel_reg, value);

	if(aes_hw_crypto_operation(0, 1, _inBuf, 0, _size, 0, 0, 1) || aes_hw_crypto_operation(0, 2, _outBuf, 0, _size, 0, 0, 0))
		system_panic("aes_hw_crypto_cmd: bad arguments\r\n");

	DataCacheOperation(1, (uint32_t)_inBuf, _size);
	DataCacheOperation(3, (uint32_t)_outBuf, _size);
	SET_REG(DMA + (1 << 12), GET_REG(DMA + (1 << 12)) | 1);
	SET_REG(DMA + (2 << 12), GET_REG(DMA + (2 << 12)) | 1);

	wait_for_dma_channel(1);
	wait_for_dma_channel(2);

	if(key_set)
	{
		SET_REG(DMA + DMA_AES + channel_reg + DMA_AES_KEY_0, 0);
		SET_REG(DMA + DMA_AES + channel_reg + DMA_AES_KEY_1, 0);
		SET_REG(DMA + DMA_AES + channel_reg + DMA_AES_KEY_2, 0);
		SET_REG(DMA + DMA_AES + channel_reg + DMA_AES_KEY_3, 0);
		SET_REG(DMA + DMA_AES + channel_reg + DMA_AES_KEY_4, 0);
		SET_REG(DMA + DMA_AES + channel_reg + DMA_AES_KEY_5, 0);
		SET_REG(DMA + DMA_AES + channel_reg + DMA_AES_KEY_6, 0);
		SET_REG(DMA + DMA_AES + channel_reg + DMA_AES_KEY_7, 0);
	}

	dma_channel_activate(1, 0);
	dma_channel_activate(2, 0);

	return 0;
}

uint32_t aes_crypto_cmd(uint32_t _encrypt, uint32_t *_inBuf, uint32_t *_outBuf, uint32_t _size, uint32_t _type, uint32_t *_key, uint32_t *_iv) {
	if(_size & 0xF || (!(_type & 0xFFF) && (_key == NULL)) || (_encrypt & 0xF) > 1)
		system_panic("aes_crypto_cmd: bad arguments\r\n");

	if(aes_hw_crypto_cmd(_encrypt, _inBuf, _outBuf, _size, _type, _key, _iv))
		return -1;

	return 0;
}
