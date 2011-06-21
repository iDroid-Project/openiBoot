#include "util.h"
#include "aes.h"

static const unsigned char default_iv[] = { 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, 0xA6, };

int aes_wrap_key(const uint8_t *_key, AESKeyLen _keyLen, const uint8_t *_iv,
				uint8_t *_out, const uint8_t *_in, uint32_t _inLen)
{
	int j, i;
	uint8_t A[16];

	if(!_iv)
		_iv = default_iv;
	*(long long *)A = *(long long *)_iv;

	memcpy(_out + 8, _in, _inLen);

	for(j = 0; j <= 5; j++)
	{
		uint32_t xor = (_inLen*j)/8;
		uint8_t *t = (uint8_t*)&xor;

		for(i = 1; i <= _inLen/8; i++)
		{
			xor++;

			long long *R = (long long*)(_out + i*8);

			memcpy(A+8, R, 8);

			aes_encrypt(A, sizeof(A), AESCustom, _key, _keyLen, NULL);

			A[4] ^= t[3];
			A[5] ^= t[2];
			A[6] ^= t[1];
			A[7] ^= t[0];

			memcpy(R, A+8, 8);
		}
	}

	memcpy(_out, A, 8);
	return _inLen + 8;
}

int aes_unwrap_key(const uint8_t *_key, AESKeyLen _keyLen, const uint8_t *_iv,
				uint8_t *_out, const uint8_t *_in, uint32_t _inLen)
{
	int j, i;
	uint8_t A[16];

	_inLen -= 8;
	memcpy(A, _in, 8);
	memcpy(_out, _in + 8, _inLen);

	for(j = 0; j <= 5; j++)
	{
		uint32_t xor = (6 - j) * (_inLen/8);
		uint8_t *t = (uint8_t*)&xor;

		for(i = 1; i <= _inLen/8; i++)
		{
			if(xor > 0xFF) {
				A[4] ^= t[3];
				A[5] ^= t[2];
				A[6] ^= t[1];
			}
			A[7] ^= t[0];

			long long *R = (long long*)(_out + _inLen - (i*8));

			memcpy(A+8, R, 8);

			aes_decrypt(A, sizeof(A), AESCustom, _key, _keyLen, NULL);

			memcpy(R, A+8, 8);
			
			xor--;
		}
	}

	if(!_iv)
		_iv = default_iv;
	if(memcmp(_out, _iv, 8) != 0)
		return 0; // If IV doesn't match result, we failed!

	return _inLen;
}

