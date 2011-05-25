/*
 * bdev.c - Block device functions
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

#include "bdev.h"
#include "arm/arm.h"
#include "util.h"

static int bdev_init_done = 0;
static LinkedList bdev_list;

static inline block_device_t *bdev_get(LinkedList *_ptr)
{
	return CONTAINER_OF(block_device_t, list_ptr, _ptr);
}

static error_t block_device_prepare(block_device_t *_bdev)
{
	if(_bdev->prepare)
		return _bdev->prepare(_bdev);

	return 0;
}

static void block_device_finish(block_device_t *_bdev)
{
	if(_bdev->finish)
		_bdev->finish(_bdev);
}

static error_t block_device_read_raw(block_device_t *_bdev, void *_dest, int _sz)
{
	if(!_bdev->read)
		return -1;

	return _bdev->read(_bdev, _dest, _sz);
}

static error_t block_device_write_raw(block_device_t *_bdev, void *_src, int _sz)
{
	if(!_bdev->write)
		return -1;

	return _bdev->write(_bdev, _src, _sz);
}

static error_t block_device_seek_raw(block_device_t *_bdev, seek_mode_t _mode, int64_t _amt)
{
	if(!_bdev->seek)
		return -1;

	return _bdev->seek(_bdev, _mode, _amt);
}

static error_t block_device_sync_raw(block_device_t *_bdev)
{
	if(!_bdev->sync)
		return -1;

	return _bdev->sync(_bdev);
}

error_t block_device_init(block_device_t *_bdev)
{
	memset(&_bdev->mbr, 0, sizeof(MBR));
	memset(&_bdev->gpt, 0, sizeof(GPT));
	memset(&_bdev->lwvm, 0, sizeof(LwVM));
	
	_bdev->list_ptr.next = NULL;
	_bdev->list_ptr.prev = NULL;
	_bdev->setup_done = 0;
	_bdev->part_mode = partitioning_unknown;
	_bdev->mbr_records = _bdev->mbr.partitions;
	_bdev->gpt_records = malloc(sizeof(GPTPartitionRecord)*128);
	_bdev->lwvm_records = _bdev->lwvm.partitions;

	return SUCCESS;
}

error_t block_device_setup(block_device_t *_bdev)
{
	if(_bdev->setup_done)
		return 0;

	block_device_prepare(_bdev);

	error_t ret = block_device_seek_raw(_bdev, seek_begin, 0);
	if(SUCCEEDED(ret))
	{
		_bdev->part_mode = partitioning_mbr;
		ret = block_device_read_raw(_bdev, &_bdev->mbr, sizeof(MBR));
		if(SUCCEEDED(ret))
		{
			if(!memcmp(LwVMType, ((LwVM*)&_bdev->mbr)->type, sizeof(LwVMType))) {
				if(FAILED(block_device_seek_raw(_bdev, seek_begin, 0)) || FAILED(block_device_read_raw(_bdev, &_bdev->lwvm, sizeof(_bdev->lwvm)))) {
					bufferPrintf("bdev: detected LwVM partition table but failed to read it.\r\n");
					return EIO;
				}
				LwVM_rangeShiftValue = 32 - __builtin_clz((_bdev->lwvm.mediaSize-1) >> 10);
				LwVM_rangeByteCount = 1 << LwVM_rangeShiftValue;
				LwVM_numValidChunks = _bdev->lwvm.mediaSize >> LwVM_rangeShiftValue;
				LwVM_chunks = malloc(_bdev->lwvm.numPartitions*sizeof(uint32_t));
				memset(LwVM_chunks, 0, _bdev->lwvm.numPartitions*sizeof(uint32_t));
				int i;
				for(i = 0; i < _bdev->lwvm.numPartitions; i++)
				{
					LwVM_chunks[i] = malloc(0x800);
					memset(LwVM_chunks[i], 0xFF, 0x800);
					int j;
					for (j = 1; j < 0x400; j++) {
						uint16_t chunk_unk = _bdev->lwvm.chunks[j];
						if(chunk_unk >> 12 == i) {
							LwVM_chunks[i][chunk_unk & 0x3ff] = j;
						}
					}
					LwVMPartitionRecord *record = &_bdev->lwvm.partitions[i];
					char *string = malloc(sizeof(record->partitionName)/2);
					memset(string, 0, sizeof(string));
					for (j = 0; record->partitionName[j*2] != 0; j++) {
						string[j] = record->partitionName[j*2];
					}
					bufferPrintf("bdev: partition id: %d, name: %s, range: %u - %u\r\n", i, string, (uint32_t)record->begin, (uint32_t)record->end);
					free(string);
					if(i == 8)
						break;
				}
				_bdev->part_mode = partitioning_lwvm;
				block_device_finish(_bdev);
				_bdev->setup_done = 1;
				return SUCCESS;
			}

			int i;
			for(i = 0; i < ARRAY_SIZE(_bdev->mbr.partitions); i++)
			{
				MBRPartitionRecord *record = &_bdev->mbr.partitions[i];	
				if(record->type == 0xee)
					_bdev->part_mode = partitioning_gpt;
		
				bufferPrintf("bdev: partition id: %d, type: %x, sectors: %d - %d\r\n", i, record->type, record->beginLBA, record->beginLBA + record->numSectors);
			}

			if(_bdev->part_mode == partitioning_gpt)
			{
				int block_size = block_device_block_size(_bdev);
				if(block_size > 0)
				{
					ret = block_device_seek_raw(_bdev, seek_begin, block_size);
					if(SUCCEEDED(ret))
					{
						ret = block_device_read_raw(_bdev, &_bdev->gpt, sizeof(_bdev->gpt));
						if(SUCCEEDED(ret))
						{
							uint32_t oldCRC32 = _bdev->gpt.headerChecksum;
							uint32_t newCRC32 = 0;

							_bdev->gpt.headerChecksum = 0; // So it doesn't interfere with CRC32 calculation -- Ricky26
							crc32(&newCRC32, &_bdev->gpt, sizeof(_bdev->gpt));

							if(oldCRC32 != newCRC32)
							{
								bufferPrintf("bdev: GPT CRC32 doesn't match, falling back to MBR.\n");
								_bdev->part_mode = partitioning_mbr;
							}
							else
							{
								bufferPrintf("bdev: guid partition table detected\n");

								ret = block_device_seek_raw(_bdev, seek_begin, _bdev->gpt.partitionEntriesFirstLBA * block_size);
								if(SUCCEEDED(ret))
								{
									ret = block_device_read_raw(_bdev, _bdev->gpt_records, sizeof(GPTPartitionRecord) * (_bdev->gpt.numPartitions > 128 ? 128 : _bdev->gpt.numPartitions));
									if(SUCCEEDED(ret))
									{
										i = 0;
										while(_bdev->gpt_records[i].type[1] != 0)
										{
											bufferPrintf("bdev: partition id: %d, sectors: %d - %d\r\n", i, (uint32_t)_bdev->gpt_records[i].beginLBA, (uint32_t)_bdev->gpt_records[i].endLBA);
											i++;
										}
									}
									else
									{
										bufferPrintf("bdev: Failed to read GPT entries, falling back to MBR.\n");
										_bdev->part_mode = partitioning_mbr;
									}
								}
								else
								{
									bufferPrintf("bdev: Failed to seek to GPT entries, falling back to MBR.\n");
									_bdev->part_mode = partitioning_mbr;
								}
							}

							_bdev->gpt.headerChecksum = oldCRC32;
						}
						else
						{
							bufferPrintf("bdev: Failed to read GPT, falling back to MBR.\n");
							_bdev->part_mode = partitioning_mbr;
						}
					}
					else
					{
						bufferPrintf("bdev: Failed to seek to GPT, falling back to MBR.\n");
						_bdev->part_mode = partitioning_mbr;
					}
				}
				else
				{
					bufferPrintf("bdev: Block size invalid, falling back to MBR.\n");
					_bdev->part_mode = partitioning_mbr;
				}
			}
		}
		else
			bufferPrintf("bdev: Failed to read MBR from block device 0x%p.\n", _bdev);
	}
	else
		bufferPrintf("bdev: Failed to seek to beginning of block device 0x%p.\n", _bdev);

	block_device_finish(_bdev);

	_bdev->setup_done = 1;

	return SUCCESS;
}

error_t block_device_register(block_device_t *_bdev)
{
	EnterCriticalSection();

	if(!bdev_init_done)
	{
		bdev_init_done = 1;
		bdev_list.prev = &bdev_list;
		bdev_list.next = &bdev_list;
	}

	LinkedList *prev = bdev_list.prev;
	_bdev->list_ptr.prev = prev;
	_bdev->list_ptr.next = &bdev_list;
	prev->next = &_bdev->list_ptr;
	bdev_list.prev = &_bdev->list_ptr;
	LeaveCriticalSection();

	bufferPrintf("bdev: Registered new device, 0x%p.\n", _bdev);

	return block_device_setup(_bdev);
}

void block_device_unregister(block_device_t *_bdev)
{
	if(_bdev->list_ptr.next != NULL && _bdev->list_ptr.prev != NULL)
	{
		EnterCriticalSection();
		LinkedList *prev = _bdev->list_ptr.prev;
		LinkedList *next = _bdev->list_ptr.next;
		next->prev = prev;
		prev->next = next;
		_bdev->list_ptr.prev = NULL;
		_bdev->list_ptr.next = NULL;
		LeaveCriticalSection();
	}
}

block_device_t *block_device_find(block_device_t *_prev)
{
	LinkedList *ptr;
	if(_prev == NULL)
		ptr = bdev_list.next;
	else
		ptr = &_prev->list_ptr;

	if(ptr == &bdev_list)
		return NULL;

	return bdev_get(ptr);
}

int block_device_size(block_device_t *_bdev)
{
	if(_bdev->size)
		return _bdev->size(_bdev);

	return -1;
}

int block_device_block_size(block_device_t *_bdev)
{
	if(_bdev->block_size)
		return _bdev->block_size(_bdev);
	
	return 1;
}

int block_device_partition_count(block_device_t *_bdev)
{
	switch(_bdev->part_mode)
	{
	case partitioning_none:
		return 1;
	
	case partitioning_mbr:
		return 4;

	case partitioning_gpt:
		return _bdev->gpt.numPartitions;

	case partitioning_lwvm:
		return _bdev->lwvm.numPartitions;

	default:
		return 0;
	}
}

block_device_handle_t block_device_open(block_device_t *_bdev, int _idx)
{
	if(_idx < 0 || _idx >= block_device_partition_count(_bdev))
		return NULL;

	if(block_device_prepare(_bdev))
		return NULL;

	block_device_handle_t ret = malloc(sizeof(block_device_handle_struct_t));
	memset(ret, 0, sizeof(block_device_handle_struct_t));

	ret->pIdx = _idx;
	ret->device = _bdev;

	switch(_bdev->part_mode)
	{
	case partitioning_none:
		return ret;

	case partitioning_mbr:
		ret->mbr_record = &_bdev->mbr_records[_idx];
		return ret;
		
	case partitioning_gpt:
		ret->gpt_record = &_bdev->gpt_records[_idx];
		return ret;

	case partitioning_lwvm:
		ret->lwvm_record = &_bdev->lwvm_records[_idx];
		return ret;

	default:
		break;
	}

	free(ret);
	block_device_finish(_bdev);
	return NULL;
}

void block_device_close(block_device_handle_t _handle)
{
	block_device_finish(_handle->device);
	free(_handle);
}

int block_device_get_start(block_device_handle_t _handle)
{
	int block_size = block_device_block_size(_handle->device);
	switch(_handle->device->part_mode)
	{
	case partitioning_mbr:
		return _handle->mbr_record->beginLBA * block_size;
		break;

	case partitioning_gpt:
		return _handle->gpt_record->beginLBA * block_size;
		break;

	case partitioning_lwvm:
		return _handle->lwvm_record->begin + 1020 * block_size;
		break;

	default:
		return 0;
	}
}

error_t block_device_read(block_device_handle_t _handle, void *_dest, int _sz)
{
	return block_device_read_raw(_handle->device, _dest, _sz);
}

error_t block_device_write(block_device_handle_t _handle, void *_src, int _sz)
{
	return block_device_write_raw(_handle->device, _src, _sz);
}

error_t block_device_seek(block_device_handle_t _handle, seek_mode_t _mode, int64_t _amt)
{
	block_device_t *bdev = _handle->device;

	if(_mode == seek_offset)
		return block_device_seek_raw(bdev, _mode, _amt);

	int block_size = block_device_block_size(bdev);

	switch(_mode)
	{
	case seek_begin:
		{
			switch(bdev->part_mode)
			{
			case partitioning_mbr:
				_amt += _handle->mbr_record->beginLBA * block_size;
				break;

			case partitioning_gpt:
				_amt += _handle->gpt_record->beginLBA * block_size;
				break;

			case partitioning_lwvm:
				_amt = ((uint64_t)LwVM_chunks[_handle->pIdx][_amt >> LwVM_rangeShiftValue] << LwVM_rangeShiftValue) + (_amt & (LwVM_rangeByteCount - 1));
				break;

			default:
				break;
			}

			return block_device_seek_raw(bdev, _mode, _amt);
		}

	case seek_end:
		{
			switch(bdev->part_mode)
			{
			case partitioning_mbr:
				_amt += (_handle->mbr_record->beginLBA + _handle->mbr_record->numSectors) * block_size;
				break;

			case partitioning_gpt:
				_amt += _handle->gpt_record->endLBA * block_size;
				break;

			case partitioning_lwvm:
				_amt += _handle->lwvm_record->end;
				_amt = ((uint64_t)LwVM_chunks[_handle->pIdx][_amt >> LwVM_rangeShiftValue] << LwVM_rangeShiftValue) + (_amt & (LwVM_rangeByteCount - 1));
				break;

			default:
				break;
			}

			return block_device_seek_raw(bdev, _mode, _amt);
		}

	default:
		return EINVAL;
	};
}

error_t block_device_sync(block_device_handle_t _h)
{
	return block_device_sync_raw(_h->device);
}
