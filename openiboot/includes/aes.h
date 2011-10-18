/**
 * aes.h
 *
 * Copyright 2011 iDroid Project
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

/**
 *	@file aes.h
 *
 *	@brief Header file for the AES function OiB performs.
 */
#ifndef AES_H
#define AES_H

#include "openiboot.h"

#define AES_128_CBC_IV_SIZE 16
#define AES_128_CBC_BLOCK_SIZE 64

typedef enum AESKeyType {
	AESCustom = 0,
	AESGID = 1,
	AESUID = 2
} AESKeyType;

typedef enum AESKeyLen {
	AES128 = 0,
	AES192 = 1,
	AES256 = 2
} AESKeyLen;

int aes_setup();
void aes_835_encrypt(void* data, int size, const void* iv);
void aes_835_decrypt(void* data, int size, const void* iv);
void aes_835_unwrap_key(void* outBuf, void* inBuf, int size, const void* iv);
void aes_836_encrypt(void* data, int size, const void* iv);
void aes_836_decrypt(void* data, int size, const void* iv);
void aes_838_encrypt(void* data, int size, const void* iv);
void aes_838_decrypt(void* data, int size, const void* iv);
void aes_89B_encrypt(void* data, int size, const void* iv);
void aes_89B_decrypt(void* data, int size, const void* iv);
void aes_img2verify_encrypt(void* data, int size, const void* iv);
void aes_img2verify_decrypt(void* data, int size, const void* iv);

void aes_encrypt(void* data, int size, AESKeyType keyType, const void* key, AESKeyLen keyLen, const void* iv);
void aes_decrypt(void* data, int size, AESKeyType keyType, const void* key, AESKeyLen keyLen, const void* iv);


// aes_wrap.c
int aes_wrap_key(const unsigned char *key, AESKeyLen keyLen, const unsigned char *iv,
		unsigned char *out,
		const unsigned char *in, unsigned int inlen);
int aes_unwrap_key(const unsigned char *key, AESKeyLen keyLen, const unsigned char *iv,
		unsigned char *out,
		const unsigned char *in, unsigned int inlen);


#endif

