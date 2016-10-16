/*
 *
 * Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "head.h"

reg_t (*syscalltable[NSYSCALLS])(va_list) = {
	[SYSCALL_EXIT] 		= sysexit,
	[SYSCALL_FORK] 		= sysfork,
	[SYSCALL_SLEEP]		= syssleep,
	[SYSCALL_GETPID]	= sysgetpid,
	[SYSCALL_WAIT]	        = syswait,

	[SYSCALL_CHDIR]	        = syschdir,
	
	[SYSCALL_GETMEM]	= sysgetmem,
	[SYSCALL_RMMEM]		= sysrmmem,
	[SYSCALL_WAITINTR]	= syswaitintr,
	
	[SYSCALL_PIPE]		= syspipe,
	[SYSCALL_READ]		= sysread,
	[SYSCALL_WRITE]		= syswrite,
	[SYSCALL_SEEK]		= sysseek,
	[SYSCALL_CLOSE]		= sysclose,
	
	[SYSCALL_STAT]		= sysstat,
	[SYSCALL_BIND]		= sysbind,
	[SYSCALL_OPEN]		= sysopen,
	[SYSCALL_REMOVE]	= sysremove,
	[SYSCALL_CLEANPATH]	= syscleanpath,
};