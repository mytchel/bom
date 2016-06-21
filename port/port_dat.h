typedef uint8_t bool;

#define true 1
#define false 0

#define nil 0

void
enable_interrupts(void);

void
disable_interrupts(void);

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

struct proc *
proc_create(void (*func)(void *), void *arg);

void
proc_remove(struct proc *);

void
proc_init_regs(struct proc *p, 
	void (*func)(void *), void *arg);

void
schedule(void);

void *
kmalloc(size_t size);

void
kfree(void *ptr);

void
mmu_switch(struct proc *p);

extern struct proc *current;
extern struct proc_regs *user_regs;
