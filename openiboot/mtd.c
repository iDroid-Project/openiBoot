#include "mtd.h"
#include "arm/arm.h"
#include "commands.h"
#include "util.h"

static int mtd_init_done = 0;
static LinkedList mtd_list;

#define mtd_get(ptr)		(CONTAINER_OF(mtd_t, list_ptr, (ptr)))
#define mtd_get_bdev(ptr) 	(CONTAINER_OF(mtd_t, bdev, (ptr)))

static error_t mtd_bdev_prepare(block_device_t *_dev)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	return mtd_prepare(dev);
}

static void mtd_bdev_finish(block_device_t *_dev)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	mtd_finish(dev);
}

static error_t mtd_bdev_seek(block_device_t *_dev, seek_mode_t _mode, int64_t _amt)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	switch(_mode)
	{
	case seek_begin:
		dev->bdev_addr = (int)_amt;
		break;

	case seek_end:
		{
			int sz = mtd_size(dev);
			if(sz <= 0)
				return EIO;
			
			dev->bdev_addr = sz - (int)_amt;
			break;
		}

	case seek_offset:
		dev->bdev_addr += (int)_amt;
		break;
	}

	return SUCCESS;
}

static error_t mtd_bdev_read(block_device_t *_dev, void *_dest, int _sz)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	int ret = mtd_read(dev, _dest, dev->bdev_addr, _sz);
	if(ret >= 0)
		dev->bdev_addr += _sz;

	return ret;
}

static error_t mtd_bdev_write(block_device_t *_dev, void *_src, int _sz)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	int ret = mtd_write(dev, _src, dev->bdev_addr, _sz);
	if(ret >= 0)
		dev->bdev_addr += _sz;

	return ret;
}

static int mtd_bdev_size(block_device_t *_dev)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	return mtd_size(dev);
}

static int mtd_bdev_block_size(block_device_t *_dev)
{
	mtd_t *dev = mtd_get_bdev(_dev);
	return mtd_block_size(dev);
}

error_t mtd_init(mtd_t *_mtd)
{
	error_t ret = device_init(&_mtd->device);
	if(ret != 0)
		return ret;

	ret = block_device_init(&_mtd->bdev);
	if(ret != 0)
		return ret;

	_mtd->bdev.prepare = mtd_bdev_prepare;
	_mtd->bdev.finish = mtd_bdev_finish;

	_mtd->bdev.read = mtd_bdev_read;
	_mtd->bdev.write = mtd_bdev_write;
	_mtd->bdev.seek = mtd_bdev_seek;

	_mtd->bdev.size = mtd_bdev_size;
	_mtd->bdev.block_size = mtd_bdev_block_size;

	_mtd->list_ptr.next = NULL;
	_mtd->list_ptr.prev = NULL;
	_mtd->prepare_count = 0;
	_mtd->bdev_addr = 0;

	return SUCCESS;
}

void mtd_cleanup(mtd_t *_mtd)
{
}

error_t mtd_register(mtd_t *_mtd)
{
	error_t ret = device_register(&_mtd->device);
	if(FAILED(ret))
		return ret;

	EnterCriticalSection();

	if(!mtd_init_done)
	{
		mtd_init_done = 1;
		mtd_list.prev = &mtd_list;
		mtd_list.next = &mtd_list;
	}

	LinkedList *prev = mtd_list.prev;
	_mtd->list_ptr.prev = prev;
	_mtd->list_ptr.next = &mtd_list;
	prev->next = &_mtd->list_ptr;
	mtd_list.prev = &_mtd->list_ptr;
	LeaveCriticalSection();

	bufferPrintf("mtd: New device, '%s', registered.\n", _mtd->device.name);

	if(_mtd->usage == mtd_filesystem)
	{
		ret = block_device_register(&_mtd->bdev);
		if(FAILED(ret))
			return ret;
	}

	return SUCCESS;
}

void mtd_unregister(mtd_t *_mtd)
{
	if(_mtd->usage == mtd_filesystem)
		block_device_unregister(&_mtd->bdev);

	if(_mtd->list_ptr.next != NULL && _mtd->list_ptr.prev != NULL)
	{
		EnterCriticalSection();
		LinkedList *prev = _mtd->list_ptr.prev;
		LinkedList *next = _mtd->list_ptr.next;
		next->prev = prev;
		prev->next = next;
		_mtd->list_ptr.prev = NULL;
		_mtd->list_ptr.next = NULL;
		LeaveCriticalSection();
	}

	device_unregister(&_mtd->device);
}

mtd_t *mtd_find(mtd_t *_prev)
{
	LinkedList *ptr;
	if(_prev == NULL)
		ptr = mtd_list.next;
	else
		ptr = _prev->list_ptr.next;

	if(ptr == &mtd_list)
		return NULL;

	return mtd_get(ptr);
}

error_t mtd_prepare(mtd_t *_mtd)
{
	int ret = 0;
	if(_mtd->prepare_count == 0 && _mtd->prepare != NULL)
		ret = _mtd->prepare(_mtd);

	_mtd->prepare_count++;
	return SUCCESS;
}

void mtd_finish(mtd_t *_mtd)
{
	if(_mtd->prepare_count == 1 && _mtd->finish != NULL)
		_mtd->finish(_mtd);

	_mtd->prepare_count--;
	if(_mtd->prepare_count < 0)
		_mtd->prepare_count = 0;
}

int mtd_size(mtd_t *_mtd)
{
	if(_mtd->size)
		return _mtd->size(_mtd);

	return -1;
}

int mtd_block_size(mtd_t *_mtd)
{
	if(_mtd->block_size)
		return _mtd->block_size(_mtd);
	
	return -1;
}

error_t mtd_read(mtd_t *_mtd, void *_dest, uint32_t _off, int _sz)
{
	if(_mtd->read == NULL)
		return ENOENT;

	return _mtd->read(_mtd, _dest, _off, _sz);
}

error_t mtd_write(mtd_t *_mtd, void *_src, uint32_t _off, int _sz)
{
	if(_mtd->write == NULL)
		return ENOENT;

	return _mtd->write(_mtd, _src, _off, _sz);
}

void mtd_list_devices()
{
	mtd_t *mtd = NULL;
	int i = 0;

	bufferPrintf("mtd: Listing Devices:\n");
	while((mtd = mtd_find(mtd)))
	{
		bufferPrintf("mtd:    Device #%d '%s'.\n", i, mtd->device.name);
		i++;
	}
}

void cmd_mtd_list(int argc, char **argv)
{
	mtd_list_devices();
}
COMMAND("mtd_list", "List all MTD devices.", cmd_mtd_list);

void cmd_mtd_read(int argc, char **argv)
{
	if(argc != 5)
	{
		bufferPrintf("Usage: %s [device] [destination] [offset] [size].\n");
		return;
	}
	
	int idx = parseNumber(argv[1]);
	mtd_t *dev = mtd_find(NULL);
	while(idx > 0 && dev != NULL)
		dev = mtd_find(dev);

	if(!dev)
	{
		bufferPrintf("Invalid MTD index.\n");
		return;
	}

	void *dest = (void*)parseNumber(argv[2]);
	uint32_t offset = parseNumber(argv[3]);
	int len = parseNumber(argv[4]);

	mtd_prepare(dev);
	mtd_read(dev, dest, offset, len);
	mtd_finish(dev);
}
COMMAND("mtd_read", "Read from a MTD device.", cmd_mtd_read);

void cmd_mtd_write(int argc, char **argv)
{
	if(argc != 5)
	{
		bufferPrintf("Usage: %s [device] [source] [offset] [size].\n");
		return;
	}
	
	int idx = parseNumber(argv[1]);
	mtd_t *dev = mtd_find(NULL);
	while(idx > 0 && dev != NULL)
		dev = mtd_find(dev);

	if(!dev)
	{
		bufferPrintf("Invalid MTD index.\n");
		return;
	}

	void *src = (void*)parseNumber(argv[2]);
	uint32_t offset = parseNumber(argv[3]);
	int len = parseNumber(argv[4]);

	mtd_prepare(dev);
	mtd_write(dev, src, offset, len);
	mtd_finish(dev);
}
COMMAND("mtd_write", "Write to a MTD device.", cmd_mtd_write);
