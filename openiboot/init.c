/*
 * init.c
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
