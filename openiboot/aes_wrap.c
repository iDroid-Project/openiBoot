/*
 * aes_wrap.c
 *
 * Copyright 2010 iDroid Project
 *
 * This file is part of iDroid. An android distribution for Apple products.
 * For more information, please visit http://www.idroidproject.org/.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

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

