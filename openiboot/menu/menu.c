/**
 * menu.c - OpeniBoot Menu
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
#include "util.h"
#include "framebuffer.h"
#include "buttons.h"
#include "timer.h"
/*#include "images/ConsolePNG.h"
#include "images/iPhoneOSPNG.h"
#include "images/AndroidOSPNG.h"
#include "images/ConsoleSelectedPNG.h"
#include "images/iPhoneOSSelectedPNG.h"
#include "images/AndroidOSSelectedPNG.h"
#include "images/HeaderPNG.h"*/
#include "images.h"
#include "actions.h"
#include "stb_image.h"
#include "pmu.h"
#include "hfs/fs.h"
#include "scripting.h"
#include "nvram.h"
#include "tasks.h"

int globalFtlHasBeenRestored; /**< int global variable to tell wether a ftl_restore has been done. */

static TaskDescriptor menu_task; /**< TaskDescriptor Menu task */

/**
 * menu_draw.
 *
 * Draws the openiboot menu to the framebuffer.
 *
 */
void menu_draw()
{
	framebuffer_clear();

	int y = 0;
	BootEntry *entry = setup_root()->list_ptr.next;
	for(;entry != setup_root(); entry = entry->list_ptr.next)
	{
		if(entry == setup_current())
			framebuffer_setcolors(COLOR_BLACK, COLOR_WHITE);
		else
			framebuffer_setcolors(COLOR_WHITE, COLOR_BLACK);

		framebuffer_setloc(0, y);
		framebuffer_print_force(entry->title);
		y++;
	}

	if(setup_root() == setup_current())
		framebuffer_setcolors(COLOR_BLACK, COLOR_WHITE);
	else
		framebuffer_setcolors(COLOR_WHITE, COLOR_BLACK);

	framebuffer_setloc(0, y);
	framebuffer_print_force("Console");

	framebuffer_setloc(0, 47);
	framebuffer_setcolors(COLOR_WHITE, COLOR_BLACK);
	framebuffer_print_force(OPENIBOOT_VERSION_STR);
}

/**
 * menu_run.
 *
 * Checks for button presses and executes the appropriate task.
 *
 * @param _V nickp666 does not yet know why this is here.
 */
static void menu_run(uint32_t _V)
{
	while(TRUE)
	{
#ifdef BUTTONS_VOLDOWN
		if(buttons_is_pushed(BUTTONS_HOLD)
				|| !buttons_is_pushed(BUTTONS_VOLDOWN))
#else
		if(buttons_is_pushed(BUTTONS_HOLD))
#endif
		{
			setup_entry(setup_current()->list_ptr.next);

			menu_draw();

			udelay(200000);
		}

#ifdef BUTTONS_VOLUP
		if(!buttons_is_pushed(BUTTONS_VOLUP))
		{
			setup_entry(setup_current()->list_ptr.prev);

			menu_draw();

			udelay(200000);
		}
#endif

		if(buttons_is_pushed(BUTTONS_HOME))
		{
			framebuffer_setdisplaytext(TRUE);
			framebuffer_clear();

			// Special case for console
			if(setup_current() == setup_root())
				OpenIBootConsole();
			else if(setup_boot() < 0)
					continue;

			task_stop();
			return;
		}

		task_yield();
	}
}

/**
 * menu_main.
 *
 * Main openiboot menu initialisation routine, reads /boot/menu.lst if it exists.
 *
 */
void menu_main()
{
	task_init(&menu_task, "menu", TASK_DEFAULT_STACK_SIZE);

	init_modules();

	if(script_run_file("(0)/boot/menu.lst"))
	{
		bufferPrintf("menu.lst NOT FOUND - Switching to console...\n");
		OpenIBootConsole();
		return;
	}

	const char* sTempOS = nvram_getvar("opib-temp-os");
	if(sTempOS && *sTempOS)
	{
		nvram_setvar("opib-temp-os","");
		nvram_save();

		setup_title(sTempOS);
		setup_boot();
	}

	setup_entry(setup_default());
	
	pmu_set_iboot_stage(0);

    framebuffer_setdisplaytext(FALSE);
	framebuffer_clear();

	menu_draw();

	task_start(&menu_task, &menu_run, NULL);
}

/**
 * menu_init_boot.
 *
 * Clears the framebuffer and loads the openiboot main menu.
 *
 */
static void menu_init_boot()
{
	framebuffer_clear();
	bufferPrintf("Loading openiBoot...\n");

	OpenIBootMain = &menu_main;
}
MODULE_INIT_BOOT(menu_init_boot);

