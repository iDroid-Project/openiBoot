#include "openiboot.h"
#include "commands.h"
#include "util.h"

initfn_t init_boot_init __attribute__ ((section(BOOT_MODULE_INIT_SECTION))) = NULL;
initfn_t init_init __attribute__ ((section(MODULE_INIT_SECTION))) = NULL;
exitfn_t exit_init __attribute__ ((section(MODULE_EXIT_SECTION))) = NULL;
OPIBCommand *command_list_init __attribute__ ((section(".commands"))) = NULL;
static int init_boot_done = FALSE;
static int init_done = FALSE;

void init_setup()
{
	init_boot_done = FALSE;
	init_done = FALSE;
}

void init_boot_modules()
{
	if(init_boot_done)
		return;

	init_boot_done = TRUE;	

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
	if(init_done)
		return;

	init_done = TRUE;	

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
		fn++;

	// Run exit modules in reverse order,
	// so that we don't need to deal with 
	// module dependancies, we can just link
	// them in the right order. -- Ricky26
	fn--;
	while(fn > &exit_init)
	{
		(*fn)();
		fn--;
	}
}
