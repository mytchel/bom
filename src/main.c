#include <stdarg.h>
#include <types.h>
#include <com.h>
#include <io.h>
#include <syscall.h>
#include <tasks.h>

void task1_func(void)
{
	int i = 0;
	uart_puts("task1 return to kernel\n");
	syscall();
	while (1) {
		kprintf("task 1 i = %i\n", i++);
		syscall();
	}
}

void task2_func(void)
{
	char c;
	
	uart_puts("task2 return to kernel\n");
	syscall();
	while (1) {
		uart_puts("waiting: ");
		c = uart_getc();	
		kprintf("%c\n", c);
		syscall();
	}
}

int
main(void)
{
	init_scheduler();
	
	kprintf("adding tasks\n");
	
	add_task(&task1_func);
	add_task(&task2_func);

	kprintf("tasks initiated\n");

	scheduler();

	return 0;
}
