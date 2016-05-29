#include <stdarg.h>
#include <types.h>
#include <com.h>
#include <io.h>
#include <syscall.h>

uint32_t* activate(uint32_t *);

#define STACK_SIZE 256
#define TASK_LIMIT 4

void task1_func(void)
{
	uart_puts("task1 return to kernel\n");
	syscall();
	while (1) {
		uart_puts("task1 running...\n");
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
		uart_puts("got: ");
		uart_putc(c);
		uart_putc('\n');
		syscall();
	}
}

uint32_t *
create_task(uint32_t *stack, void (*func)(void))
{
	stack += STACK_SIZE - 10;
	stack[9] = (uint32_t) func;

	return activate(stack);
}

int
main(void)
{
	kprintf("This is '%s' a test i: %i h: %h u: %u\n", "not", -43, 43, 43);
	
	uint32_t *user_tasks[TASK_LIMIT];
	uint32_t user_stacks[TASK_LIMIT][STACK_SIZE];
	uint32_t current_task, task_count;

	user_tasks[0] = create_task(user_stacks[0], &task1_func);
	user_tasks[1] = create_task(user_stacks[1], &task2_func);

	task_count = 2;
	current_task = 0;

	uart_puts("tasks initiated\n");

	while (1) {
		uart_puts("activate next task\n");
		user_tasks[current_task] = activate(user_tasks[current_task]);
		uart_puts("back to kernel\n");
		
//		current_task++;
		if (current_task == task_count)
			current_task = 0;

	}
}
