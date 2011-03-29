#ifndef  VFL_VFL_H
#define  VFL_VFL_H

#include "../../includes/vfl.h"
#include "nand.h"

typedef struct _vfl_vfl_geometry
{
	uint16_t pages_per_block;
	uint16_t pages_per_sublk;
	uint32_t pages_total;
	uint32_t some_page_mask;
	uint32_t some_sublk_mask;
	uint16_t blocks_per_ce;
	uint16_t bytes_per_page;
	uint32_t num_ecc_bytes;
	uint32_t meta_per_logical_page;
	uint32_t one;
	uint16_t num_ce;
	uint16_t ecc_bits;
	uint16_t field_24;
	uint16_t field_26;
	uint16_t some_crazy_val;
	uint16_t field_2A;
} vfl_vfl_geometry_t;

struct _vfl_vfl_context;

// VFL-VFL Device Struct
typedef struct _vfl_vfl_device
{
	vfl_device_t vfl;

	uint32_t current_version;
	struct _vfl_vfl_context *contexts;
	nand_device_t *device;
	vfl_vfl_geometry_t geometry;
	uint8_t *bbt;

	uint32_t *pageBuffer;
	uint16_t *chipBuffer;
} vfl_vfl_device_t;

// VFL-VFL Functions
int vfl_vfl_device_init(vfl_vfl_device_t *_vfl);
void vfl_vfl_device_cleanup(vfl_vfl_device_t *_vfl);

vfl_vfl_device_t *vfl_vfl_device_allocate();

#endif //VFL_VFL_H
