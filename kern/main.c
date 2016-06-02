#include <stdarg.h>
#include <types.h>
#include <com.h>
#include <io.h>
#include <syscall.h>
#include <tasks.h>

static void
task1_func(void)
{
	int i = 0, j;
	kprintf("task1 started\n");
	while (1) {
		for (j = 0; j < 1000; j++);
		kprintf("task 1 i = %i\n", i++);
	}
}

static void
task2_func(void)
{
	int i = 0;
	
	kprintf("task2 started\n");
	while (1) {
		kprintf("task2 %i\n", i++);
	}
}

void
main_setup(void)
{
	kprintf("in main\n");
	
	tasks_init();
	
	kprintf("adding tasks\n");
	
	add_task(&task1_func);
	add_task(&task2_func);

	kprintf("tasks initiated\n");
}
