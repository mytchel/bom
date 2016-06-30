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
	PROC_stopped, 
	PROC_ready,
	PROC_sleeping
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
	
	struct proc *parent;

	struct segment *segs[Smax];
	struct page *mmu;
	
	struct proc *next;
};

extern struct proc *current;
extern int (*syscalltable[NSYSCALLS])(va_list args);

/* Process */

struct proc *
newproc();

void
procremove(struct proc *);

void
procready(struct proc *);

void
procinitstack(struct proc *);

int
setlabel(struct label *);

int
gotolabel(struct label *);

void
schedule(void);

void
setsystick(uint32_t);

/* Set up proc->label for return from fork. */
void
forklabel(struct proc *, struct ureg *);

/* Memory */

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

/* Helper */

void
memmove(void *n, void *o, size_t);

/* Initialisation */

void
initprocs(void);
