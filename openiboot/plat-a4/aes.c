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

static void cmd_aes(int argc, char** argv)
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
		return;
	}

	if(strcmp(argv[2], "gid") == 0)
	{
		keyType = 512 | (2 << 28);
	}
	else if(strcmp(argv[2], "uid") == 0)
	{
		keyType = 513 | (2 << 28);
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
}
COMMAND("aes", "use the hardware crypto engine", cmd_aes);
