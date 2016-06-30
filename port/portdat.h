#include "../include/syscall.h"
#include "../include/stdarg.h"

typedef uint8_t bool;

#define true 1
#define false 0

#define nil 0

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

enum { Sstack, Stext, Sdata, Smax };

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

/*	---------	Process */

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

int
setlabel(struct label *);

int
gotolabel(struct label *);

void
schedule(void);

void
forkfunc(struct proc *, int (*func)(void));

void
forkchild(struct proc *, struct ureg *);

/*	---------	Memory */

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

struct segment *
newseg(int, void *, size_t);

struct segment *
findseg(struct proc *, void *);

struct segment *
copyseg(struct segment *, bool);

bool
fixfault(void *);

/*	---------	Helper */

void
memmove(void *n, void *o, size_t);

/*	---------	Timer functions */

/* Number of ticks since last call. */
uint32_t
ticks(void);

uint32_t
tickstoms(uint32_t);

uint32_t
mstoticks(uint32_t);

/* Set systick interrupt to occur in .. */
void
setsystick(uint32_t);

/*	---------	Initialisation */

void
initprocs(void);

/*	---------	Global Variables */

extern struct proc *current;
extern int (*syscalltable[NSYSCALLS])(va_list args);
