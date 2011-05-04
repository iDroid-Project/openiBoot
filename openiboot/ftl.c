#include "vfl.h"
#include "ftl.h"

// FTL types
#ifdef CONFIG_FTL_YAFTL
#include "ftl/yaftl.h"
#endif

error_t ftl_init(ftl_device_t *_dev)
{
	error_t ret = mtd_init(&_dev->mtd);
	if(FAILED(ret))
		return ret;

	return SUCCESS;
}

void ftl_cleanup(ftl_device_t *_dev)
{
	mtd_cleanup(&_dev->mtd);
}

error_t ftl_register(ftl_device_t *_dev)
{
	error_t ret = mtd_register(&_dev->mtd);
	if(FAILED(ret))
		return ret;

	return SUCCESS;
}

void ftl_unregister(ftl_device_t *_dev)
{
	mtd_unregister(&_dev->mtd);
}

error_t ftl_open(ftl_device_t *_dev, vfl_device_t *_vfl)
{
	if(_dev->open)
		return _dev->open(_dev, _vfl);

	return ENOENT;
}

void ftl_close(ftl_device_t *_dev)
{
	if(_dev->close)
		_dev->close(_dev);
}

error_t ftl_read_single_page(ftl_device_t *_dev, uint32_t _page, uint8_t *_buffer)
{
	if(_dev->read_single_page)
		return _dev->read_single_page(_dev, _page, _buffer);

	return ENOENT;
}

error_t ftl_write_single_page(ftl_device_t *_dev, uint32_t _page, uint8_t *_buffer)
{
	if(_dev->write_single_page)
		return _dev->write_single_page(_dev, _page, _buffer);

	return ENOENT;
}

error_t ftl_detect(ftl_device_t **_dev, vfl_device_t *_vfl)
{
	// TODO: Implement FTL detection! -- Ricky26
	ftl_yaftl_device_t *yaftl = ftl_yaftl_device_allocate();
	ftl_init(&yaftl->ftl);

	*_dev = &yaftl->ftl;

	ftl_open(*_dev, _vfl);
	return SUCCESS;
}

