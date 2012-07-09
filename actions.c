/*
 * actions.c - OpeniBoot Actions
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

#include "actions.h"
#include "commands.h"
#include "hardware/platform.h"
#include "arm/arm.h"
#include "lcd.h"
#include "mmu.h"
#include "util.h"
#include "hfs/fs.h"
#include "wdt.h"
#include "dma.h"
#include "radio.h"
#include "syscfg.h"
#include "scripting.h"
#include "images.h"
#include "multitouch.h"
#include "pmu.h"
#include "platform.h"
#include "timer.h"

#define MACH_APPLE_IPHONE	1506
#ifndef MACH_ID
#define MACH_ID MACH_APPLE_IPHONE
#endif

#ifndef INITRD_LOAD
#ifdef CONFIG_S5L8720
#define INITRD_LOAD (RAMStart + 0x07000000)
#else
#define INITRD_LOAD (RAMStart + 0x08000000)
#endif
#endif

/* this code is adapted from http://www.simtec.co.uk/products/SWLINUX/files/booting_article.html, which is distributed under the BSD license */

/* list of possible tags */
#define ATAG_NONE       0x00000000
#define ATAG_CORE       0x54410001
#define ATAG_MEM        0x54410002
#define ATAG_VIDEOTEXT  0x54410003
#define ATAG_RAMDISK    0x54410004
#define ATAG_INITRD2    0x54420005
#define ATAG_SERIAL     0x54410006
#define ATAG_REVISION   0x54410007
#define ATAG_VIDEOLFB   0x54410008
#define ATAG_CMDLINE    0x54410009
#define ATAG_IPHONE_NAND       0x54411001
#define ATAG_IPHONE_WIFI       0x54411002
#define ATAG_IPHONE_PROX_CAL   0x54411004
#define ATAG_IPHONE_MT_CAL     0x54411005

/* structures for each atag */
struct atag_header {
	uint32_t size; /* length of tag in words including this header */
	uint32_t tag;  /* tag type */
};

struct atag_core {
	uint32_t flags;
	uint32_t pagesize;
	uint32_t rootdev;
};

struct atag_mem {
	uint32_t     size;
	uint32_t     start;
};

struct atag_videotext {
	uint8_t              x;
	uint8_t              y;
	uint16_t             video_page;
	uint8_t              video_mode;
	uint8_t              video_cols;
	uint16_t             video_ega_bx;
	uint8_t              video_lines;
	uint8_t              video_isvga;
	uint16_t             video_points;
};

struct atag_ramdisk {
	uint32_t flags;
	uint32_t size;
	uint32_t start;
};

struct atag_initrd2 {
	uint32_t start;
	uint32_t size;
};

struct atag_serialnr {
	uint32_t low;
	uint32_t high;
};

struct atag_revision {
	uint32_t rev;
};

struct atag_videolfb {
	uint16_t             lfb_width;
	uint16_t             lfb_height;
	uint16_t             lfb_depth;
	uint16_t             lfb_linelength;
	uint32_t             lfb_base;
	uint32_t             lfb_size;
	uint8_t              red_size;
	uint8_t              red_pos;
	uint8_t              green_size;
	uint8_t              green_pos;
	uint8_t              blue_size;
	uint8_t              blue_pos;
	uint8_t              rsvd_size;
	uint8_t              rsvd_pos;
};

struct atag_cmdline {
	char    cmdline[1];
};

struct atag_iphone_nand {
	uint32_t	nandID;
	uint32_t	numBanks;
	int		banksTable[8];
	ExtentList	extentList;
};

#define ATAG_IPHONE_WIFI_TYPE_2G 0
#define ATAG_IPHONE_WIFI_TYPE_IPHONE_3G 1
#define ATAG_IPHONE_WIFI_TYPE_IPOD 2

struct atag_iphone_wifi {
	uint8_t		mac[6];
	uint32_t	calSize;
	uint8_t		cal[];
};

struct atag_iphone_cal_data {
	uint32_t	size;
	uint8_t		data[];
};

struct atag {
	struct atag_header hdr;
	union {
		struct atag_core             core;
		struct atag_mem              mem;
		struct atag_videotext        videotext;
		struct atag_ramdisk          ramdisk;
		struct atag_initrd2          initrd2;
		struct atag_serialnr         serialnr;
		struct atag_revision         revision;
		struct atag_videolfb         videolfb;
		struct atag_cmdline          cmdline;
		struct atag_iphone_nand      nand;
		struct atag_iphone_wifi      wifi;
		struct atag_iphone_cal_data  mt_cal;
	} u;
};

error_t chainload(uint32_t address)
{
	EnterCriticalSection();

#ifndef MALLOC_NO_WDT
	wdt_disable();
#endif

	arm_disable_caches();
	mmu_disable();
	CallArm(address);
	return SUCCESS;
}

error_t chainload_image(char* name)
{
	Image* image = images_get(fourcc(name));
	if(image == NULL)
		return ENOENT;

	void* imageData;
	images_read(image, &imageData);
	return chainload((uint32_t)imageData);
}

#define tag_next(t)     ((struct atag *)((uint32_t *)(t) + (t)->hdr.size))
#define tag_size(type)  ((sizeof(struct atag_header) + sizeof(struct type)) >> 2)
static struct atag *params; /* used to point at the current tag */

static void setup_core_tag(void * address, long pagesize)
{
	params = (struct atag *)address;         /* Initialise parameters to start at given address */

	params->hdr.tag = ATAG_CORE;            /* start with the core tag */
	params->hdr.size = tag_size(atag_core); /* size the tag */

	params->u.core.flags = 1;               /* ensure read-only */
	params->u.core.pagesize = pagesize;     /* systems pagesize (4k) */
	params->u.core.rootdev = 0;             /* zero root device (typicaly overidden from commandline )*/

	params = tag_next(params);              /* move pointer to next tag */
}

static void setup_ramdisk_tag(uint32_t size)
{
	params->hdr.tag = ATAG_RAMDISK;         /* Ramdisk tag */
	params->hdr.size = tag_size(atag_ramdisk);  /* size tag */

	params->u.ramdisk.flags = 0;            /* Load the ramdisk */
	params->u.ramdisk.size = size;          /* Decompressed ramdisk size */
	params->u.ramdisk.start = 0;            /* Unused */

	params = tag_next(params);              /* move pointer to next tag */
}

static void setup_initrd2_tag(uint32_t start, uint32_t size)
{
	params->hdr.tag = ATAG_INITRD2;         /* Initrd2 tag */
	params->hdr.size = tag_size(atag_initrd2);  /* size tag */

	if(start > RAMStart)
		start -= RAMStart;

	params->u.initrd2.start = start;        /* physical start */
	params->u.initrd2.size = size;          /* compressed ramdisk size */

	params = tag_next(params);              /* move pointer to next tag */
}

static void setup_mem_tag(uint32_t start, uint32_t len)
{
	params->hdr.tag = ATAG_MEM;             /* Memory tag */
	params->hdr.size = tag_size(atag_mem);  /* size tag */

	params->u.mem.start = start;            /* Start of memory area (physical address) */
	params->u.mem.size = len;               /* Length of area */

	params = tag_next(params);              /* move pointer to next tag */
}

static void setup_cmdline_tag(const char * line)
{
	int linelen = strlen(line) + 1;

	if(!linelen)
		return;                             /* do not insert a tag for an empty commandline */

	params->hdr.tag = ATAG_CMDLINE;         /* Commandline tag */
	params->hdr.size = (sizeof(struct atag_header) + linelen + 1 + 4) >> 2;

	strcpy(params->u.cmdline.cmdline,line); /* place commandline into tag */

	params = tag_next(params);              /* move pointer to next tag */
}

static void setup_wifi_tags()
{
#if defined(CONFIG_IPHONE_2G) || defined(CONFIG_IPHONE_3G)
	uint8_t* mac;
	uint32_t calSize;
	uint8_t* cal;

	if(radio_nvram_get(2, &mac) < 0)
		return;

	if((calSize = radio_nvram_get(1, &cal)) < 0)
		return;

	memcpy(&params->u.wifi.mac, mac, 6);
	params->u.wifi.calSize = calSize;
	memcpy(params->u.wifi.cal, cal, calSize);

	params->hdr.tag = ATAG_IPHONE_WIFI;         /* iPhone NAND tag */
	params->hdr.size = (sizeof(struct atag_header) + sizeof(struct atag_iphone_wifi) + calSize + 4) >> 2;
	params = tag_next(params);              /* move pointer to next tag */
#endif
}

static void setup_prox_tag()
{
#ifdef CONFIG_IPHONE_3G
	uint8_t* prox_cal;
	int prox_cal_size;

	prox_cal = syscfg_get_entry(SCFG_PxCl, &prox_cal_size);
	if(!prox_cal)
	{
		return;
	}

	params->u.mt_cal.size = prox_cal_size;
	memcpy(params->u.mt_cal.data, prox_cal, prox_cal_size);

	params->hdr.tag = ATAG_IPHONE_PROX_CAL;
	params->hdr.size = (sizeof(struct atag_header) + sizeof(struct atag_iphone_cal_data) + prox_cal_size + 4) >> 2;
	params = tag_next(params);              /* move pointer to next tag */

	bufferPrintf("Proximity calibration data installed.\r\n");
#endif
}

static void setup_mt_tag()
{
#ifndef CONFIG_IPHONE_2G
	uint8_t* cal;
	int cal_size;

	cal = syscfg_get_entry(SCFG_MtCl, &cal_size);
	if(!cal)
	{
		return;
	}

	params->u.mt_cal.size = cal_size;
	memcpy(params->u.mt_cal.data, cal, cal_size);

	params->hdr.tag = ATAG_IPHONE_MT_CAL;
	params->hdr.size = (sizeof(struct atag_header) + sizeof(struct atag_iphone_cal_data) + cal_size + 4) >> 2;
	params = tag_next(params);              /* move pointer to next tag */

	bufferPrintf("Multi-touch calibration data installed.\r\n");
#endif
}

static void setup_end_tag()
{
	params->hdr.tag = ATAG_NONE;            /* Empty tag ends list */
	params->hdr.size = 0;                   /* zero length */
}

static void* kernel = NULL;
static uint32_t kernelSize;
static void* ramdisk = NULL;
static uint32_t ramdiskSize;
static uint32_t ramdiskRealSize;

void set_ramdisk(void* location, int size) {
	if(ramdisk)
		free(ramdisk);

	if(location && size && *((unsigned short*)location) == 0x8b1f)
	{
		bufferPrintf("initrd: Detected GZipped InitRD.\n");

		// the gzip file format places the uncompressed length in the last four bytes of the file. Read it and calculate the size in KB.
		ramdiskRealSize = ((*((uint32_t*)((uint8_t*)location + size - sizeof(uint32_t)))) + 1023) / 1024;
	}
	else
		ramdiskRealSize = 0;

	ramdiskSize = size;

	bufferPrintf("%s\n", __func__);

	if(location)
	{
		ramdisk = malloc(size);
		memcpy(ramdisk, location, size);
	}
	else
		ramdisk = NULL;

	bufferPrintf("%s\n", __func__);
}

void set_kernel(void* location, int size) {
	if(kernel)
		free(kernel);

	kernelSize = size;
	kernel = malloc(size);
	memcpy(kernel, location, size);
}


static void setup_tags(struct atag* parameters, const char* commandLine)
{
	setup_core_tag(parameters, 4096);       /* standard core tag 4k pagesize */
	setup_mem_tag(MemoryStart, RAMEnd-RAMStart);    /* 128Mb at 0x00000000 */

	if(ramdiskRealSize > 0)
		setup_ramdisk_tag(ramdiskRealSize);

	if(ramdisk != NULL && ramdiskSize > 0)
		setup_initrd2_tag(INITRD_LOAD, ramdiskSize);
	else
		setup_initrd2_tag(0, 0); // Prevent Linux from using defaults.

	setup_cmdline_tag(commandLine);
	setup_prox_tag();
	setup_mt_tag();
	setup_wifi_tags();
	setup_end_tag();                    /* end of tags */
}

static error_t load_multitouch_images()
{
#ifdef CONFIG_IPHONE_2G
	Image* image = images_get(fourcc("mtza"));
	if (image == NULL)
		return ENOENT;

	void* aspeedData;
	size_t aspeedLength = images_read(image, &aspeedData);
	
	image = images_get(fourcc("mtzm"));
	if(image == NULL)
		return ENOENT;
	
	void* mainData;
	size_t mainLength = images_read(image, &mainData);
	
	multitouch_setup(aspeedData, aspeedLength, mainData,mainLength);
	free(aspeedData);
	free(mainData);
#endif

#if defined(CONFIG_IPHONE_3G) || defined(CONFIG_IPOD_TOUCH_1G)
	Image* image = images_get(fourcc("mtz2"));
	if(image == NULL)
		return ENOENT;

	void* imageData;
	size_t length = images_read(image, &imageData);
	
	multitouch_setup(imageData, length);
	free(imageData);
#endif

    return SUCCESS;
}

void boot_linux(const char* args, uint32_t mach_type) {
	uint32_t exec_at = (uint32_t) kernel;
	uint32_t param_at = exec_at - 0x2000;
	int i;

	if(exec_at > RAMStart)
		exec_at = (exec_at - RAMStart) + MemoryStart;

	load_multitouch_images();

	setup_tags((struct atag*) param_at, args);

	EnterCriticalSection();
	exit_modules();
	platform_shutdown();

	bufferPrintf("Booting Linux (0x%08x)...\r\n", mach_type);

	/* FIXME: This overwrites openiboot! We make the semi-reasonable assumption
	 * that this function's own code doesn't reside in 0x0100-0x1100 */

	if(ramdiskSize > 0 && ramdisk != (void*)INITRD_LOAD)
	{
		for(i = 0; i < ((ramdiskSize + sizeof(uint32_t) - 1)/sizeof(uint32_t)); i++) {
			((uint32_t*)INITRD_LOAD)[i] = ((uint32_t*)ramdisk)[i];
		}
	}

	for(i = 0; i < (0x1000/sizeof(uint32_t)); i++) {
		((uint32_t*)0x100)[i] = ((uint32_t*)param_at)[i];
	}

	asm (	"MOV	R4, %0\n"
		"MOV	R0, #0\n"
		"MOV	R1, %1\n"
		"MOV	R2, %2\n"
		"BX	R4"
		:
		: "r"(exec_at), "r"(mach_type), "r"(0x100)
	    );
}

static BootEntry rootEntry;
static BootEntry *currentEntry;
static BootEntry *defaultEntry;

static void setup_init()
{
	memset(&rootEntry, 0, sizeof(rootEntry));
	rootEntry.type = kBootAuto;
	rootEntry.title = "";
	rootEntry.machine = MACH_ID;
	rootEntry.list_ptr.next = &rootEntry;
	rootEntry.list_ptr.prev = &rootEntry;
	currentEntry = &rootEntry;
	defaultEntry = &rootEntry;
}
MODULE_INIT_BOOT(setup_init);

BootEntry *setup_root()
{
	return &rootEntry;
}

BootEntry *setup_current()
{
	return currentEntry;
}

BootEntry *setup_default()
{
	return defaultEntry;
}

void setup_title(const char *title)
{
	BootEntry *entry = rootEntry.list_ptr.next;
	while(entry != &rootEntry)
	{
		if(entry->title != NULL)
		{
			if(strcmp(entry->title, title) == 0)
			{
				currentEntry = entry;
				return;
			}
		}

		entry = entry->list_ptr.next;
	}

	BootEntry *prev = rootEntry.list_ptr.prev;

	entry = malloc(sizeof(BootEntry));
	memset(entry, 0, sizeof(BootEntry));

	entry->title = strdup(title);
	entry->machine = MACH_ID;

	entry->list_ptr.prev = prev;
	entry->list_ptr.next = &rootEntry;
	rootEntry.list_ptr.prev = entry;
	prev->list_ptr.next = entry;
	currentEntry = entry;

	if(defaultEntry == &rootEntry)
		defaultEntry = currentEntry;
}

void setup_entry(BootEntry *entry)
{
	currentEntry = entry;
}

void setup_auto()
{
	currentEntry->type = kBootAuto;
}

void setup_kernel(const char *kernel, const char *args)
{
	if(currentEntry->kernel != NULL)
		free(currentEntry->kernel);

	if(currentEntry->path != NULL)
		free(currentEntry->path);

	currentEntry->type = kBootLinux;

	if(kernel == NULL)
		currentEntry->kernel = NULL;
	else
		currentEntry->kernel = strdup(kernel);

	if(args == NULL)
		currentEntry->path = NULL;
	else
		currentEntry->path = strdup(args);
}

void setup_initrd(const char *initrd)
{
	if(currentEntry->ramdisk != NULL)
		free(currentEntry->ramdisk);

	if(initrd == NULL)
		currentEntry->ramdisk = NULL;
	else
		currentEntry->ramdisk = strdup(initrd);
}

void setup_image(const char *image)
{
	if(currentEntry->path != NULL)
		free(currentEntry->path);

	currentEntry->type = kBootImage;
	currentEntry->path = strdup(image);
}

error_t setup_boot()
{
	switch(currentEntry->type)
	{
	case kBootAuto:
		// This should be the default node,
		// try and boot ibox, failing that
		// boot ibot.
		if(chainload_image("ibox") != 0)
			return chainload_image("ibot");
		return SUCCESS;

	case kBootImage:
		return chainload_image(currentEntry->path);

	case kBootLinux:
		{
			if(currentEntry->kernel == NULL)
			{
				bufferPrintf("setup: Can't boot linux without a kernel!\n");
				return EINVAL;
			}

			uint32_t size;
			uint8_t *data = script_load_file(currentEntry->kernel, &size);
			if(data == NULL)
			{
				bufferPrintf("setup: Failed to load kernel!\n");
				return EINVAL;
			}

			bufferPrintf("setup: Loaded kernel at 0x%08x.\n", data);
			set_kernel(data, size);
			free(data);

			if(currentEntry->ramdisk != NULL)
			{
				data = script_load_file(currentEntry->ramdisk, &size);
				if(data == NULL)
				{
					bufferPrintf("setup: Failed to load ramdisk.\n");
					return EINVAL;
				}

				bufferPrintf("setup: Loaded ramdisk at 0x%08x.\n", data);
				set_ramdisk(data, size);
				free(data);
			}

			if(currentEntry->path == NULL)
				boot_linux("console=tty root=/dev/ram0 init=/init rw", currentEntry->machine);
			else
				boot_linux(currentEntry->path, currentEntry->machine);

			return SUCCESS;
		}

	case kBootChainload:
		{
			uint32_t addr = parseNumber(currentEntry->path);
			return chainload(addr);
		}
	}

	bufferPrintf("setup: Invalid boot entry selected.\n");
	return EINVAL;
}

static error_t cmd_setup_title(int argc, char **argv)
{
	if(argc != 2)
	{
		bufferPrintf("Usage: %s [title]\n", argv[0]);
		return EINVAL;
	} else
		setup_title(argv[1]);

	return SUCCESS;
}
COMMAND("title", "Select a boot entry by title.", cmd_setup_title);

static error_t cmd_setup_default(int argc, char **argv)
{
	if(argc > 1)
	{
		bufferPrintf("Usage: %s\n", argv[0]);
		return EINVAL;
	} else
		defaultEntry = setup_current();

	return SUCCESS;
}
COMMAND("default", "Set the current entry as the default.", cmd_setup_default);

static error_t cmd_setup_auto(int argc, char **argv)
{
	if(argc > 1)
	{
		bufferPrintf("Usage: %s\n", argv[0]);
		return EINVAL;
	} else
		setup_auto();

	return SUCCESS;
}
COMMAND("auto", "Set current boot entry to boot fallback bootloader.\n", cmd_setup_auto);

static error_t cmd_setup_kernel(int argc, char **argv)

{
	if(argc <= 1)
	{
		// No arguments, just use the last sent file, with no command line
		// TODO: This leaks memory, fix it. -- Ricky26
		char *ptr = malloc(received_file_size);
		memcpy(ptr, (void*)0x09000000, received_file_size);
		char buff[128];
		sprintf(buff, "%d+%d", (uint32_t)ptr, received_file_size);
		setup_kernel(buff, NULL);
	}
	else if(argc == 2)
	{
		char *ptr = malloc(received_file_size);
		memcpy(ptr, (void*)0x09000000, received_file_size);
		char buff[128];
		sprintf(buff, "%d+%d", (uint32_t)ptr, received_file_size);
		setup_kernel(buff, argv[1]);
	}
	else if(argc == 3)
		setup_kernel(argv[1], argv[2]);
	else {
		bufferPrintf("Usage: %s [kernel] [command line]\n", argv[0]);
		return EINVAL;
	}

	return SUCCESS;
}
COMMAND("kernel", "Set the kernel of the current boot entry.", cmd_setup_kernel);

static error_t cmd_setup_initrd(int argc, char **argv)
{
	if(argc <= 1)
	{
		char *ptr = malloc(received_file_size);
		memcpy(ptr, (void*)0x09000000, received_file_size);

		char buff[128];
		sprintf(buff, "%d+%d", (uint32_t)ptr, received_file_size);
		setup_initrd(buff);
	}
	else if(argc == 2)
		setup_initrd(argv[1]);
	else {
		bufferPrintf("Usage: %s [initrd]\n", argv[0]);
		return EINVAL;
	}

	return SUCCESS;
}
COMMAND("initrd", "Set the ramdisk for the current boot entry.", cmd_setup_initrd);

static error_t cmd_setup_image(int argc, char **argv)
{
	if(argc != 2) {
		bufferPrintf("Usage: %s [image]\n", argv[0]);
		return -1;
	}
	else
		setup_image(argv[1]);

	return SUCCESS;
}
COMMAND("image", "Set the image to chainload for the current boot entry.", cmd_setup_image);

static error_t cmd_setup_machine(int argc, char **argv)
{
	if(argc != 2)
	{
		bufferPrintf("Usage: %s machine_id\n", argv[0]);
		bufferPrintf("Current machine ID: %d.\r\n", currentEntry->machine);
		return EINVAL;
	}

	uint32_t num;
	if(strcmp(argv[1], "default") == 0)
		num = MACH_ID;
	else if(strcmp(argv[1], "compat") == 0)
		num = MACH_APPLE_IPHONE;
	else
		num = parseNumber(argv[1]);

	currentEntry->machine = num;

	return SUCCESS;
}
COMMAND("machine_id", "Select a machine ID for booting the linux kernel.", cmd_setup_machine);

static error_t cmd_setup_boot(int argc, char **argv)
{
	if(argc > 1) {
		bufferPrintf("Usage: %s\n", argv[0]);
		return EINVAL;
	}
	else
		setup_boot();

	return SUCCESS;
}
COMMAND("boot", "Boot the current boot entry.", cmd_setup_boot);

static error_t cmd_go(int argc, char** argv)
{
	uint32_t address;

	if(argc < 2) {
		address = 0x09000000;
	} else {
		address = parseNumber(argv[1]);
	}

	bufferPrintf("Jumping to 0x%x (interrupts disabled)\r\n", address);

	// make as if iBoot was called from ROM
	pmu_set_iboot_stage(0x1F);

	udelay(100000);

	return chainload(address);
}
COMMAND("go", "jump to a specified address (interrupts disabled)", cmd_go);

static error_t cmd_jump(int argc, char** argv)
{
	if(argc < 2) {
		bufferPrintf("Usage: %s <address>\r\n", argv[0]);
		return EINVAL;
	}

	uint32_t address = parseNumber(argv[1]);
	bufferPrintf("Jumping to 0x%x\r\n", address);
	udelay(100000);

	CallArm(address);

	return SUCCESS;
}
COMMAND("jump", "jump to a specified address (interrupts enabled)", cmd_jump);
