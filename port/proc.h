#ifndef __PROC
#define __PROC

#include "proc_regs.h"

#define MAX_PROCS		512
#define STACK_SIZE		1024

#define PROC_stopped		0
#define PROC_running		1
#define PROC_sleeping		2

struct proc {
	struct proc_regs regs;
	
	int state;
	int pid;
	reg_t stack[STACK_SIZE];
	
	struct page *page;

	struct proc *next;
};

extern struct proc *current;
extern struct proc_regs *user_regs;

struct proc *
proc_create(void (*func)(void *), void *arg);

void
proc_remove(struct proc *);

void
proc_init_regs(struct proc *p, 
	void (*func)(void *), void *arg);

void
schedule(void);

#endif
