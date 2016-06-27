typedef uint8_t bool;

#define true 1
#define false 0

#define nil 0

#define MAX_PROCS	512
#define STACK_SIZE	1024
#define KSTACK_SIZE	1024 
#define MAX_OPEN_FILES	32

enum { SEG_stack, SEG_text, SEG_data, NSEG };

struct page {
	void *pa;
	struct page *next;
};

struct segment {
	void *top, *bottom;
	struct page *pages;
};

enum {
	PROC_stopped, PROC_scheduling, 
	PROC_exiting, PROC_ready, 
	PROC_sleeping, PROC_wait,
	PROC_mount,
};

struct proc {
	struct label label;
	reg_t stack[KSTACK_SIZE]; /* Kernel Stack. */
	
	int state;
	int pid;

	struct segment segments[NSEG];
	
	struct proc *next;
};

extern struct proc *current;

struct proc *
newproc();

void
procremove(struct proc *);

void
procinitstack(struct proc *p);

int
setlabel(struct label *);

int
gotolabel(struct label *);

void
schedule(void);

void
schedulerinit(void);

void *
kmalloc(size_t size);

void
kfree(void *ptr);

void
mmuswitch(struct proc *p);

void *
getpage(void);
