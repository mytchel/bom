#include "dat.h"
#include "../port/com.h"
#include "../port/syscall.h"
#include "../include/std.h"

static int
sysexit(int code)
{
	kprintf("pid %i exited with status %i\n", current->pid, code);
	
	current->state = PROC_exiting;
	schedule();
	
	/* Never reached. */
	return 1;
}

static int
sysyield(void)
{
	schedule();
	return 0;
}

static int
sysfork()
{
	struct proc *p;
		
	puts("fork\n");
	p = newproc();
	kprintf("created new process with pid %i\n", p->pid);

	forklabel(p, current->ureg);
	p->state = PROC_ready;
	
	return p->pid;
}

static int
sysgetpid(void)
{
	return current->pid;
}

static int
sysmount(struct fs *fs, char *path)
{
	kprintf("mount proc %i at %s\n", current->pid, path);

	while (1) {
		kprintf("mounted process %i rescheduling\n", current->pid);
		schedule();
		kprintf("mounted process %i back\n", current->pid);
	}
		
	return 0;
}

static int
sysopen(const char *path, int mode)
{
	kprintf("should open\n");
	return -1;
}

static int
sysclose(int fd)
{
	kprintf("should close '%i'\n", fd);
	
	return -1;
}

static int
sysstat(int fd)
{
	kprintf("should stat '%i'\n", fd);
	
	return -1;
}

static int
sysread(int fd, char *buf, size_t n)
{
	kprintf("should read\n");

	return -1;
}

static int
syswrite(int fd, char *buf, size_t n)
{
	kprintf("should write\n");

	return -1;
}

reg_t syscalltable[] = {
	[SYSCALL_EXIT] 		= (reg_t) sysexit,
	[SYSCALL_FORK] 		= (reg_t) sysfork,
	[SYSCALL_YIELD]		= (reg_t) sysyield,
	[SYSCALL_GETPID]	= (reg_t) sysgetpid,
	[SYSCALL_MOUNT] 	= (reg_t) sysmount,
	[SYSCALL_OPEN]		= (reg_t) sysopen,
	[SYSCALL_CLOSE]		= (reg_t) sysclose,
	[SYSCALL_STAT]		= (reg_t) sysstat,
	[SYSCALL_READ]		= (reg_t) sysread,
	[SYSCALL_WRITE]		= (reg_t) syswrite,
};
