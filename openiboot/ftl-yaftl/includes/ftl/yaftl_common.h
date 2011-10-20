#ifndef FTL_YAFTL_COMMON_H
#define FTL_YAFTL_COMMON_H

#include "openiboot.h"
#include "util.h"

// Page types (as defined in the spare data "type" bitfield)
#define PAGETYPE_INDEX		(0x4)	// Index block indicator
#define PAGETYPE_CLOSED		(0x8)	// Closed (full) block
#define PAGETYPE_LBN		(0x10)	// User data (also called lbn: maybe logical block number? lBlock 0 is system and lBlock 1 is user?)
#define PAGETYPE_FTL_CLEAN	(0x20)	// FTL context (unmounted, clean)
// 0x40: ?
#define PAGETYPE_VFL		(0x80)	// VFL context

typedef struct {
	uint32_t lpn;			// Logical page number
	uint32_t usn;			// Update sequence number
	uint8_t  field_8;
	uint8_t  type;			// Page type
	uint16_t field_A;
} __attribute__((packed)) SpareData;

#endif // FTL_YAFTL_COMMON_H
