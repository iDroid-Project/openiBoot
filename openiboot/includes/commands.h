/**
 * commands.h
 *
 * Copyright 2011 iDroid Project
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

#ifndef COMMANDS_H
#define COMMANDS_H

#include "openiboot.h"

typedef error_t (*OPIBCommandRoutine)(int argc, char** argv);

typedef struct OPIBCommand {
	char* name;
	char* description;
	OPIBCommandRoutine routine;
} OPIBCommand;

typedef OPIBCommand **OIBCommandIterator;

extern OPIBCommand *command_list_init;
OPIBCommand *command_get_next(OIBCommandIterator*);
char **command_parse(char *str, int *argc);
int command_run(int argc, char **argv);

#define COMMAND(_name, _desc, _fn) OPIBCommand _fn##_struct = { \
		.name = _name, \
		.description = _desc, \
		.routine = &_fn, \
	}; \
	static OPIBCommand *_fn##_structptr __attribute__((section(".commands"))) = &_fn##_struct;

#endif
