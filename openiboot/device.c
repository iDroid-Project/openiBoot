/*
 * device.c - device functions
 *
 * Copyright 2010 iDroid Project
 *
 * This file is part of iDroid. An android distribution for Apple products.
 * For more information, please visit http://www.idroidproject.org/.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include "device.h"
#include "arm/arm.h"
#include "commands.h"
#include "util.h"

static uint32_t device_init_done = 0;
static LinkedList device_list;

static inline device_t *device_get(LinkedList *_ptr)
{
	return CONTAINER_OF(device_t, list_ptr, _ptr);
}

error_t device_init(device_t *_dev)
{
	_dev->list_ptr.next = NULL;
	_dev->list_ptr.prev = NULL;

	_dev->children.next = &_dev->children;
	_dev->children.prev = &_dev->children;

	return SUCCESS;
}

error_t device_register(device_t *_dev)
{
	EnterCriticalSection();

	if(!device_init_done)
	{
		device_init_done = 1;
		device_list.prev = &device_list;
		device_list.next = &device_list;
	}

	LinkedList *prev = device_list.prev;
	_dev->list_ptr.prev = prev;
	_dev->list_ptr.next = &device_list;
	prev->next = &_dev->list_ptr;
	device_list.prev = &_dev->list_ptr;
	LeaveCriticalSection();

	return SUCCESS;
}

void device_unregister(device_t *_dev)
{
	if(_dev->list_ptr.next == NULL || _dev->list_ptr.prev == NULL)
		return;

	EnterCriticalSection();
	LinkedList *prev = _dev->list_ptr.prev;
	LinkedList *next = _dev->list_ptr.next;
	next->prev = prev;
	prev->next = next;
	_dev->list_ptr.prev = NULL;
	_dev->list_ptr.next = NULL;
	LeaveCriticalSection();
}

device_t *device_find(device_t *_where, fourcc_t _fcc, device_t *_dev)
{
	LinkedList *root;
	if(_where == NULL)
		root = &device_list;
	else
		root = &_where->children;

	LinkedList *ptr;
	if(_dev == NULL)
	{
		if(_where == NULL)
			ptr = device_list.next;
		else
			ptr = _where->children.next;
	}
	else
		ptr = _dev->list_ptr.next;

	while(ptr != root)
	{
		device_t *dev = device_get(ptr);
		if(dev->fourcc == _fcc)
			return dev;

		ptr = ptr->next;
	}

	return NULL;
}

error_t device_ioctl(device_t *_dev, uint32_t _id, void *_in, size_t _in_amt, void *_out, size_t _out_amt)
{
	if(!_dev->ioctl)
		return ENOENT;

	return _dev->ioctl(_dev, _id, _in, _in_amt, _out, _out_amt);
}

error_t device_get_info(device_t *_dev, device_info_t _item, void *_result, size_t _sz)
{
	if(_dev->get_info)
		return _dev->get_info(_dev, _item, _result, _sz);

	return ENOENT;
}

error_t device_set_info(device_t *_dev, device_info_t _item, void *_val, size_t _sz)
{
	if(_dev->set_info)
		return _dev->set_info(_dev, _item, _val, _sz);

	return ENOENT;
}

static int cmd_device_list(int _argc, char **_argv)
{
	LinkedList *ptr = device_list.next;
	if(ptr == &device_list)
	{
		bufferPrintf("devices: No devices registered.\n");
		return -1;
	}

	bufferPrintf("devices: Listing devices:\n");

	while(ptr != &device_list)
	{
		device_t *dev = device_get(ptr);
		bufferPrintf("devices:    %s\n", dev->name);

		ptr = ptr->next;
	}

	return 0;
}
COMMAND("devices", "List all registered devices.", cmd_device_list);
