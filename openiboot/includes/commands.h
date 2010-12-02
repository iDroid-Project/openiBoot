#ifndef COMMANDS_H
#define COMMANDS_H

#include "openiboot.h"

typedef void (*OPIBCommandRoutine)(int argc, char** argv);

typedef struct OPIBCommand {
	char* name;
	char* description;
	OPIBCommandRoutine routine;
} OPIBCommand;

typedef OPIBCommand **OIBCommandIterator;

extern OPIBCommand *command_list_init;
OPIBCommand *command_get_next(OIBCommandIterator*);
int command_run(int argc, char **argv);

#define COMMAND(_name, _desc, _fn) OPIBCommand _fn##_struct = { \
		.name = _name, \
		.description = _desc, \
		.routine = &_fn, \
	}; \
	OPIBCommand *_fn##_structptr __attribute__((section(".commands"))) = &_fn##_struct;

#endif
