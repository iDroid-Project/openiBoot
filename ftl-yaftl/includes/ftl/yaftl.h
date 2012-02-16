#ifndef FTL_YAFTL_H
#define FTL_YAFTL_H

#include "ftl.h"
#include "vfl.h"
#include "nand.h"
#include "openiboot.h"
#include "yaftl_common.h"

typedef struct _ftl_yaftl_device {
	ftl_device_t ftl;
} ftl_yaftl_device_t;

ftl_yaftl_device_t *ftl_yaftl_device_allocate();

#endif // FTL_YAFTL_H
