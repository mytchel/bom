#ifndef __MACHINE_TASKS
#define __MACHINE_TASKS

#define MAX_TASKS 5
#define TASK_STACK_SIZE 256

struct machine_task {
	uint32_t psr;
	uint32_t regs[15];
	uint32_t stack[TASK_STACK_SIZE];

	struct task *task;
};

extern uint32_t current_task_ptr;

void machine_tasks_init();
void machine_tasks_next();

void machine_tasks_add(void (*func)(void), struct task *);

#endif
