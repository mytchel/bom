#include <stdarg.h>
#include <types.h>
#include <com.h>
#include <io.h>
#include <syscall.h>
#include <tasks.h>

void activate_task();

struct task task_queue[MAX_TASKS];

uint32_t current_task_pcb;

static uint32_t current_task, task_count;

void
add_task(void (*func)(void))
{
	int i;
	
	kprintf("add task %i\n", task_count);

	task_queue[task_count].psr = 16; /* user mode */
	for (i = 0; i < 15; i++) 
		task_queue[task_count].regs[i] = 0;

	task_queue[task_count].regs[13] = (uint32_t)
		&(task_queue[task_count].stack[TASK_STACK_SIZE - 1]);
	task_queue[task_count].regs[14] = (uint32_t) func;

	task_count++;
}

void task1_func(void);

void
scheduler(void)
{
	kprintf("activate next task: %i\n", current_task);
	current_task_pcb = (uint32_t) &(task_queue[current_task]);
	activate_task();
	
	current_task++;
	if (current_task == task_count)
		current_task = 0;
}

void
init_scheduler()
{
	task_count = 0;
	current_task = 0;
}
