#include "dat.h"
#include "../port/com.h"
#include "../port/syscall.h"
#include "../include/std.h"

static struct mount *mount;

static int
sys_exit(int code)
{
	kprintf("%i exited with status %i\n", current->pid, code);
	proc_remove(current);
	schedule();
	/* Never reached. */
	return 1;
}

static int
sys_fork(void)
{
	kprintf("should fork\n");
	return -1;
}

static int
sys_getpid(void)
{
	return current->pid;
}

static int
sys_open(const char *path, int mode)
{
	kprintf("should open '%s' with mode %i\n", path, mode);
	return -1;
}

static int
sys_close(int fd)
{
	kprintf("should close '%i'\n", fd);
	return -1;
}

static int
sys_read(int fd, char *buf, size_t n)
{
	kprintf("should read but instead rescheduling %i\n", current->pid);
	
	reschedule();
	
	kprintf("and back in kernel for %i\n", current->pid);
	
	return -1;
}

static int
sys_write(int fd, char *buf, size_t n)
{
	kprintf("should write\n");
	return -1;
}

static int
sys_mount(char *path, struct mount *m)
{
	mount = m;
	return 0;
}

reg_t syscall_table[] = {
	(reg_t) sys_exit,
	(reg_t) sys_fork,
	(reg_t) sys_getpid,
	(reg_t) sys_open,
	(reg_t) sys_close,
	(reg_t) sys_read,
	(reg_t) sys_write,
	(reg_t) sys_mount,
};

