#ifndef  VFL_H
#define  VFL_H

#include "openiboot.h"
#include "nand.h"

#define ERROR_ARG	0x80000001
#define ERROR_NAND	0x80000002
#define ERROR_EMPTY	0x80000003

// VFL Device Prototypes
struct _vfl_device;

typedef int (*vfl_open_t)(struct _vfl_device *, nand_device_t *_dev);
typedef void (*vfl_close_t)(struct _vfl_device *);

typedef int (*vfl_read_t)(struct _vfl_device *, uint32_t _page, uint8_t *_buffer,
		uint8_t *_sparebuffer, int _empty_ok, int *_refresh, int _encrypted);

typedef int (*vfl_write_t)(struct _vfl_device *, uint32_t _page, uint8_t *_buffer,
		uint8_t *_sparebuffer, int _encrypted);

// VFL Device Struct
typedef struct _vfl_device
{
	vfl_open_t open;
	vfl_close_t close;

	vfl_read_t read;
	vfl_write_t write;
} vfl_device_t;

// VFL Device Functions
void vfl_init(vfl_device_t *_vfl);
void vfl_cleanup(vfl_device_t *_vfl);

vfl_device_t *vfl_allocate();

int vfl_open(vfl_device_t *_vfl, nand_device_t *_dev);
void vfl_close(vfl_device_t *_vfl);

int vfl_read(vfl_device_t *_vfl, uint32_t _page, uint8_t* _buffer, uint8_t* _spare,
		int _empty_ok, int* _refresh_page, int _encrypted);

int vfl_write(vfl_device_t *_vfl, uint32_t _page, uint8_t* _buffer, uint8_t* _spare,
		int _encrypted);

#endif //VFL_H
