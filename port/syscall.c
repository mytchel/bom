#include "dat.h"
#include "../port/com.h"
#include "../port/syscall.h"

static void
sys_exit(int code)
{
	kprintf("%i exited with status %i\n", current->pid, code);
	proc_remove(current);
	schedule();
}

static int
sys_fork(void)
{
	kprintf("should fork\n");
	return -1;
}

static int
sys_open(const char *path, int mode)
{
	kprintf("should open '%s' with mode %i\n", path, mode);
	return -1;
}

static void
sys_close(int fd)
{
	kprintf("should close '%i'\n", fd);
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

reg_t syscall_table[] = {
	(reg_t) sys_exit,
	(reg_t) sys_fork,
	(reg_t) sys_open,
	(reg_t) sys_close,
	(reg_t) sys_read,
	(reg_t) sys_write,
};

