#ifndef  DEVICE_H
#define  DEVICE_H

#include "openiboot.h"

/**
 * @file
 *
 * This file defines the device interface.
 *
 * The device interface is designed to represent
 * any kind of device that needs to be accessed in
 * an abstract manor.
 *
 * It provides a way of getting information about a
 * device and sending IO control messages to devices.
 *
 * @defgroup Device
 */

/**
 * This is the device information enumeration.
 *
 * These are the items supported by device_get_info() and
 * device_set_info().
 *
 * @ingroup Device
 */
typedef enum _device_info
{
	// NAND
	diPagesPerBlock,
	diNumCE,
	diBlocksPerCE,
	diBytesPerPage,
	diBytesPerSpare,
	diVendorType,
	diECCBits,
	diBanksPerCE_VFL,
	diTotalBanks_VFL,
	diPagesPerBlock2,
	diECCBits2,
	diBanksPerCE,
	diBlocksPerBank,
	diBlocksPerBank_dw,
	diBanksPerCE_dw,
	diPagesPerBlock_dw,
	diPagesPerBlock2_dw,
	diPageNumberBitWidth,
	diPageNumberBitWidth2,
	diNumBusses,
	diNumCEPerBus,
	diPPN,
	diReturnOne,
	diNumECCBytes,
	diMetaPerLogicalPage,
	diPagesPerCE,
	diBankAddressSpace,

} device_info_t;

struct _device;

typedef error_t (*device_ioctl_t)(struct _device *, uint32_t _id, void *_in, size_t _in_amt, void *_out, size_t _out_amt);

typedef error_t (*device_get_info_t)(struct _device *, device_info_t _item, void *_result, size_t _sz);
typedef error_t (*device_set_info_t)(struct _device *, device_info_t _item, void *_val, size_t _sz);

/**
 * This is the device structure.
 *
 * This contains all the informations for
 * the device_* functions to work.
 *
 * @ingroup Device
 */
typedef struct _device
{
	fourcc_t fourcc;
	char *name;

	device_ioctl_t ioctl;

	device_get_info_t get_info;
	device_set_info_t set_info;

	LinkedList list_ptr;
	LinkedList children;

	void *data;
} device_t;

// Used by drivers
error_t device_init(device_t *_device);

error_t device_register(device_t *_device);
void device_unregister(device_t *_device);

// Used by clients
device_t *device_find(device_t *_where, fourcc_t _fcc, device_t *_last);

error_t device_ioctl(device_t *_dev, uint32_t _id, void *_in, size_t _in_amt, void *_out, size_t _out_amt);

error_t device_get_info(device_t *_dev, device_info_t _item, void *_result, size_t _sz);
error_t device_set_info(device_t *_dev, device_info_t _item, void *_val, size_t _sz);

#endif //DEVICE_H
