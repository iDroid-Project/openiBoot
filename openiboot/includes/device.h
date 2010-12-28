#ifndef  DEVICE_H
#define  DEVICE_H

#include "openiboot.h"

struct _device;
typedef int (*ioctl_t)(struct _device *, uint32_t _id, void *_in, size_t _in_amt, void *_out, size_t _out_amt);

typedef struct _device
{
	const fourcc_t fourcc;
	const char *name;

	ioctl_t ioctl;

	LinkedList list_ptr;
	LinkedList children;

	void *data;
} device_t;

// Used by drivers
int device_init(device_t *_device);

int device_register(device_t *_device);
void device_unregister(device_t *_device);

// Used by clients
device_t *device_find(device_t *_where, fourcc_t _fcc, device_t *_last);

int device_ioctl(device_t *_dev, uint32_t _id, void *_in, size_t _in_amt, void *_out, size_t _out_amt);

#endif //DEVICE_H
