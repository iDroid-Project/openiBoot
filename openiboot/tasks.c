#include "openiboot.h"
#include "arm/arm.h"
#include "tasks.h"
#include "event.h"
#include "clock.h"
#include "util.h"
#include "timer.h"

static TaskDescriptor bootstrapTask;
static TaskDescriptor irqTask;

TaskDescriptor *IRQTask = NULL;
TaskDescriptor *IRQBackupTask = NULL;

static void task_add_after(TaskDescriptor *_a, TaskDescriptor *_b)
{
	EnterCriticalSection();

	TaskDescriptor *prev = _b;
	TaskDescriptor *next = _b->taskList.next;

	prev->taskList.next = _a;
	_a->taskList.prev = prev;

	next->taskList.prev = _a;
	_a->taskList.next = next;

	LeaveCriticalSection();
}

static void task_add_before(TaskDescriptor *_a, TaskDescriptor *_b)
{
	EnterCriticalSection();

	TaskDescriptor *prev = _b->taskList.prev;
	TaskDescriptor *next = _b;

	prev->taskList.next = _a;
	_a->taskList.prev = prev;

	next->taskList.prev = _a;
	_a->taskList.next = next;

	LeaveCriticalSection();
}

static void task_remove(TaskDescriptor *_t)
{
	EnterCriticalSection();

	TaskDescriptor *prev = _t->taskList.prev;
	TaskDescriptor *next = _t->taskList.next;

	prev->taskList.next = next;
	next->taskList.prev = prev;

	_t->taskList.next = _t;
	_t->taskList.prev = _t;

	LeaveCriticalSection();
}

void tasks_setup()
{
	task_init(&bootstrapTask, "bootstrap", 0);
	bootstrapTask.state = TASK_RUNNING;

	task_init(&irqTask, "irq", 0);
	irqTask.state = TASK_RUNNING;

	IRQTask = &irqTask;
	IRQBackupTask = NULL;
	CurrentRunning = &bootstrapTask;
}

void task_init(TaskDescriptor *_td, char *_name, size_t _stackSize)
{
	memset(_td, 0, sizeof(TaskDescriptor));

	_td->identifier1 = TaskDescriptorIdentifier1;
	_td->identifier2 = TaskDescriptorIdentifier2;
	strcpy(_td->taskName, _name);

	_td->criticalSectionNestCount = 1;
	_td->state = TASK_STOPPED;

	_td->storageSize = _stackSize;
	_td->storage = _stackSize > 0? malloc(_stackSize): NULL;

	_td->taskList.next = _td;
	_td->taskList.prev = _td;

	bufferPrintf("tasks: Initialized %s.\n", _td->taskName);
}

void task_destroy(TaskDescriptor *_td)
{
	if(_td->storage)
		free(_td->storage);
}

void task_run(void (*_fn)(void*), void *_arg)
{
	LeaveCriticalSection();

	//bufferPrintf("tasks: New task started %s. 0x%08x(0x%08x).\r\n",
	//		CurrentRunning->taskName, _fn, _arg);

	_fn(_arg);

	//bufferPrintf("tasks: Task ending %s. 0x%08x(0x%08x).\r\n",
	//		CurrentRunning->taskName, _fn, _arg);

	EnterCriticalSection();
	task_stop();
}

error_t task_start(TaskDescriptor *_td, void *_fn, void *_arg)
{
	EnterCriticalSection();

	if(_td->state == TASK_STOPPED)
	{
		_td->criticalSectionNestCount = 1;

		_td->savedRegisters.r4 = (uint32_t)_fn;
		_td->savedRegisters.r5 = (uint32_t)_arg;
		_td->savedRegisters.lr = (uint32_t)&StartTask;
		_td->savedRegisters.sp = (uint32_t)(_td->storage + _td->storageSize);

		_td->state = TASK_RUNNING;

		if(CurrentRunning == IRQTask)
			task_add_after(_td, IRQBackupTask);
		else
		{
			task_add_before(_td, CurrentRunning);
			SwapTask(_td);
		}

		LeaveCriticalSection();

		//bufferPrintf("tasks: Added %s to tasks.\n", _td->taskName);
		return SUCCESS;
	}
		
	LeaveCriticalSection();

	return EINVAL;
}

void task_stop()
{
	EnterCriticalSection();

	if(CurrentRunning == IRQTask)
	{
		LeaveCriticalSection();

		bufferPrintf("tasks: Cannot stop IRQ task.\r\n");
		return;
	}

	TaskDescriptor *next = CurrentRunning->taskList.next;
	if(next == CurrentRunning)
	{
		LeaveCriticalSection();
		bufferPrintf("tasks: Cannot stop last task! Expect hell to break loose now!\n");
		return;
	}

	// Remove us from the queue
	CurrentRunning->state = TASK_STOPPED;
	task_remove(CurrentRunning);

	//bufferPrintf("tasks: %s died, swapping to %s.\r\n",
	//		CurrentRunning->taskName, next->taskName);

	// Swap onto next task
	SwapTask(next);
	
	// Can't ever reach here, code will continue in task_yield after
	// the call to SwapTask... -- Ricky26
}

error_t task_yield()
{
	EnterCriticalSection();
	if(CurrentRunning == IRQTask)
	{
		LeaveCriticalSection();

		bufferPrintf("tasks: Can't swap during ISR!\r\n");
		return EINVAL;
	}

	TaskDescriptor *next = CurrentRunning->taskList.next;
	if(next != CurrentRunning)
	{
		// We have another thread to schedule! -- Ricky26
		
		//bufferPrintf("tasks: Swapping from %s to %s.\n", CurrentRunning->taskName, next->taskName);
		SwapTask(next);
		//bufferPrintf("tasks: Swapped from %s to %s.\n", CurrentRunning->taskName, next->taskName);

		LeaveCriticalSection();
		return SUCCESS_VALUE(1);
	}

	LeaveCriticalSection();
	return SUCCESS_VALUE(0); // didn't yield
}

void tasks_run()
{
	while(1)
	{
		if(!task_yield())
		{
			//bufferPrintf("WFI\r\n");
			WaitForInterrupt(); // Nothing to do, wait for an interrupt.
		}
	}
}

void task_wake_event(Event *_evt, void *_obj)
{
	TaskDescriptor *task = _obj;
	//bufferPrintf("tasks: Resuming sleeping task %p.\n", task);
	
	if(CurrentRunning == IRQTask)
		task_add_before(task, IRQBackupTask);
	else
		task_add_before(task, CurrentRunning); // Add us back to the queue.
}

error_t task_sleep(int _ms)
{
	EnterCriticalSection();

	if(CurrentRunning == IRQTask)
	{
		LeaveCriticalSection();

		bufferPrintf("tasks: You can't suspend an ISR.\r\n");
		return EINVAL;
	}

	TaskDescriptor *next = CurrentRunning->taskList.next;
	if(next == CurrentRunning)
	{
		LeaveCriticalSection();
		
		bufferPrintf("tasks: Last thread cannot sleep!\n");

		uint64_t start = timer_get_system_microtime();
		while(!has_elapsed(start, _ms*1000));
		return SUCCESS;
	}

	uint32_t ticks = _ms * 1000;
	TaskDescriptor *task = CurrentRunning;

	//bufferPrintf("tasks: Putting task %p to sleep for %d ms.\n", task, _ms);
	task_remove(task);
	event_add(&task->sleepEvent, ticks, &task_wake_event, task);
	SwapTask(next);

	LeaveCriticalSection();

	return SUCCESS;
}

void task_suspend()
{
	EnterCriticalSection();

	if(CurrentRunning == IRQTask)
	{
		LeaveCriticalSection();

		bufferPrintf("tasks: You can't suspend an ISR.\r\n");
		return;
	}

	TaskDescriptor *next = CurrentRunning->taskList.next;
	if(next == CurrentRunning)
	{
		LeaveCriticalSection();

		bufferPrintf("tasks: Last task cannot be suspended!\r\n");
		return;
	}

	task_remove(CurrentRunning);
	SwapTask(next);
	LeaveCriticalSection();
}

void task_wait()
{
	CurrentRunning->wasWoken = 0;
	task_suspend();
}

error_t task_wait_timeout(int _ms)
{
	CurrentRunning->wasWoken = 0;
	task_sleep(_ms);

	return SUCCESS_VALUE(CurrentRunning->wasWoken);
}

void task_wake(TaskDescriptor *_task)
{
	EnterCriticalSection();
	_task->wasWoken = 1;
	task_add_after(_task, CurrentRunning);
	SwapTask(_task);
	LeaveCriticalSection();
}
