#include "vfl.h"
#include "chipid.h"
#include "util.h"

// VFL types
#ifdef CONFIG_VFL_VFL
#include "vfl/vfl.h"
#endif

#ifdef CONFIG_VFL_VSVFL
#include "vfl/vsvfl.h"
#endif

void vfl_init(vfl_device_t *_vfl)
{
	memset(_vfl, 0, sizeof(*_vfl));
}

void vfl_cleanup(vfl_device_t *_vfl)
{
}

vfl_device_t *vfl_allocate()
{
	vfl_device_t *ret = malloc(sizeof(*ret));
	vfl_init(ret);
	return ret;
}

error_t vfl_open(vfl_device_t *_vfl, nand_device_t *_dev)
{
	if(!_vfl->open)
		return ENOENT;

	return _vfl->open(_vfl, _dev);
}

void vfl_close(vfl_device_t *_vfl)
{
	if(!_vfl->close)
		return;

	_vfl->close(_vfl);
}

error_t vfl_read_single_page(vfl_device_t *_vfl, uint32_t _page, uint8_t* buffer, uint8_t* spare,
		int empty_ok, int* refresh_page)
{
	if(!_vfl->read_single_page)
		return ENOENT;

	return _vfl->read_single_page(_vfl, _page, buffer, spare, empty_ok, refresh_page);
}

error_t vfl_detect(vfl_device_t **_vfl, nand_device_t *_nand, vfl_signature_style_t _sign)
{
	uint8_t sigbuf[264];

	error_t ret;
	switch(_sign)
	{
	case vfl_old_signature:
		// TODO: this. -- ricky26
		bufferPrintf("vfl: Old-style signature not supported.\r\n");
		return ENOENT;

	case vfl_new_signature:
		 ret = nand_device_read_special_page(_nand, 0, "NANDDRIVERSIGN\0\0", sigbuf, sizeof(sigbuf));
		 break;

	default:
		return EINVAL;
	}

	if(FAILED(ret))
		return ret;

	if(sigbuf[0] != '3' || sigbuf[3] != 'C'
			|| sigbuf[1] > '1' || sigbuf[2] > '1'
			 || sigbuf[4] > 6 || chipid_get_power_epoch() + '0' != sigbuf[2])
	{
		bufferPrintf("vfl: Incompatible signature.\r\n");
		return ENOENT;
	}

	uint32_t flags = *(uint32_t*)&sigbuf[4];
	if((_sign & 0x800) && (!(flags & 0x10000) ||
				((_sign >> 10) & 1) ||
				!((!(flags & 0x10000)) & ((_sign >> 10) & 1))
			))
	{
		bufferPrintf("vfl: warning: metadata whitening mismatch.\r\n");
	}

	if(sigbuf[1] == '1')
	{
		bufferPrintf("vfl: Detected VSVFL.\r\n");
#ifndef CONFIG_VFL_VSVFL
		bufferPrintf("vfl: VSVFL support not included!\r\n");
		return ENOENT;
#else
		*_vfl = &vfl_vsvfl_device_allocate()->vfl;
#endif
	}
	else if(sigbuf[1] == '0')
	{
		bufferPrintf("vfl: Detected old-style VFL.\r\n");
#ifndef CONFIG_VFL_VFL
		bufferPrintf("vfl: Standard VFL support not included!\r\n");
		return ENOENT;
#else
		*_vfl = &vfl_vfl_device_allocate()->vfl;
#endif
	}
	else
	{
		bufferPrintf("vfl: No valid VFL signature found!\r\n");
		return ENOENT;
	}

	if(!*_vfl)
		return ENOENT;

	return vfl_open(*_vfl, _nand);
}
