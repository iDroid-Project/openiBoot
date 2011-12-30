/*
 * syscfg.c
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

#include "openiboot.h"
#include "syscfg.h"
#include "mtd.h"
#include "util.h"
#include "commands.h"

#define SCFG_MAGIC 0x53436667
#define CNTB_MAGIC 0x434e5442
#define SCFG_LOCATION 0x4000

typedef struct SCfgHeader
{
	uint32_t magic;
	uint32_t bytes_used;
	uint32_t bytes_total;
	uint32_t version;
	uint32_t unknown;
	uint32_t entries;
} SCfgHeader;

typedef struct SCfgEntry
{
	uint32_t magic;
	union {
		uint8_t data[16];
		struct {
			uint32_t type;
			uint32_t size;
			uint32_t offset;
		} cntb;
	};
} SCfgEntry;

typedef struct OIBSyscfgEntry
{
	int type;
	uint32_t size;
	uint8_t* data;
} OIBSyscfgEntry;

static SCfgHeader header;
static OIBSyscfgEntry* entries;

static mtd_t *syscfg_dev = NULL;

static mtd_t *syscfg_device()
{
	if(!syscfg_dev)
	{
		mtd_t *ptr = NULL;
		while((ptr = mtd_find(ptr)))
		{
			if(ptr->usage == mtd_boot_images)
			{
				syscfg_dev = ptr;
				break;
			}
		}
	}

	return syscfg_dev;
}

int syscfg_setup()
{
	int i;
	SCfgEntry curEntry;
	uint32_t cursor;

	mtd_t *dev = syscfg_device();
	if(!dev)
		return -1;

	mtd_prepare(dev);
	mtd_read(dev, &header, SCFG_LOCATION, sizeof(header));
	if(header.magic != SCFG_MAGIC)
	{
		mtd_finish(dev);
		bufferPrintf("syscfg: cannot find readable syscfg partition!\r\n");
		return -1;
	}

	bufferPrintf("syscfg: found version 0x%08x with %d entries using %d of %d bytes\r\n", header.version, header.entries, header.bytes_used, header.bytes_total);

	entries = (OIBSyscfgEntry*) malloc(sizeof(OIBSyscfgEntry) * header.entries);

	cursor = SCFG_LOCATION + sizeof(header);
	for(i = 0; i < header.entries; ++i)
	{
		mtd_read(dev, &curEntry, cursor, sizeof(curEntry));

		if(curEntry.magic != CNTB_MAGIC)
		{
			entries[i].type = curEntry.magic;
			entries[i].size = 16;
			entries[i].data = (uint8_t*) malloc(16);
			memcpy(entries[i].data, curEntry.data, 16);
		} else
		{
			entries[i].type = curEntry.cntb.type;
			entries[i].size = curEntry.cntb.size;
			entries[i].data = (uint8_t*) malloc(curEntry.cntb.size);
			mtd_read(dev, entries[i].data, SCFG_LOCATION + curEntry.cntb.offset, curEntry.cntb.size);
		}

		cursor += sizeof(curEntry);
	}

	mtd_finish(dev);

	return 0;
}

uint8_t* syscfg_get_entry(uint32_t type, int* size)
{
	int i;

	for(i = 0; i < header.entries; ++i)
	{
		if(entries[i].type == type)
		{
			*size = entries[i].size;
			return entries[i].data;
		}
	}

	return NULL;
}

void syscfg_list()
{
	int i;

	for(i = 0; i < header.entries; ++i)
	{
		char *tp = (char*)&entries[i].type;

		bufferPrintf("%c%c%c%c: ", tp[3], tp[2], tp[1], tp[0]);

		int j;
		for(j = 0; j < entries[i].size; j++)
			bufferPrintf("%c", entries[i].data[j]);

		bufferPrintf("\r\n");
	}
}

static error_t cmd_syscfg_list(int argc, char **argv)
{
	syscfg_list();

	return 0;
}
COMMAND("syscfg_list", "List all syscfg entries.", cmd_syscfg_list);
