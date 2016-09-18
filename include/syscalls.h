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

#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#define SYSCALL_EXIT		1
#define SYSCALL_FORK		2
#define SYSCALL_SLEEP		3
#define SYSCALL_GETPID		4

#define SYSCALL_GETMEM		5
#define SYSCALL_RMMEM		6

#define SYSCALL_PIPE		7
#define SYSCALL_READ		8
#define SYSCALL_WRITE		9
#define SYSCALL_CLOSE		10

#define SYSCALL_BIND		11
#define SYSCALL_OPEN		12
#define SYSCALL_REMOVE		13

#define SYSCALL_WAITINTR	14
#define NSYSCALLS		15

#define OK		0
/* General error */
#define ERR		-1
/* Pipe connection closed somewhere along the line. */
#define ELINK		-2
/* Pipe/File mode wrong for operation. */
#define EMODE		-3
/* No such file. */
#define ENOFILE		-4
/* Function not implimented */
#define ENOIMPL		-5
/* No memory */
#define ENOMEM		-6
/* End of file */
#define EOF		-7

/* Should processor aspects be shared with the child
 * rather than copied. */
#define FORK_smem	(1<<0)
#define FORK_sfgroup	(1<<1)
#define FORK_sngroup	(1<<2)

#define O_RDONLY	(1<<0)
#define O_WRONLY	(1<<1)
#define O_RDWR		(O_RDONLY|O_WRONLY)
#define O_CREATE	(1<<2)

#endif
