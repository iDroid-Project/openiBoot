#ifndef  VFL_H
#define  VFL_H

#include "openiboot.h"

#define ERROR_ARG	0x80000001
#define ERROR_NAND	0x80000002
#define ERROR_EMPTY	0x80000003

typedef struct _vfl_context
{
	uint32_t usn_inc; // 0x000
	uint16_t control_block[3]; // 0x004
	uint8_t unk1[2]; // 0x00A
	uint32_t usn_dec; // 0x00C
	uint16_t active_context_block; // 0x010
	uint16_t next_context_page; // 0x012
	uint8_t unk2[2]; // 0x014
	uint16_t field_16; // 0x016
	uint16_t field_18; // 0x018
	uint16_t num_reserved_blocks; // 0x01A
	uint16_t reserved_block_pool_start; // 0x01C
	uint16_t total_reserved_blocks; // 0x01E
	uint16_t reserved_block_pool_map[820]; // 0x020
	uint8_t bad_block_table[282];			// 0x688
	uint16_t vfl_context_block[4];			// 0x7A2
	uint16_t remapping_schedule_start;		// 0x7AA
	uint8_t unk3[0x4C];				// 0x7AC
	uint32_t checksum1;				// 0x7F8
	uint32_t checksum2;				// 0x7FC
} vfl_context_t;

typedef struct _vfl_spare_data
{
	union
	{
		struct
		{
			uint32_t logicalPageNumber;
			uint32_t usn;
		} __attribute__ ((packed)) user;

		struct
		{
			uint32_t usnDec;
			uint16_t idx;
			uint8_t field_6;
			uint8_t field_7;
		} __attribute__ ((packed)) meta;
	};

	uint8_t type2;
	uint8_t type1;
	uint8_t eccMark;
	uint8_t field_B;

} __attribute__ ((packed)) vfl_spare_data_t;

#endif //VFL_H
