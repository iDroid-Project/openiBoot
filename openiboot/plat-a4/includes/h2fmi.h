#ifndef  H2FMI_H
#define  H2FMI_H

#include "openiboot.h"

typedef struct _h2fmi_struct
{
	uint32_t bus_num;
	uint32_t base_address;
	uint32_t clock_gate;
	uint32_t interrupt;

	uint32_t bitmap;
	uint32_t num_chips;
	uint32_t nand_timings; // field_38

	uint32_t field_48;
	uint32_t field_7C;
	uint32_t field_1A0;
	
} h2fmi_struct_t;

#endif //H2FMI_H
