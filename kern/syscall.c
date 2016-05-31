#include <com.h>
#include <types.h>
#include <syscall.h>

static void
task_exit()
{
	kprintf("exit\n");
}

static void
task_sleep()
{
	kprintf("sleep\n");
}

static void
task_putc(uint32_t i)
{
	char c = (char) i;
	putc(c);
}

static uint32_t
task_getc()
{
	char c = getc();
	return (uint32_t) c;
}

static uint32_t
task_fork()
{
	kprintf("fork?\n");
	return 0;
}

uint32_t syscall_table[SYSCALL_MAX] = {
	[SYSCALL_exit]	= (uint32_t) &task_exit,
	[SYSCALL_sleep]	= (uint32_t) &task_sleep,
	[SYSCALL_putc]	= (uint32_t) &task_putc,
	[SYSCALL_getc]	= (uint32_t) &task_getc,
	[SYSCALL_fork]	= (uint32_t) &task_fork,
};
