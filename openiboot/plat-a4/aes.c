#include "aes.h"
#include "arm/arm.h"
#include "cdma.h"
#include "clock.h"
#include "hardware/dma.h"
#include "tasks.h"
#include "util.h"
#include "commands.h"

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
		keyType = 512;
	}
	else if(strcmp(argv[2], "uid") == 0)
	{
		keyType = 513;
	}
	else
	{
		hexToBytes(argv[6], &key, (int*)&keyLength);
		switch(keyLength*(sizeof(uint32_t)))
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
				return;
		}
	}

	hexToBytes(argv[3], &data, (int*)&dataLength);
	uint8_t* outBuf = malloc(dataLength);
	memset(outBuf, 0, dataLength);

	if(argc > 6)
	{
		hexToBytes(argv[4], &iv, (int*)&ivLength);
	}

	if(strcmp(argv[1], "enc") == 0)
	{
		aes_crypto_cmd(1, data, outBuf, dataLength, keyType, key, iv);
		bytesToHex(outBuf, dataLength);
		bufferPrintf("\r\n");
	}
	else if(strcmp(argv[1], "dec") == 0)
	{
		aes_crypto_cmd(0, data, outBuf, dataLength, keyType, key, iv);
		bytesToHex(outBuf, dataLength);
		bufferPrintf("\r\n");
	}
	else
	{
		bufferPrintf("Usage: %s <enc/dec> <gid/uid/key> [data] [iv]\r\n", argv[0]);
	}

	if(outBuf)
		free(outBuf);
}
COMMAND("aes", "use the hardware crypto engine", cmd_aes);
