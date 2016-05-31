#ifndef __SYSCALL
#define __SYSCALL

#define SYSCALL_exit	0
#define SYSCALL_sleep	1
#define SYSCALL_putc	2
#define SYSCALL_getc	3
#define SYSCALL_fork	4
#define SYSCALL_MAX	5

extern uint32_t syscall_table[SYSCALL_MAX];

uint32_t syscall(uint32_t);

#endif
