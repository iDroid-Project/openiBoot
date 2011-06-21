#ifndef HFSCPROTECT_H
#define HFSCPROTECT_H

typedef struct HFSPlusCprotectV2
{
	uint16_t xattr_version; //=2 (version?)
	uint16_t zero ; //=0
	uint32_t unknown; // leaks stack dword in one code path :)
	uint32_t protection_class_id;
	uint32_t wrapped_length; //0x28
	uint8_t wrapped_key[0x28];
} __attribute__ ((packed)) HFSPlusCprotectV2;

typedef struct HFSPlusCprotectV4
{
	uint16_t xattr_version; //=2 (version?)
	uint16_t zero ; //=0
	uint32_t xxx_length; // 0xc
	uint32_t protection_class_id;
	uint32_t wrapped_length; //0x28
	uint8_t xxx_junk[20]; //uninitialized ?
	uint8_t wrapped_key[0x28];
} __attribute__ ((packed)) HFSPlusCprotectV4;

#endif
