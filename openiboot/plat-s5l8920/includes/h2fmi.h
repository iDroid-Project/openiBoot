#ifndef  H2FMI_H
#define  H2FMI_H

#include "openiboot.h"

typedef struct _h2fmi_failure_details
{
	uint32_t overall_status;
} h2fmi_failure_details_t;

#define H2FMI_STATE_IDLE 0
#define H2FMI_STATE_READ 1

#define H2FMI_READ_IDLE 0
#define H2FMI_READ_1	1
#define H2FMI_READ_2	2
#define H2FMI_READ_3	3
#define H2FMI_READ_4	4
#define H2FMI_READ_DONE 5

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
	uint32_t field_10;
	uint32_t field_14;
	uint32_t field_18;
	uint32_t some_ecc_field; // 1C
	uint32_t field_20;
	uint32_t field_24;
	uint32_t field_28;
	uint32_t some_ecc_field2; // 2C
	uint32_t some_ecc_field3; // 30
	uint32_t field_34;
	uint32_t timing_register_cache_408; // 38
	uint32_t page_size; // 3C
	uint8_t ecc_bits; // 44
	int8_t ecc_tag; // 45
	h2fmi_state_t state; // 6C
	uint32_t field_78;
	uint32_t field_7C;
	uint32_t field_80;
	uint16_t *field_10C;
	uint32_t *field_110;
	uint32_t field_114;
	uint32_t field_118;
	uint64_t field_124;
	uint64_t field_12C;
	uint64_t field_134;
	uint32_t field_13C;
	uint32_t field_140;
	uint16_t field_144;
	uint32_t field_148;
	uint32_t field_14C;
	uint32_t field_150;
	uint32_t field_154;
	uint32_t field_158;
	uint32_t field_15C;
	h2fmi_failure_details_t failure_details; // 160
	uint32_t field_170;
	uint32_t field_180;
	uint8_t field_182;
	
} h2fmi_struct_t;

#endif //H2FMI_H
