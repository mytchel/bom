#ifndef __PROC
#define __PROC

#include "proc_machine.h"

#define MAX_PROCS			1024
#define STACK_SIZE			1024

#define PROC_stopped		0
#define PROC_running		1
#define PROC_sleeping		2
#define PROC_exiting		3

struct proc {
	struct proc_machine machine;
	
	uint8_t state;
	uint32_t pid;
	uint32_t stack[STACK_SIZE];
	
	struct proc *next;
};

struct proc *
proc_create(void (*func)(void *), void *arg);

void
proc_init_regs(struct proc *p, 
	void (*func)(void *), void *arg);

#endif
