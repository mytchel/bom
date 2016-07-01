#ifndef __SYSCALLS_H
#define __SYSCALLS_H

#define SYSCALL_EXIT	1
#define SYSCALL_FORK	2
#define SYSCALL_SLEEP	3
#define SYSCALL_GETPID	4

#define SYSCALL_PIPE	5
#define SYSCALL_CLOSE	6
#define SYSCALL_READ	7
#define SYSCALL_WRITE	8

#define NSYSCALLS	9

#define SYS_ERR		-1
#define SYS_ERRPC	-2

/* Fgroup copy, share, else new */
#define FORK_FC		(1<<0)
#define FORK_FS		(1<<1)

#endif
