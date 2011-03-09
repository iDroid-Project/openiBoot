#ifndef TASKS_H
#define TASKS_H

#include "openiboot.h"

#define TASK_STACK_SIZE (1*1024*1024) // 1MB

int tasks_setup();
void tasks_run();

void task_init(TaskDescriptor *_td, char *_name);
void task_destroy(TaskDescriptor *_td);
int task_start(TaskDescriptor *_td, void *_fn, void *_arg);
void task_stop();

int task_yield();
int task_sleep(int _ms);

void task_wait();
int task_wait_timeout(int _ms);
void task_wake(TaskDescriptor *_task);

#endif
