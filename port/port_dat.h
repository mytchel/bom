typedef uint8_t bool;

#define true 1
#define false 0

#define nil 0

void
enable_interrupts(void);

void
disable_interrupts(void);

enum { SEGMENT_STACK, NSEG };

struct segment {
	int type;
	void *base, *top; /* Virtual Address */
	uint32_t size;
};

#define MAX_PROCS	512
#define STACK_SIZE	1024
#define KSTACK_SIZE	256

enum { PROC_stopped, PROC_scheduling, PROC_running, PROC_ready, PROC_sleeping };

struct proc {
	struct proc_regs regs;
	
	reg_t stack[KSTACK_SIZE]; /* Kernel Stack. */
	
	int state;
	int pid;
	
	struct segment segs[NSEG];

	struct proc *next;
};

void panic(const char *);

struct proc *
proc_create(void (*func)(void *), void *arg);

void
proc_remove(struct proc *);

void
proc_init_regs(struct proc *p, 
	void (*func)(void *), void *arg);

void
run_proc(struct proc *);

void
schedule(void);

void *
kmalloc(size_t size);

void
kfree(void *ptr);

void
mmu_switch(struct proc *p);

struct page *
make_page(void *va);

void
free_page(struct page *p);

extern struct proc *current;
extern struct proc_regs *user_regs;
