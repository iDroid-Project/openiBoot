#include "openiboot.h"
#include "arm/arm.h"
#include "util.h"
#include "malloc.h"
#include "hardware/dma.h"
#include "cdma.h"
#include "clock.h"
#include "timer.h"
#include "interrupt.h"
#include "mmu.h"
#include "tasks.h"
#include "commands.h"

typedef struct segmentBuffer {
	uint32_t address;
	uint32_t value;
	uint32_t offset;
	uint32_t length;
	uint32_t iv[4];
} segmentBuffer;

typedef struct DMAInfo {
	uint32_t signalled;
	uint32_t txrx_register;
	uint32_t unk_separator;
	segmentBuffer* segmentBuffer;
	uint32_t dmaSegmentNumber;
	uint32_t unsegmentedSize;
	DMASegmentInfo* segmentsInfo;
	uint32_t segmentOffset;
	uint32_t previousSegmentOffset;
	uint32_t previousDmaSegmentNumber;
	uint32_t previousUnsegmentedSize;
	void (*handler)(int channel);
	int channel;
	uint32_t dataSize;
	dmaAES* dmaAESInfo;
	uint32_t dmaAES_channel;
	uint32_t current_segment;
	uint32_t irq_state;
} DMAInfo;

typedef struct DMATxRxInfo {
	uint8_t unkn0;
	uint8_t unkn1;
	uint8_t unkn2;
	uint8_t unkn3;
	uint32_t unkn4;
} DMATxRxInfo;

static const DMATxRxInfo DMATxRx[8] = {
	{3, 4, 0, 0, 0x80000020},
	{3, 4, 0, 1, 0x800000A0},
	{3, 4, 0, 1, 0x800000A4},
	{5, 8, 1, 0, 0x81000020},
	{5, 8, 1, 1, 0x81200014},
	{5, 8, 1, 2, 0x81200018},
	{5, 8, 1, 3, 0x81300014},
	{5, 8, 1, 4, 0x81300018},
};

static DMAInfo dmaInfo[32];

static uint32_t dmaAES_channel_used = 0;

static void dmaIRQHandler(uint32_t token);
void dma_continue_async(int channel);

int dma_setup() {
	clock_gate_switch(0x25, ON);

	return 0;
}

int dma_channel_activate(int channel, uint32_t activate) {
	return 0;
}

signed int dma_init_channel(uint8_t direction, uint32_t channel, DMASegmentInfo* segmentsInfo, uint32_t txrx_register, uint32_t size, uint32_t Setting1Index, uint32_t Setting2Index, void* handler) {
	int i = 0;
	DMAInfo* dma = &dmaInfo[channel];

	if (!dma->signalled) {
		dma->segmentBuffer = memalign(0x20, 32 * sizeof(*dma->segmentBuffer));
		memset(dma->segmentBuffer, 0, (32 * sizeof(*dma->segmentBuffer)));

		//bufferPrintf("cdma: new segment buffer 0x%08x.\r\n", dma->segmentBuffer);

		if (!dma->segmentBuffer)
			system_panic("CDMA: can't allocate command chain\r\n");

		for (i = 0; i != 32; i ++)
			dma->segmentBuffer[i].address = get_physical_address((uint32_t)(&dma->segmentBuffer[i+1]));

		dma->signalled = 1;
		dma->txrx_register = 0;
		dma->unk_separator = 0;

		interrupt_set_int_type(DMA_CHANNEL_INTERRUPT_BASE + channel, 0);
		interrupt_install(DMA_CHANNEL_INTERRUPT_BASE + channel, dmaIRQHandler, channel);
		interrupt_enable(DMA_CHANNEL_INTERRUPT_BASE + channel);
	}

	dma->irq_state = 0;
	dma->dmaSegmentNumber = 0;
	dma->segmentsInfo = segmentsInfo;
	dma->segmentOffset = 0;
	dma->dataSize = size;
	dma->unsegmentedSize = size;
	dma->handler = handler;
	dma->channel = channel;

	uint8_t Setting1;
	uint8_t Setting2;

	switch(Setting1Index)
	{
	case 1:
		Setting1 = 0 << 2;
		break;

	case 2:
		Setting1 = 1 << 2;
		break;

	case 4:
		Setting1 = 2 << 2;
		break;

	default:
		return -1;
	}

	switch (Setting2Index)
	{
	case 1:
		Setting2 = 0 << 4;
		break;

	case 2:
		Setting2 = 1 << 4;
		break;

	case 4:
		Setting2 = 2 << 4;
		break;

	case 8:
		Setting2 = 3 << 4;
		break;

	case 16:
		Setting2 = 4 << 4;
		break;

	case 32:
		Setting2 = 5 << 4;
		break;

	default:
		return -1;
	}

	uint32_t channel_reg = channel << 12;
	SET_REG(DMA + channel_reg, 2);

	uint8_t direction_setting;
	if (direction == 1) // if out
		direction_setting = 1 << 1;
	else
		direction_setting = 0 << 1;

	SET_REG(DMA + DMA_SETTINGS + channel_reg, dma->unk_separator | Setting1 | Setting2 | direction_setting);
	SET_REG(DMA + DMA_TXRX_REGISTER + channel_reg, txrx_register);
	SET_REG(DMA + DMA_SIZE + channel_reg, size);

	if (dma->dmaAESInfo)
		dma->current_segment = 0;

	dma_continue_async(channel);
	return 0;
}

void dma_continue_async(int channel) {

	//bufferPrintf("cdma: continue_async.\r\n");

	uint8_t segmentId;
	uint32_t segmentLength;
	uint32_t value;
	DMAInfo* dma = &dmaInfo[channel];

	if (!dma->unsegmentedSize)
		system_panic("CDMA: ASSERT FAILED\r\n");

	dma->previousUnsegmentedSize = dma->unsegmentedSize;
	dma->previousDmaSegmentNumber = dma->dmaSegmentNumber;
	dma->previousSegmentOffset = dma->segmentOffset;
	if (dma->dmaAESInfo)
	{
		for (segmentId = 0; segmentId < 28;) {
			if (!dma->unsegmentedSize)
				break;

			dma->segmentBuffer[segmentId].value = 2;
			dma->dmaAESInfo->ivGenerator(dma->dmaAESInfo->ivParameter, dma->current_segment, dma->segmentBuffer[segmentId].iv);
			segmentId++;
			dma->current_segment++;
			segmentLength = 0;

			int encryptedSegmentOffset;
			int encryptedSegmentOffsetEnd = dma->dmaAESInfo->dataSize;

			for (encryptedSegmentOffset = 0; encryptedSegmentOffset < encryptedSegmentOffsetEnd; encryptedSegmentOffset += segmentLength) {
				if (encryptedSegmentOffset >= encryptedSegmentOffsetEnd)
					break;

				segmentLength = dma->segmentsInfo[dma->dmaSegmentNumber].size - dma->segmentOffset;
				if (encryptedSegmentOffset + segmentLength > encryptedSegmentOffsetEnd)
					segmentLength = encryptedSegmentOffsetEnd - encryptedSegmentOffset;

				value = 0x10003;
				if (!encryptedSegmentOffset)
					value = 0x30003;

				dma->segmentBuffer[segmentId].value = value;
				dma->segmentBuffer[segmentId].offset = dma->segmentsInfo[dma->dmaSegmentNumber].ptr + dma->segmentOffset;
				dma->segmentBuffer[segmentId].length = segmentLength;

				if (!segmentLength)
					system_panic("Caught trying to generate zero-length cdma segment on channel %d, irqState: %d\r\n", channel, dma->irq_state);

				dma->segmentOffset += segmentLength;

				if (dma->segmentOffset >= dma->segmentsInfo[dma->dmaSegmentNumber].size)
				{
					++dma->dmaSegmentNumber;
					dma->segmentOffset = 0;
				}

				++segmentId;
			}

			dma->unsegmentedSize -= encryptedSegmentOffsetEnd;
		}

		if (!dma->unsegmentedSize)
			dma->segmentBuffer[segmentId-1].value |= 0x100;

		dma->segmentBuffer[segmentId].value = 0;
	} else {
		for (segmentId = 0; segmentId < 31; segmentId++) {
			int segmentLength = dma->segmentsInfo[dma->dmaSegmentNumber].size - dma->segmentOffset;

			dma->segmentBuffer[segmentId].value = 3;
			dma->segmentBuffer[segmentId].offset = dma->segmentsInfo[dma->dmaSegmentNumber].ptr + dma->segmentOffset;
			dma->segmentBuffer[segmentId].length = segmentLength;

			if (!segmentLength)
				system_panic("Caught trying to generate zero-length cdma segment on channel %d, irqState: %d\r\n", channel, dma->irq_state);

			dma->segmentOffset = 0;

			if (segmentLength >= dma->unsegmentedSize) {
				dma->segmentBuffer[segmentId].value |= 0x100;
				dma->unsegmentedSize = 0;
				break;
			}

			dma->unsegmentedSize -= segmentLength;
			dma->dmaSegmentNumber++;
		}

		if (segmentId == 31)
			dma->segmentBuffer[31].value = 0;
		else
			dma->segmentBuffer[segmentId+1].value = 0;
	}

	DataCacheOperation(1, (uint32_t)dma->segmentBuffer, 32 * (segmentId + 2));

	uint32_t channel_reg = channel << 12;
	SET_REG(DMA + channel_reg + 0x14, get_physical_address((uint32_t)dma->segmentBuffer));

	value = 0x1C0009;

	if (dma->dmaAESInfo)
		value |= (uint8_t)(dma->dmaAES_channel) << 8;

	//bufferPrintf("cdma: continue async 0x%08x.\r\n", value);
	SET_REG(DMA + channel_reg, value);
}

int dma_set_aes(int channel, dmaAES* dmaAESInfo) {
	//bufferPrintf("cdma: set_aes.\r\n");

	DMAInfo* dma = &dmaInfo[channel];
	uint32_t value;
	int i;

	dma->dmaAESInfo = dmaAESInfo;
	if(!dmaAESInfo)
		return 0;

	if (!dma->dmaAES_channel)
	{
		EnterCriticalSection();
		for (i = 2; i < 9; i++) {
			if (dmaAES_channel_used & (1 << i))
				continue;

			dmaAES_channel_used |= (1 << i);
			dma->dmaAES_channel = i;
			break;
		}
		LeaveCriticalSection();

		if (!dma->dmaAES_channel)
			system_panic("CDMA: no AES filter contexts: 0x%08x\r\n", dmaAES_channel_used);
	}

	uint32_t dmaAES_channel_reg = dma->dmaAES_channel << 12;

	value = (channel & 0xFF) << 8;

	if (dma->dmaAESInfo->inverse & 0xF)
		value |= 0x20000;
	else
		value |= 0x30000;

	switch(GET_BITS(dma->dmaAESInfo->type, 28, 4))
	{
		case 2: // AES 256
			value |= 0x80000;
			break;

		case 1: // AES 192
			value |= 0x40000;
			break;

		case 0: // AES 128
			break;

		default: // Fail
			return -1;
	}

	uint32_t someval = dma->dmaAESInfo->type & 0xFFF;
	if(someval == 0x200)
	{
		value |= 0x200000;
	}
	else if(someval == 0x201)
	{
		value |= 0x400000;
	}
	else if(someval == 0)
	{
		switch(GET_BITS(dma->dmaAESInfo->type, 28, 4))
		{
		case 2: // AES-256
			SET_REG(DMA + DMA_AES + DMA_AES_KEY_7 + dmaAES_channel_reg, dma->dmaAESInfo->key[7]);
			SET_REG(DMA + DMA_AES + DMA_AES_KEY_6 + dmaAES_channel_reg, dma->dmaAESInfo->key[6]);

		case 1: // AES-192
			SET_REG(DMA + DMA_AES + DMA_AES_KEY_5 + dmaAES_channel_reg, dma->dmaAESInfo->key[5]);
			SET_REG(DMA + DMA_AES + DMA_AES_KEY_4 + dmaAES_channel_reg, dma->dmaAESInfo->key[4]);

		case 0: // AES-128
			SET_REG(DMA + DMA_AES + DMA_AES_KEY_3 + dmaAES_channel_reg, dma->dmaAESInfo->key[3]);
			SET_REG(DMA + DMA_AES + DMA_AES_KEY_2 + dmaAES_channel_reg, dma->dmaAESInfo->key[2]);
			SET_REG(DMA + DMA_AES + DMA_AES_KEY_1 + dmaAES_channel_reg, dma->dmaAESInfo->key[1]);
			SET_REG(DMA + DMA_AES + DMA_AES_KEY_0 + dmaAES_channel_reg, dma->dmaAESInfo->key[0]);
			value |= 0x100000;
			break;

		default:
			return -1;
		}
	}
	else if(someval != 0x100)
		return -1;

	SET_REG(DMA + dmaAES_channel_reg + DMA_AES, value);
	return 0;
}

int dma_cancel(int channel) {

	//bufferPrintf("cdma: dma_cancel.\r\n");

	DMAInfo* dma = &dmaInfo[channel];
	uint64_t startTime = timer_get_system_microtime();

	if (!dma->signalled)
		return channel;

	dma_channel_activate(channel, 1);

	uint32_t channel_reg = DMA + (channel << 12);
	if (GET_BITS(GET_REG(channel_reg), 16, 2) == 1) {
		SET_REG(channel_reg, 4);

		while (GET_BITS(GET_REG(channel_reg), 16, 2) == 1) {
			if (has_elapsed(startTime, 10000))
				system_panic("CDMA: channel %d timeout during abort\r\n", channel);
		}

		SET_REG(channel_reg, 2);
	}

	dma->signalled = 1;
	dma_set_aes(channel, 0);

	return dma_channel_activate(channel, 0);
}

void dmaIRQHandler(uint32_t token) {
	uint32_t wholeSegment;
	uint32_t currentSegment;
	uint8_t segmentId;
	int channel = token;
	uint32_t channel_reg = channel << 12;
	DMAInfo* dma = &dmaInfo[channel];

	GET_REG(DMA + channel_reg);
	uint32_t status = GET_REG(DMA + channel_reg);
	//bufferPrintf("cdma: intsts 0x%08x\r\n", status);

	if (status & 0x40000)
		system_panic("CDMA: channel %d error interrupt\r\n", channel);

	if (status & 0x100000)
		system_panic("CDMA: channel %d spurious CIR\r\n", channel);

	SET_REG(DMA + channel_reg, 0x80000);

	if (dma->unsegmentedSize || GET_REG(DMA + DMA_SIZE + channel_reg)) {
		if (GET_REG(DMA + channel_reg) & 0x30000) {
			GET_REG(DMA + channel_reg);
			dma->unsegmentedSize = GET_REG(DMA + DMA_SIZE + channel_reg) + dma->unsegmentedSize;
			wholeSegment = dma->previousUnsegmentedSize - dma->unsegmentedSize;

			for (segmentId = 0; segmentId < 32; segmentId++) {
				currentSegment = dma->segmentsInfo[dma->previousDmaSegmentNumber + segmentId].size - dma->previousSegmentOffset;
				if (wholeSegment <= currentSegment)
					break;
				wholeSegment -= currentSegment;
				dma->previousSegmentOffset = 0;
			}

			dma->dmaSegmentNumber = segmentId + dma->previousDmaSegmentNumber;
			dma->segmentOffset = dma->previousSegmentOffset + wholeSegment;
			dma->irq_state = 1;
		} else {
			dma->irq_state = 2;
		}
		dma_continue_async(channel);
	} else {
		dma->signalled = 1;
		
		//bufferPrintf("cmda: done\r\n");

		dma_set_aes(channel, 0);

		dma_channel_activate(channel, 0);

		if (dma->handler)
			dma->handler(dma->channel);
	}
}
static dmaAES_CRYPTO *aes_crypto = NULL; // first one is not used. lol.

uint32_t aes_hw_crypto_operation(uint32_t _arg0, uint32_t _channel, uint32_t *_buffer, uint32_t _arg3, uint32_t _size, uint32_t _arg5, uint32_t _arg6, uint32_t _arg7) {
	uint32_t unkn0;
	uint32_t value = 0;
	uint32_t channel_reg = _channel << 12;

	SET_REG(DMA + channel_reg, 2);

	aes_crypto[_channel].buffer = _buffer;
	aes_crypto[_channel].unkn0 = &aes_crypto[_channel].unkn8;
	aes_crypto[_channel].unkn1 = (_arg7 ? 0x30103 : 0x103);
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

		switch(_arg6)
		{
			case 1:
				value |= (0 << 4);
				break;
			case 2:
				value |= (1 << 4);
				break;
			case 4:
				value |= (2 << 4);
				break;
			case 8:
				value |= (3 << 4);
				break;
			case 16:
				value |= (4 << 4);
				break;
			case 32:
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
	SET_REG(DMA + channel_reg, (unkn0 | (_arg7 << 8) | 0x1C0000));
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
		value |= (1 << 17);
	}

	if(_encrypt & 0xF) {
		if((_encrypt & 0xF) != 1)
			system_panic("aes_hw_crypto_cmd: bad arguments\r\n");
	} else {
		value |= (1 << 16);
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

	uint32_t key_shift = 0;
	uint32_t key_set = 0;
	if ((_type & 0xFFF) == 0) {
		switch(GET_BITS(_type, 28, 4))
		{
			case 2:	// AES 256
				value |= (2 << 18);
				break;

			case 1:	// AES 192
				value |= (1 << 18);
				break;

			case 0:	// AES 128
				value |= (0 << 18);
				break;

			default:// Fail
				system_panic("aes_hw_crypto_cmd: bad arguments\r\n");
		}
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
				break;
			default:			// Fail
				system_panic("aes_hw_crypto_cmd: bad arguments\r\n");
		}
		key_set = 1;
	} else {
		if ((_type & 0xFFF) == 512) {
			key_shift = 1; // still broken
		} else if ((_type & 0xFFF) == 513) {
			key_shift = 0;
		} else {
			key_shift = 2; // wrong I guess but never used
		}
		// Key deactivated?
		if(GET_REG(DMA + DMA_AES) & (1 << key_shift))
			return -1;
	}

	if(key_shift)
		value |= 1 << 19;

	value |= (key_set << 20) | (key_shift << 21);
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

uint32_t aes_crypto_cmd(uint32_t _encrypt, void *_inBuf, void *_outBuf, uint32_t _size, uint32_t _type, void *_key, void *_iv) {
	if(_size & 0xF || (!(_type & 0xFFF) && (_key == NULL)) || (_encrypt & 0xF) > 1) {
		bufferPrintf("aes_crypto_cmd: bad arguments\r\n");
		return -1;
	}

	aes_crypto = memalign(DMA_ALIGN, sizeof(dmaAES_CRYPTO) * 3);
	if (!aes_crypto) {
		bufferPrintf("aes_crypto_cmd: out of memory\r\n");
		return -1;
	}

	if(aes_hw_crypto_cmd(_encrypt, (uint32_t*)_inBuf, (uint32_t*)_outBuf, _size, _type, (uint32_t*)_key, (uint32_t*)_iv)) {
		free(aes_crypto);
		return -1;
	}

	free(aes_crypto);
	return 0;
}

static int cmd_cdma_aes(int argc, char** argv)
{
	uint8_t* key = NULL;
	uint32_t keyLength;
	uint32_t keyType;
	uint8_t* iv = NULL;
	uint32_t ivLength;

	if(argc < 6) {
		bufferPrintf("Usage: %s [enc/dec] [inBuf] [outBuf] [size] [gid/uid/key] [iv]\r\n", argv[0]);
		return -1;
	}

	uint32_t *inBuf = (uint32_t*)parseNumber(argv[2]);
	uint32_t *outBuf = (uint32_t*)parseNumber(argv[3]);
	uint32_t size = parseNumber(argv[4]);

	if(strcmp(argv[5], "gid") == 0)
	{
		keyType = 512 | (2 << 28);
	}
	else if(strcmp(argv[5], "uid") == 0)
	{
		keyType = 513 | (2 << 28);
	}
	else
	{
		hexToBytes(argv[5], &key, (int*)&keyLength);
		switch(keyLength*8)
		{
			case 128:
				keyType = 0 << 28;
				break;
			case 192:
				keyType = 1 << 28;
				break;
			case 256:
				keyType = 2 << 28;
				break;
			default:
				bufferPrintf("Usage: %s [enc/dec] [inBuf] [outBuf] [size] [gid/uid/key] [iv]\r\n", argv[0]);
				goto return_free;
		}
	}

	if(argc > 6)
	{
		hexToBytes(argv[6], &iv, (int*)&ivLength);
	}

	if(strcmp(argv[1], "enc") == 0)
	{
		aes_crypto_cmd(0x10, inBuf, outBuf, size, keyType, key, iv);
	}
	else if(strcmp(argv[1], "dec") == 0)
	{
		aes_crypto_cmd(0x11, inBuf, outBuf, size, keyType, key, iv);
	}
	else
	{
		bufferPrintf("Usage: %s [enc/dec] [inBuf] [outBuf] [size] [gid/uid/key] [iv]\r\n", argv[0]);
	}

return_free:
	if (key)
		free(key);

	if (iv)
		free(iv);

	return 0;
}
COMMAND("cdma_aes", "use hw crypto on a buffer", cmd_cdma_aes);
