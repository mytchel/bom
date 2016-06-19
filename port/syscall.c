#include "types.h"
#include "../include/com.h"
#include "../include/proc.h"

static void
sys_exit(int code)
{
	kprintf("exiting\n");
	proc_remove(current);
	schedule();
}

static int
sys_open(const char *path, int mode)
{
	kprintf("should open\n");
	return -1;
}

static int
sys_read(int fd, char *buf, size_t n)
{
	kprintf("should read\n");
	return 0;
}

static int
sys_write(int fd, char *buf, size_t n)
{
	kprintf("should write\n");
	return 0;
}

static int
sys_fork(void)
{
	kprintf("should fork\n");
	return -1;
}

reg_t syscall_table[] = {
	(reg_t) sys_exit,
	(reg_t) sys_open,
	(reg_t) sys_read,
	(reg_t) sys_write,
	(reg_t) sys_fork,
};

