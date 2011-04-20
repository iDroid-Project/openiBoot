#ifndef  MTD_H
#define  MTD_H

#include "device.h"
#include "bdev.h"

struct _mtd;
typedef error_t (*mtd_prepare_t)(struct _mtd *);
typedef void (*mtd_finish_t)(struct _mtd *);
typedef error_t (*mtd_read_t)(struct _mtd *, void *_dest, uint32_t _off, int _sz);
typedef error_t (*mtd_write_t)(struct _mtd *, void *_src, uint32_t _off, int _sz);

typedef int (*mtd_get_attribute_t)(struct _mtd *);

typedef enum _mtd_use
{
	mtd_boot_images,
	mtd_filesystem,
	mtd_other,

} mtd_usage_t;

typedef struct _mtd
{
	device_t device;
	block_device_t bdev;
	uint32_t bdev_addr;

	LinkedList list_ptr;

	mtd_prepare_t prepare;
	mtd_finish_t finish;
	mtd_read_t read;
	mtd_write_t write;

	mtd_get_attribute_t size;
	mtd_get_attribute_t block_size;

	mtd_usage_t usage;
	int prepare_count;
} mtd_t;

// Used by drivers
error_t mtd_init(mtd_t *_mtd);
void mtd_cleanup(mtd_t *_mtd);

error_t mtd_register(mtd_t *_mtd);
void mtd_unregister(mtd_t *_mtd);

// Used by clients
mtd_t *mtd_find(mtd_t *_prev);

error_t mtd_prepare(mtd_t *_mtd);
void mtd_finish(mtd_t *_mtd);

int mtd_size(mtd_t *_mtd);
int mtd_block_size(mtd_t *_mtd);

error_t mtd_read(mtd_t *_mtd, void *_dest, uint32_t _off, int _sz);
error_t mtd_write(mtd_t *_mtd, void *_src, uint32_t _off, int _sz);

void mtd_list_devices();

#endif //MTD_H
