#include <stdarg.h>
#include <types.h>
#include <com.h>
#include <io.h>
#include <syscall.h>
#include <tasks.h>

static void
task1_func(void)
{
	int i = 0;
	kprintf("task1 return to kernel\n");
	while (1) {
		kprintf("task 1 i = %i\n", i++);
	}
}

static void
task2_func(void)
{
	char c;
	
	kprintf("task2 return to kernel\n");
	while (1) {
		kprintf("waiting: ");
		c = (char) syscall(SYSCALL_getc);
		kprintf("%c\n", c);
	}
}

int
main(void)
{
	kprintf("in main\n");
	
	init_scheduler();
	
	kprintf("adding tasks\n");
	
	add_task(&task1_func);
	add_task(&task2_func);

	kprintf("tasks initiated\n");

	while (1) {
		scheduler();
	}

	return 0;
}
