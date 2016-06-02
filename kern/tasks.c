#include <stdarg.h>
#include <types.h>
#include <com.h>
#include <io.h>
#include <syscall.h>
#include <tasks.h>
#include <machine/tasks.h>

struct task task_queue[MAX_TASKS];

static int task_count;

void
add_task(void (*func)(void))
{
	struct task *new;
	
	kprintf("add task %i\n", task_count);
	
	new = &task_queue[task_count];
	
	new->name = "test";
	new->pid = task_count;

	machine_tasks_add(func, new);
	
	task_count++;
}

void
tasks_init()
{
	machine_tasks_init();
	task_count = 0;
}
