#include <stdarg.h>
#include <types.h>
#include <com.h>
#include <io.h>
#include <syscall.h>
#include <tasks.h>

struct task tasks[MAX_TASKS];

uint32_t current_task, task_count;

void
add_task(void (*func)(void))
{
	int i;
	
	asm("cpsid if"); /* disable interrupts */
	kprintf("add task %i\n", task_count);

	tasks[task_count].psr = 16; /* user mode */
	for (i = 0; i < 15; i++) 
		tasks[task_count].regs[i] = 0;

	tasks[task_count].regs[13] = (uint32_t)
		&(tasks[task_count].stack[TASK_STACK_SIZE - 1]);
	tasks[task_count].regs[14] = (uint32_t) func;

	task_count++;
	
	asm("cpsie if"); /* enable interrupts */
}

void task1_func(void);

void
scheduler(void)
{
	current_task = 0;
	while (task_count > 0) {
		kprintf("activate next task: %i\n", current_task);
		activate((uint32_t *) &(tasks[current_task]));
		
		current_task++;
		if (current_task == task_count)
			current_task = 0;

	}
}

void
init_scheduler()
{
	task_count = 0;
}
