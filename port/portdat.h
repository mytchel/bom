#include "../include/syscalls.h"
#include "../include/stdarg.h"

enum { SEG_RW, SEG_RO };

struct page {
	void *pa, *va;
	size_t size;
	struct page *next;
};

struct segment {
	int type;
	void *base, *top;
	int size;
	struct page *pages;
};

enum {
	PROC_unused, /* Not really a state */
	PROC_suspend,
	PROC_ready,
	PROC_sleep,
};

enum { Sstack, Stext, Sdata, Sbss, Smax };

struct proc {
	struct label label;
	char kstack[KSTACK];

	struct ureg *ureg;
	
	int state;
	int pid;
	int faults;
	uint32_t quanta;
	
	int sleep;
	
	struct proc *parent;

	struct segment *segs[Smax];
	struct page *mmu;
	
	struct proc *next;
};

struct proc *
newproc();

void
procremove(struct proc *);

void
procready(struct proc *);

void
procsleep(struct proc *, uint32_t);

void
procsuspend(struct proc *);

void
initprocs(void);

void
schedule(void);

struct segment *
newseg(int, void *, size_t);

struct segment *
findseg(struct proc *, void *);

struct segment *
copyseg(struct segment *, bool);

bool
fixfault(void *);

void
memmove(void *n, void *o, size_t);

void
kprintf(const char *, ...);

/****** Machine implimented ******/

void
puts(const char *);

char
getc(void);

/* Number of ticks since last call. */
uint32_t
ticks(void);

uint32_t
tickstoms(uint32_t);

uint32_t
mstoticks(uint32_t);

/* Set systick interrupt to occur in .. ms */
void
setsystick(uint32_t);

void
forkfunc(struct proc *, int (*func)(void));

void
forkchild(struct proc *, struct ureg *);

void *
kmalloc(size_t);

void
kfree(void *, size_t);

void
mmuswitch(struct proc *);

void
mmuenable(void);

void
mmudisable(void);

void
mmuputpage(struct page *);

struct page *
newpage(void *);

void
freepage(struct page *);

int
setlabel(struct label *);

int
gotolabel(struct label *);


/* Global Variables */

extern struct proc *current;
extern int (*syscalltable[NSYSCALLS])(va_list args);
