#ifndef __PROC
#define __PROC

#define MAX_PROCS			1024

#define PROC_state_running		1
#define PROC_state_sleeping		2
#define PROC_state_exiting		3

struct proc {
	uint8_t state;
};

struct proc *
proc_create(void (*func)(void *), void *arg);

#endif
