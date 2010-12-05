#include "openiboot.h"
#include "commands.h"
#include "util.h"

initfn_t init_boot_init __attribute__ ((section(BOOT_MODULE_INIT_SECTION))) = NULL;
initfn_t init_init __attribute__ ((section(MODULE_INIT_SECTION))) = NULL;
exitfn_t exit_init __attribute__ ((section(MODULE_EXIT_SECTION))) = NULL;
OPIBCommand *command_list_init __attribute__ ((section(".commands"))) = NULL;

void init_boot_modules()
{
	bufferPrintf("init: Initializing boot modules.\n");

	initfn_t *fn = &init_boot_init;
	fn++;
	while(*fn)
	{
		(*fn)();
		fn++;
	}
}

void init_modules()
{
	bufferPrintf("init: Initializing modules.\n");

	initfn_t *fn = &init_init;
	fn++;

	while(*fn != NULL)
	{
		(*fn)();
		fn++;
	}
}

void exit_modules()
{
	bufferPrintf("init: Shutting down modules.\n");

	exitfn_t *fn = &exit_init;
	fn++;
	while(*fn)
	{
		(*fn)();
		fn++;
	}
}
