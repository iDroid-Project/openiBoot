#include "openiboot.h"
#include "hfs/common.h"
#include "hfs/bdev.h"
#include "util.h"

typedef struct _bdevio_device
{
	io_func io;
	block_device_handle_t handle;
} bdevio_device_t;

#define bdevio_device_get(x) (CONTAINER_OF(bdevio_device_t, io, (x)))


static int bdevio_read(io_func *_io, off_t _addr, size_t _sz, void *_dest)
{
	bdevio_device_t *bdev = bdevio_device_get(_io);
	int ret = block_device_seek(bdev->handle, seek_begin, _addr);
	if(ret < 0)
		return ret;

	return block_device_read(bdev->handle, _dest, _sz);
}

static int bdevio_write(io_func *_io, off_t _addr, size_t _sz, void *_src)
{
	bdevio_device_t *bdev = bdevio_device_get(_io);
	int ret = block_device_seek(bdev->handle, seek_begin, _addr);
	if(ret < 0)
		return ret;

	return block_device_write(bdev->handle, _src, _sz);
}

void bdevio_close(io_func *_io)
{
	bdevio_device_t *bdev = bdevio_device_get(_io);
	free(bdev);
}

io_func *bdevio_open(block_device_handle_t _handle)
{
	bdevio_device_t *ret = malloc(sizeof(bdevio_device_t));
	memset(ret, 0, sizeof(bdevio_device_t));
	
	ret->handle = _handle;
	ret->io.read = bdevio_read;
	ret->io.write = bdevio_write;
	ret->io.close = bdevio_close;

	return &ret->io;
}

bdevfs_device_t *bdevfs_open(int _devIdx, int _pIdx)
{
	block_device_t *dev = NULL;
	while(_devIdx >= 0 && (dev = block_device_find(dev)))
		_devIdx--;

	if(!dev)
		return NULL;

	block_device_handle_t handle = block_device_open(dev, _pIdx);
	if(!handle)
		return NULL;

	bdevfs_device_t *ret = malloc(sizeof(bdevfs_device_t));
	ret->handle = handle;
	ret->io = bdevio_open(handle);
	ret->volume = openVolume(ret->io);
	return ret;
}

void bdevfs_close(bdevfs_device_t *bdev)
{
	closeVolume(bdev->volume);
	bdevio_close(bdev->io);
	block_device_close(bdev->handle);
	free(bdev);
};
