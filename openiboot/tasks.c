#include "tasks.h"
#include "openiboot-asmhelpers.h"
#include "util.h"

const TaskDescriptor bootstrapTaskInit = {
	TaskDescriptorIdentifier1,
	{0, 0},
	{0, 0},
	TASK_RUNNING,
	1,
	{0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
	{{0, 0}, 0, 0, 0, 0},
	{0, 0},
	0,
	0,
	0,
	0,
	0,
	"bootstrap",
	TaskDescriptorIdentifier2
	};

static TaskDescriptor bootstrapTask;

/*static void task_add_after(TaskDescriptor *_a, TaskDescriptor *_b)
{
	EnterCriticalSection();

	TaskDescriptor *prev = _b;
	TaskDescriptor *next = _b->taskList.next;

	prev->taskList.next = _a;
	_a->taskList.prev = prev;

	next->taskList.prev = _a;
	_a->taskList.next = next;

	LeaveCriticalSection();
}*/

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

	LeaveCriticalSection();
}

int tasks_setup()
{
	memcpy(&bootstrapTask, &bootstrapTaskInit, sizeof(TaskDescriptor));
	bootstrapTask.taskList.next = &bootstrapTask;
	bootstrapTask.taskList.prev = &bootstrapTask;
	CurrentRunning = &bootstrapTask;
	return 0;
}

void task_init(TaskDescriptor *_td, char *_name)
{
	memset(_td, 0, sizeof(TaskDescriptor));
	_td->identifier1 = TaskDescriptorIdentifier1;
	_td->identifier2 = TaskDescriptorIdentifier2;
	_td->state = TASK_STOPPED;
	strcpy(_td->taskName, _name);
	_td->storage = malloc(TASK_STACK_SIZE);
	_td->storageSize = TASK_STACK_SIZE;

	bufferPrintf("Tasks: Initialized %s.\n", _td->taskName);
}

void task_destroy(TaskDescriptor *_td)
{
	if(_td->storage)
		free(_td->storage);
}

int task_start(TaskDescriptor *_td, void *_fn, void *_arg)
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

		task_add_before(_td, &bootstrapTask);
		
		LeaveCriticalSection();

		//bufferPrintf("Tasks: Added %s to tasks.\n", _td->taskName);
		return 1;
	}
		
	LeaveCriticalSection();

	return 0;
}

void task_stop()
{
	EnterCriticalSection();

	TaskDescriptor *next = CurrentRunning->taskList.next;
	if(next == CurrentRunning)
	{
		LeaveCriticalSection();
		bufferPrintf("Tasks: Cannot stop last task! Expect hell to break loose now!\n");
		return;
	}

	// Remove us from the queue
	task_remove(CurrentRunning);
	CurrentRunning->state = TASK_STOPPED;

	// Swap onto next task
	SwapTask(next);
	
	// Can't ever reach here, code will continue in task_yield after
	// the call to SwapTask... -- Ricky26
}

int task_yield()
{
	EnterCriticalSection();
	TaskDescriptor *next = CurrentRunning->taskList.next;
	if(next != CurrentRunning)
	{
		// We have another thread to schedule! -- Ricky26
		TaskDescriptor *td = (TaskDescriptor*)next;
		//bufferPrintf("Tasks: Swapping from %s to %s.\n", CurrentRunning->taskName, td->taskName);
		SwapTask(td);
		//bufferPrintf("Tasks: Swapped from %s to %s.\n", CurrentRunning->taskName, td->taskName);
		LeaveCriticalSection();
		return 1;
	}
	LeaveCriticalSection();

	return 0; // didn't yield
}

void tasks_run()
{
	while(1)
	{
		if(!task_yield())
		{
			//__asm("wfi"); // Nothing to do, wait for an interrupt.
			//WaitForInterrupt();
		}
	}
}
