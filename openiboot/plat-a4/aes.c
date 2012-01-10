#include "aes.h"
#include "arm/arm.h"
#include "cdma.h"
#include "clock.h"
#include "hardware/dma.h"
#include "tasks.h"
#include "util.h"
#include "commands.h"

static const uint8_t Gen835[] = {0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01};
static const uint8_t Gen89B[] = {0x18, 0x3E, 0x99, 0x67, 0x6B, 0xB0, 0x3C, 0x54, 0x6F, 0xA4, 0x68, 0xF5, 0x1C, 0x0C, 0xBD, 0x49};
static const uint8_t Gen836[] = {0x00, 0xE5, 0xA0, 0xE6, 0x52, 0x6F, 0xAE, 0x66, 0xC5, 0xC1, 0xC6, 0xD4, 0xF1, 0x6D, 0x61, 0x80};
static const uint8_t Gen838[] = {0x8C, 0x83, 0x18, 0xA2, 0x7D, 0x7F, 0x03, 0x07, 0x17, 0xD2, 0xB8, 0xFC, 0x55, 0x14, 0xF8, 0xE1};

static uint8_t Key835[16];
static uint8_t Key89B[16];
static uint8_t Key836[16];
static uint8_t Key838[16];

void doAES(int enc, void* data, int size, AESKeyType keyType, const void* key, AESKeyLen keyLen, const void* iv);

int aes_setup() {
	memcpy(Key835, Gen835, 16);
	aes_encrypt(Key835, 16, AESUID, NULL, 0, NULL);

	memcpy(Key89B, Gen89B, 16);
	aes_encrypt(Key89B, 16, AESUID, NULL, 0, NULL);

	memcpy(Key836, Gen836, 16);
	aes_encrypt(Key836, 16, AESUID, NULL, 0, NULL);

	memcpy(Key838, Gen838, 16);
	aes_encrypt(Key838, 16, AESUID, NULL, 0, NULL);

	return 0;
}

void aes_img2verify_encrypt(void* data, int size, const void* iv)
{
}

void aes_img2verify_decrypt(void* data, int size, const void* iv)
{
}

void aes_835_encrypt(void* data, int size, const void* iv)
{
	aes_encrypt(data, size, AESCustom, Key835, AES128, iv);
}

void aes_835_decrypt(void* data, int size, const void* iv)
{
	aes_encrypt(data, size, AESCustom, Key835, AES128, iv);
}

void aes_835_unwrap_key(void* outBuf, void* inBuf, int size, const void* iv) {
	aes_unwrap_key(Key835, AES128, iv, outBuf, inBuf, size);
}

void aes_89B_encrypt(void* data, int size, const void* iv)
{
	aes_encrypt(data, size, AESCustom, Key89B, AES128, iv);
}

void aes_89B_decrypt(void* data, int size, const void* iv)
{
	aes_decrypt(data, size, AESCustom, Key89B, AES128, iv);
}

void aes_836_encrypt(void* data, int size, const void* iv)
{
	aes_encrypt(data, size, AESCustom, Key836, AES128, iv);
}

void aes_836_decrypt(void* data, int size, const void* iv)
{
	aes_decrypt(data, size, AESCustom, Key836, AES128, iv);
}

void aes_838_encrypt(void* data, int size, const void* iv)
{
	aes_encrypt(data, size, AESCustom, Key838, AES128, iv);
}

void aes_838_decrypt(void* data, int size, const void* iv)
{
	aes_decrypt(data, size, AESCustom, Key838, AES128, iv);
}

void aes_encrypt(void* data, int size, AESKeyType keyType, const void* key, AESKeyLen keyLen, const void* iv)
{
	doAES(0x10, data, size, keyType, key, keyLen, iv);
}

void aes_decrypt(void* data, int size, AESKeyType keyType, const void* key, AESKeyLen keyLen, const void* iv)
{
	doAES(0x11, data, size, keyType, key, keyLen, iv);
}

void doAES(int enc, void* data, int size, AESKeyType _keyType, const void* key, AESKeyLen keyLen, const void* iv)
{
	uint32_t keyType = 0;
	uint8_t* buff = NULL;

	switch(_keyType)
	{
		case AESCustom:
			switch(keyLen)
			{
				case AES128:
					keyType = 0 << 28;
					break;
				case AES192:
					keyType = 1 << 28;
					break;
				case AES256:
					keyType = 2 << 28;
					break;
				default:
					return;
			}
			break;
		case AESGID:
			keyType = 512 | (2 << 28);
			break;
		case AESUID:
			keyType = 513 | (2 << 28);
			break;
		default:
			return;
	}

	buff = memalign(DMA_ALIGN, size);

	if (!buff) {
		bufferPrintf("out of memory.\r\n");
		goto return_free;
	}

	memcpy(buff, data, size);
	aes_crypto_cmd(enc, buff, buff, size, keyType, (void*)key, (void*)iv);
	memcpy(data, buff, size);

return_free:
	if(buff)
		free(buff);
}

static error_t cmd_aes(int argc, char** argv)
{
	uint8_t* key = NULL;
	uint8_t* iv = NULL;
	uint8_t* data = NULL;
	uint8_t* buff = NULL;

	uint32_t keyType;
	uint32_t keyLength;
	uint32_t ivLength;
	uint32_t dataLength;

	if(argc < 4) {
		bufferPrintf("Usage: %s <enc/dec> <gid/uid/key> [data] [iv]\r\n", argv[0]);
		return EINVAL;
	}

	if(strcmp(argv[2], "gid") == 0)
	{
		keyType = 512;
	}
	else if(strcmp(argv[2], "uid") == 0)
	{
		keyType = 513;
	}
	else
	{
		hexToBytes(argv[2], &key, (int*)&keyLength);
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
				bufferPrintf("Usage: %s <enc/dec> <gid/uid/key> [data] [iv]\r\n", argv[0]);
				goto return_free;
		}
	}

	hexToBytes(argv[3], &data, (int*)&dataLength);
	buff = memalign(DMA_ALIGN, dataLength);

	if (!buff) {
		bufferPrintf("out of memory.\r\n");
		goto return_free;
	}

	memcpy(buff, data, dataLength);
	free(data);
	data = NULL;

	if(argc > 4)
	{
		hexToBytes(argv[4], &iv, (int*)&ivLength);
	}

	if(strcmp(argv[1], "enc") == 0)
		aes_crypto_cmd(0x10, buff, buff, dataLength, keyType, key, iv);
	else if(strcmp(argv[1], "dec") == 0)
		aes_crypto_cmd(0x11, buff, buff, dataLength, keyType, key, iv);
	else
	{
		bufferPrintf("Usage: %s <enc/dec> <gid/uid/key> [data] [iv]\r\n", argv[0]);
		goto return_free;
	}

	bytesToHex(buff, dataLength);
	bufferPrintf("\r\n");

return_free:
	if (data)
		free(data);

	if (iv)
		free(iv);

	if (key)
		free(key);

	if (buff)
		free(buff);

	return SUCCESS;
}
COMMAND("aes", "use the hardware crypto engine", cmd_aes);
