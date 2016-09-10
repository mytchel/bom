/*
 *   Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "head.h"

reg_t (*syscalltable[NSYSCALLS])(va_list) = {
	[SYSCALL_EXIT] 		= sysexit,
	[SYSCALL_FORK] 		= sysfork,
	[SYSCALL_SLEEP]		= syssleep,
	[SYSCALL_GETPID]	= sysgetpid,
	
	[SYSCALL_GETMEM]	= sysgetmem,
	[SYSCALL_RMMEM]		= sysrmmem,
	[SYSCALL_WAITINTR]	= syswaitintr,
	
	[SYSCALL_PIPE]		= syspipe,
	[SYSCALL_READ]		= sysread,
	[SYSCALL_WRITE]		= syswrite,
	[SYSCALL_CLOSE]		= sysclose,
	
	[SYSCALL_BIND]		= sysbind,
	[SYSCALL_OPEN]		= sysopen,
	[SYSCALL_REMOVE]	= sysremove,
};
