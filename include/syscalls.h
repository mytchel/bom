#ifndef __SYSCALLS_H
#define __SYSCALLS_H

#define SYSCALL_EXIT	1
#define SYSCALL_FORK	2
#define SYSCALL_SLEEP	3
#define SYSCALL_GETPID	4

#define SYSCALL_PIPE	5
#define SYSCALL_READ	6
#define SYSCALL_WRITE	7
#define SYSCALL_CLOSE	8

#define SYSCALL_BIND	9
#define SYSCALL_OPEN	10

#define NSYSCALLS	11

/* General error, either args or memory alloc failed. */
#define ERR		-1

/* Pipe connection closed or in incompatable state.
 * eg: pipe link is reading when attempting to read. */
#define ELINK		-2

/* Should processor aspects be copyied to child
 * rather than shared with child. */
#define FORK_cmem	(1<<0)
#define FORK_cfgroup	(1<<1)
#define FORK_cngroup	(1<<2)

/* Opening file in read or write mode. */
/* You can not open it in RW (for now?). */
#define O_RDONLY	(1<<0)
#define O_WRONLY	(1<<1)
#define O_RDWR		(1<<2)
#define O_CREATE	(1<<2)

#endif
