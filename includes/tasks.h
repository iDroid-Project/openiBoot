/*
 * tasks.h
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

#ifndef TASKS_H
#define TASKS_H

#include "openiboot.h"

#define TASK_DEFAULT_STACK_SIZE (1*1024*1024) // 1MB

void tasks_setup();
void tasks_run();

void task_init(TaskDescriptor *_td, char *_name, size_t _stack_size);
void task_destroy(TaskDescriptor *_td);
error_t task_start(TaskDescriptor *_td, void *_fn, void *_arg);
void task_stop();

error_t task_yield();
error_t task_sleep(int _ms);

void task_wait();
error_t task_wait_timeout(int _ms);
void task_wake(TaskDescriptor *_task);

#endif
