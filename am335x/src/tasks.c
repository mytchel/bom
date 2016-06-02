#include <stdarg.h>
#include <com.h>
#include <io.h>
#include <syscall.h>
#include <tasks.h>
#include <machine/types.h>
#include <machine/tasks.h>

void activate_task();

uint32_t current_task_ptr;

static struct machine_task machine_tasks[MAX_TASKS];

static uint32_t task_count, current_task;

void
machine_tasks_init()
{
	task_count = 0;
	current_task = 0;
	machine_tasks_next();
}

void
machine_tasks_next(struct task *current)
{
	kprintf("go to next task\n");

	current_task++;
	if (current_task >= task_count)
		current_task = 0;
	
	current_task_ptr = (uint32_t) &machine_tasks[current_task];
}

void
machine_tasks_add(void (*func)(void), struct task *task)
{
	struct machine_task * mt = &machine_tasks[task_count];
	
	mt->task = task;
	
	mt->psr = 16; /* user mode */

	mt->regs[13] = (uint32_t)
		&(mt->stack[TASK_STACK_SIZE - 1]);
	mt->regs[14] = (uint32_t) func + 4;
	
	task_count++;
}

