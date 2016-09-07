#ifndef __SYSCALLS_H
#define __SYSCALLS_H

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
