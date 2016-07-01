#include "dat.h"

int (*syscalltable[NSYSCALLS])(va_list) = {
	[SYSCALL_EXIT] 		= sysexit,
	[SYSCALL_FORK] 		= sysfork,
	[SYSCALL_SLEEP]		= syssleep,
	[SYSCALL_GETPID]	= sysgetpid,
	
	[SYSCALL_PIPE]		= syspipe,
	[SYSCALL_CLOSE]		= sysclose,
	[SYSCALL_READ]		= sysread,
	[SYSCALL_WRITE]		= syswrite,
};
