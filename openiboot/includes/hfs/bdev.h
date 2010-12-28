#ifndef  HFS_BDEV_H
#define  HFS_BDEV_H

#include "openiboot.h"
#include "../bdev.h"
#include "hfs/common.h"
#include "hfs/hfsplus.h"

typedef struct _bdevfs_device
{
	block_device_handle_t handle;
	io_func *io;
	Volume *volume;
} bdevfs_device_t;

void bdevio_close(io_func *_io);
io_func *bdevio_open(block_device_handle_t _handle);

bdevfs_device_t *bdevfs_open(int _devIdx, int _pIdx);
void bdevfs_close(bdevfs_device_t *bdev);

#endif //HFS_BDEV_H
