typedef uint8_t bool;

#define true 1
#define false 0

#define nil 0

#define MAX_PROCS	512
#define STACK_SIZE	1024
#define KSTACK_SIZE	1024 

enum {	PROC_stopped, PROC_scheduling, 
	PROC_ready, PROC_sleeping, 
	PROC_mount, PROC_wait };

struct proc {
	struct proc_regs regs;
	reg_t stack[KSTACK_SIZE]; /* Kernel Stack. */
	
	int state;
	int pid;
	
	/* Used by exit to determine if it should exit the
	 * process or if it should jump to another part. 
	 * Useful for signals and mount. */
	void *sysexit;
	int sysret;
	
	struct proc *next;
};

extern struct proc *current;

void
panic(const char *);

struct proc *
proc_create(int (*func)(int, void *), int argc, void *args);

void
proc_remove(struct proc *);

void
proc_init_stack(struct proc *p);

void
proc_init_regs(struct proc *p, int (*exit)(int),
	int (*func)(int, void *), int argc, void *args);

void
resume_proc(struct proc *);

/* Actually takes a variable number of arguments. 
 * Is machine dependent. */
void
handle_syscall(void);

/* Finds the next process to run and runs it. */
void
schedule(void);

/* Like schedule but saves the current process. 
 * For use in system calls. */
void
reschedule(void);

void *
kmalloc(size_t size);

void
kfree(void *ptr);

void
mmu_switch(struct proc *p);

void
fs_mount(struct proc *p, char *path);

struct proc *
fs_find_mount(struct proc *p, const char *path);

/* Kernel initialisation functions. */

void
fs_init(void);

void
scheduler_init(void);

