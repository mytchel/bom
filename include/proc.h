#ifndef __PROC
#define __PROC

#define PROC_state_running		1
#define PROC_state_sleeping		2
#define PROC_state_exiting		3

struct proc {
	uint8_t state;
};

void
schedule(void);

struct proc *
proc_create(void (*func)(void *), void *arg);

#endif
