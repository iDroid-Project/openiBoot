#ifndef ACTIONS_H
#define ACTIONS_H

#include "openiboot.h"

typedef enum _BootType
{
	kBootAuto,
	kBootLinux,
	kBootImage,
	kBootChainload,
} BootType;

typedef struct _BootEntry
{
	BootType type;
	char *title;
	char *path; // Use for linux kernel args and image name
	char *kernel;
	char *ramdisk;
	uint32_t machine; // Used for linux kernels.

	LinkedList list_ptr;
} BootEntry;

void chainload(uint32_t address);
int chainload_image(char *name);
void set_kernel(void* location, int size);
void set_ramdisk(void* location, int size);
void boot_linux(const char* args, uint32_t mach_id);

BootEntry *setup_root();
BootEntry *setup_current();
BootEntry *setup_default();

void setup_title(const char *title);
void setup_entry(BootEntry *entry);
void setup_auto();
void setup_kernel(const char *kernel, const char *args);
void setup_initrd(const char *initrd);
void setup_image(const char *image);
int setup_boot();

#endif
