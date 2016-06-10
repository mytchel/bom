#ifndef __PROC_MACHINE
#define __PROC_MACHINE

#define STACK_SIZE 256

struct proc_machine {
	uint32_t psr, sp, lr;
	uint32_t stack[STACK_SIZE];
};

void
proc_init_regs(struct proc_machine *p, 
	void (*func)(void *), 
	void *arg);

void
activate_proc(struct proc_machine *p);

#endif
