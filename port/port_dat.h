typedef uint8_t bool;

#define true 1
#define false 0

#define nil 0

#define MAX_PROCS	512
#define STACK_SIZE	1024
#define KSTACK_SIZE	256

enum { 	FS_open, FS_close, FS_stat, FS_read, FS_write };

struct proc_fs {
	int func;
	int ret;
	struct proc *proc;
	
	int fd;
	
	char *path;
	int mode;
	
	char *buf;
	size_t size;
};

enum {	PROC_stopped, PROC_scheduling, 
	PROC_ready, PROC_sleeping, 
	PROC_mount, PROC_wait };


struct proc {
	struct proc_regs regs;
	
	reg_t stack[KSTACK_SIZE]; /* Kernel Stack. */
	
	int state;
	int pid;
	
	struct proc_fs fs;
	
	struct proc *next;
};

extern struct proc *current;

void
panic(const char *);

struct proc *
proc_create(void (*func)(void *), void *arg);

void
proc_remove(struct proc *);

void
proc_init_regs(struct proc *p, 
	void (*func)(void *), void *arg);

void
run_proc(struct proc *);

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

