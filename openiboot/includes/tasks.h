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
