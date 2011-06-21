#ifndef HFSCPROTECT_H
#define HFSCPROTECT_H

typedef struct HFSPlusCprotect
{
	uint16_t xattr_version; //=2 (version?)
	uint16_t zero ; //=0
	uint32_t unknown; //leaks stack dword in one code path :)
	uint32_t protection_class_id;
	uint32_t wrapped_length ; // 40 bytes (32+8 bytes from aes wrap integrity)
	uint8_t wrapped_key[1] ; // wrappedlength
} __attribute__ ((packed)) HFSPlusCprotect;


#endif
