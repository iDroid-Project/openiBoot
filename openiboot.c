/*
 * openiboot.c - ASCIItastic!
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
#include "platform.h"
#include "tasks.h"
#include "util.h"

mainfn_t OpenIBootMain = NULL;

void OpenIBootConsole()
{
	init_modules();
	bufferPrintf(	"  ___                   _ ____              _   \r\n"
					" / _ \\ _ __   ___ _ __ (_) __ )  ___   ___ | |_ \r\n"
					"| | | | '_ \\ / _ \\ '_ \\| |  _ \\ / _ \\ / _ \\| __|\r\n"
					"| |_| | |_) |  __/ | | | | |_) | (_) | (_) | |_ \r\n"
					" \\___/| .__/ \\___|_| |_|_|____/ \\___/ \\___/ \\__|\r\n"
					"      |_|                                       \r\n"
					"\r\n"
					"version: %s\r\n", OPENIBOOT_VERSION_STR);
	DebugPrintf("                    DEBUG MODE\r\n");
}

void OpenIBootStart()
{
	platform_init();
	init_setup();

	OpenIBootMain = &OpenIBootConsole;
	init_boot_modules();

	OpenIBootMain();

	tasks_run(); // Runs forever.
}

void OpenIBootShutdown()
{
	exit_modules();
	platform_shutdown();
}
