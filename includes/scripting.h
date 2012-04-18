/*
 * scripting.h
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

#ifndef SCRIPTING_H
#define SCRIPTING_H

uint8_t *script_load_hfs_file(int disk, int part, char *path, uint32_t *size);
uint8_t *script_load_file(char *id, uint32_t *size);
char **script_split_file(char *_data, uint32_t _sz, uint32_t *_count);
int script_run_command(char* command);
int script_run_commands(char** cmds, uint32_t count);
int script_run_file(char *path);

#endif
