#include <machine/tasks.h>

#ifndef __TASKS
#define __TASKS

#define MAX_TASKS 5
#define TASK_STACK_SIZE 256

struct task {
	const char *name;
	uint32_t pid;
};

extern struct task tasks_queue[MAX_TASKS];

void add_task(void (*func)(void));

void tasks_init(void);

#endif
