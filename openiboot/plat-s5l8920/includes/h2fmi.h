#ifndef  H2FMI_H
#define  H2FMI_H

#include "openiboot.h"
#include "nand.h"
#include "cdma.h"

#define H2FMI_STATE_IDLE	0
#define H2FMI_STATE_READ	1
#define H2FMI_STATE_WRITE	2

#define H2FMI_READ_IDLE 0
#define H2FMI_READ_1	1
#define H2FMI_READ_2	2
#define H2FMI_READ_3	3
#define H2FMI_READ_4	4
#define H2FMI_READ_DONE 5

typedef struct _h2fmi_geometry
{
	uint32_t field_0;
	uint16_t num_ce;
	uint16_t blocks_per_ce;
	uint16_t pages_per_block;
	uint16_t bytes_per_page;
	uint16_t bbt_format;
	uint16_t bytes_per_spare;
	uint16_t banks_per_ce_vfl;
	uint16_t banks_per_ce;
	uint16_t blocks_per_bank;
	uint16_t bank_address_space;
	uint16_t pages_per_block_2;
	uint16_t total_block_space;
	uint16_t unk1A;
	uint32_t unk1C;
	uint32_t vendor_type;
	uint8_t ecc_bits;
	uint8_t ecc_tag;

	uint32_t num_fmi;
	uint32_t blocks_per_bank_32;
	uint32_t banks_per_ce_32;
	uint32_t pages_per_block_32;
	uint32_t pages_per_block_2_32;

	uint32_t page_number_bit_width;
	uint32_t page_number_bit_width_2;
	uint32_t is_ppn;
	uint32_t num_ecc_bytes;
	uint32_t meta_per_logical_page;
	uint32_t field_60;
	uint32_t pages_per_ce;

} h2fmi_geometry_t;

typedef struct _h2fmi_failure_details
{
	uint32_t overall_status;
} h2fmi_failure_details_t;

typedef struct _h2fmi_state
{
	uint32_t state;
	uint32_t read_state;
} h2fmi_state_t;

typedef struct _h2fmi_struct
{
	uint32_t bus_num;
	uint32_t base_address;
	uint32_t clock_gate;

	uint32_t bitmap;
	uint32_t num_chips;

	uint32_t dma0;
	uint32_t dma1;
	
	uint32_t field_8;
	uint32_t field_C;
	uint32_t is_ppn; // 10
	uint32_t pages_per_block; // 14
	uint32_t bbt_format; // 18
	uint32_t bytes_per_spare; // 1C
	uint32_t blocks_per_ce; // 20
	uint32_t banks_per_ce_vfl; // 24
	uint32_t meta_per_logical_page; // 28
	uint32_t ecc_bytes; // 2C
	uint32_t bytes_per_page; // 30
	uint32_t banks_per_ce; // 34
	uint32_t timing_register_cache_408; // 38
	uint32_t page_size; // 3C
	uint8_t ecc_bits; // 44
	int8_t ecc_tag; // 45
	uint32_t field_48; // 48
	dmaAES aes_struct;
	dmaAES *aes_info; // 64
	uint8_t *aes_iv_pointer; // 68
	h2fmi_state_t state; // 6C
	uint32_t current_page_index; // 78
	uint32_t num_pages_to_read;
	uint32_t field_80;
	uint32_t field_100;
	uint16_t *chips;
	uint32_t *pages;
	uint8_t **data_ptr; // 114
	uint8_t **wmr_ptr; // 118
	uint64_t field_124;
	uint64_t field_12C;
	uint64_t field_134;
	uint32_t field_13C;
	uint32_t field_140;
	uint16_t current_chip; // 144
	uint8_t *field_148;
	uint32_t field_14C;
	uint32_t field_150;
	uint32_t field_154;
	uint8_t *field_158;
	uint32_t field_15C;
	h2fmi_failure_details_t failure_details; // 160
	uint32_t field_170;
	uint32_t field_180;
	uint8_t field_182;
	
} h2fmi_struct_t;

void h2fmi_setup_ftl(uint32_t _start_page, uint32_t _smth, uint32_t _dataBuf, uint32_t _count);
void h2fmi_clear_ftl();

uint32_t h2fmi_read_single_page(uint32_t _ce, uint32_t _page, uint8_t *_ptr, uint8_t *_meta_ptr, uint8_t *_6, uint8_t *_7, uint32_t _8);

#endif //H2FMI_H
