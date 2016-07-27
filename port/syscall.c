#include "dat.h"

reg_t (*syscalltable[NSYSCALLS])(va_list) = {
	[SYSCALL_EXIT] 		= sysexit,
	[SYSCALL_FORK] 		= sysfork,
	[SYSCALL_SLEEP]		= syssleep,
	[SYSCALL_GETPID]	= sysgetpid,
	
	[SYSCALL_GETMEM]	= sysgetmem,
	[SYSCALL_RMMEM]		= sysrmmem,
	
	[SYSCALL_PIPE]		= syspipe,
	[SYSCALL_READ]		= sysread,
	[SYSCALL_WRITE]		= syswrite,
	[SYSCALL_CLOSE]		= sysclose,
	
	[SYSCALL_BIND]		= sysbind,
	[SYSCALL_OPEN]		= sysopen,
	[SYSCALL_REMOVE]	= sysremove,
};
