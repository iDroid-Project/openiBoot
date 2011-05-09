/*
 * commands.c - OpeniBoot base commands
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
#include "arm/arm.h"
#include "commands.h"
#include "util.h"

int received_file_size; // Makes commands automatically use output from last command.

OPIBCommand *command_get_next(OIBCommandIterator *_it)
{
	if(*_it == NULL)
		*_it = &command_list_init;

	(*_it)++;
	return **_it;
}

char **command_parse(char *str, int *argc)
{
	while((*str == '\t') 
		||(*str == ' '))
		str++;

	int len = strlen(str);
	char *end = str + len;
	char *ptr = str;
	int quote = 0;
	while(ptr < end)
	{
		if(*ptr == '"')
			quote = !quote;
		else if(*ptr == '#' && !quote)
		{
			*ptr = 0;
			break;
		}

		ptr++;
	}

	return tokenize(str, argc);	
}

int command_run(int argc, char **argv)
{
	OIBCommandIterator cmdIt = NULL;
	OPIBCommand *cmd;

	if(*argv[0] == 0)
		return 0; // Hack to prevent empty lines erroring out.

	while((cmd = command_get_next(&cmdIt)))
	{
		if(strcmp(argv[0], cmd->name) == 0) {
			cmd->routine(argc, argv);
			return 0;
		}
	}

	return -1;
}

void cmd_help(int argc, char** argv)
{
	OIBCommandIterator it = NULL;
	OPIBCommand *cmd;
	while((cmd = command_get_next(&it)))
        bufferPrintf("%-20s%s\r\n", cmd->name, cmd->description);
}
COMMAND("help", "list the available commands", cmd_help);

void cmd_reboot(int argc, char** argv) {
	Reboot();
}
COMMAND("reboot", "reboot the device", cmd_reboot);

void cmd_md(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <len>\r\n", argv[0]);
		return;
	}

	uint32_t address = parseNumber(argv[1]);
	uint32_t len = parseNumber(argv[2]);
	bufferPrintf("dumping memory 0x%x - 0x%x\r\n", address, address + len);
	buffer_dump_memory(address, len);
}
COMMAND("md", "display a block of memory as 32-bit integers", cmd_md);

void cmd_hexdump(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <len>\r\n", argv[0]);
		return;
	}

	uint32_t address = parseNumber(argv[1]);
	uint32_t len = parseNumber(argv[2]);
	bufferPrintf("dumping memory 0x%x - 0x%x\r\n", address, address + len);
	hexdump((void*)address, len);
}
COMMAND("hexdump", "display a block of memory like 'hexdump -C'", cmd_hexdump);

void cmd_cat(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <len>\r\n", argv[0]);
		return;
	}

	uint32_t address = parseNumber(argv[1]);
	uint32_t len = parseNumber(argv[2]);
	addToBuffer((char*) address, len);
}
COMMAND("cat", "dumps a block of memory", cmd_cat);

void cmd_mwb(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <data>\r\n", argv[0]);
		return;
	}

	uint8_t* address = (uint8_t*) parseNumber(argv[1]);
	uint8_t data = parseNumber(argv[2]);
	*address = data;
	bufferPrintf("Written to 0x%x to 0x%x\r\n", (uint8_t)data, address);
}
COMMAND("mwb", "write a byte into a memory address", cmd_mwb);

void cmd_mws(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <string>\r\n", argv[0]);
		return;
	}

	char* address = (char*) parseNumber(argv[1]);
	strcpy(address, argv[2]);
	bufferPrintf("Written %s to 0x%x\r\n", argv[2], address);
}
COMMAND("mws", "write a string into a memory address", cmd_mws);

void cmd_mw(int argc, char** argv) {
	if(argc < 3) {
		bufferPrintf("Usage: %s <address> <data>\r\n", argv[0]);
		return;
	}

	uint32_t* address = (uint32_t*) parseNumber(argv[1]);
	uint32_t data = parseNumber(argv[2]);
	*address = data;
	bufferPrintf("Written to 0x%x to 0x%x\r\n", data, address);
}
COMMAND("mw", "write a 32-bit dword into a memory address", cmd_mw);

void cmd_echo(int argc, char** argv) {
	int i;
	for(i = 1; i < argc; i++) {
		bufferPrintf("%s ", argv[i]);
	}
	bufferPrintf("\r\n");
}
COMMAND("echo", "echo back a line", cmd_echo);

void cmd_version(int argc, char** argv) {
	bufferPrintf("%s\r\n", OPENIBOOT_VERSION_STR);
}
COMMAND("version", "display the version string", cmd_version);

