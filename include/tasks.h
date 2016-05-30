#define MAX_TASKS 5
#define TASK_STACK_SIZE 256

struct task {
	uint32_t psr;
	uint32_t regs[15];
	uint32_t stack[TASK_STACK_SIZE];
};

extern struct task tasks[MAX_TASKS];
extern uint32_t current_task, task_count;

void activate(uint32_t *task_struct);
void add_task(void (*func)(void));

void scheduler();
void init_scheduler();

