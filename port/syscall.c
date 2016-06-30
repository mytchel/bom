#include "dat.h"
#include "../port/com.h"

static int
sysexit(va_list args)
{
	int code = va_arg(args, int);
	
	kprintf("pid %i exited with status %i\n", 
		current->pid, code);

	procremove(current);
	schedule();
	
	/* Never reached. */
	return 1;
}

static int
syssleep(va_list args)
{
	int ms;
	ms = va_arg(args, int);

	if (ms <= 0) {
		schedule();
		return 0;
	}

	procsleep(current, ms);
		
	return 0;
}

static int
sysfork(va_list args)
{
	int i;
	struct proc *p;
	

	p = newproc();
	if (p == nil)
		return -1;

	for (i = 0; i < Smax; i++) {
		if (current->segs[i] != nil) {
			p->segs[i] = copyseg(current->segs[i], true);
			if (p->segs[i] == nil)
				return -1;
		}
	}

	forkchild(p, current->ureg);
	
	p->parent = current;
	p->quanta = current->quanta;

	procready(p);
	return p->pid;
}

static int
sysgetpid(va_list args)
{
	return current->pid;
}

static int
sysmount(va_list args)
{
	kprintf("shoudl mount\n");
	return -1;
}

static int
sysopen(va_list args)
{
	kprintf("should open\n");
	return -1;
}

static int
sysclose(va_list args)
{
	kprintf("should close\n");
	return -1;
}

static int
sysstat(va_list args)
{
	kprintf("should stat\n");
	return -1;
}

static int
sysread(va_list args)
{
	kprintf("should read\n");
	return -1;
}

static int
syswrite(va_list args)
{
	kprintf("should write\n");
	return -1;
}

int (*syscalltable[NSYSCALLS])(va_list) = {
	[SYSCALL_EXIT] 		= sysexit,
	[SYSCALL_FORK] 		= sysfork,
	[SYSCALL_SLEEP]		= syssleep,
	[SYSCALL_GETPID]	= sysgetpid,
	[SYSCALL_MOUNT] 	= sysmount,
	[SYSCALL_OPEN]		= sysopen,
	[SYSCALL_CLOSE]		= sysclose,
	[SYSCALL_STAT]		= sysstat,
	[SYSCALL_READ]		= sysread,
	[SYSCALL_WRITE]		= syswrite,
};
