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

