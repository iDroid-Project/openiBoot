#include "nand.h"
#include "util.h"

void nand_device_init(nand_device_t *_nand)
{
	memset(_nand, 0, sizeof(*_nand));
}

void nand_device_cleanup(nand_device_t *_nand)
{
}

nand_device_t *nand_device_allocate()
{
	nand_device_t *ret = malloc(sizeof(*ret));
	nand_device_init(ret);
	return ret;
}

nand_geometry_t *nand_device_get_geometry(nand_device_t *_dev)
{
	if(!_dev->get_geometry)
		return NULL;

	return _dev->get_geometry(_dev);
}

int nand_device_read_single_page(nand_device_t *_dev, uint32_t _chip, uint32_t _block,
		uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer)
{
	if(!_dev->read_single_page)
		return -1;

	return _dev->read_single_page(_dev, _chip, _block, _page, _buffer, _spareBuffer);
}

int nand_device_write_single_page(nand_device_t *_dev, uint32_t _chip, uint32_t _block,
		uint32_t _page, uint8_t *_buffer, uint8_t *_spareBuffer)
{
	if(!_dev->write_single_page)
		return -1;

	return _dev->write_single_page(_dev, _chip, _block, _page, _buffer, _spareBuffer);
}

void nand_device_enable_encryption(nand_device_t *_dev, int _enabled)
{
	if(_dev->enable_encryption)
		_dev->enable_encryption(_dev, _enabled);
}

