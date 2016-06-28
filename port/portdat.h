#include "../include/syscall.h"
#include "../include/stdarg.h"

typedef uint8_t bool;

#define true 1
#define false 0

#define nil 0

enum { SEG_stack, SEG_text, SEG_data, NSEG };

struct page {
	void *pa, *va;
	struct page *next;
};

struct segment {
	int type;
	void *base, *top;
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
	char kstack[KSTACK];

	struct ureg *ureg;
	
	int state;
	int pid;

	struct segment *segs[NSEG];
	struct page *mmu;
	
	struct proc *next;
};

extern struct proc *current;
extern int (*syscalltable[NSYSCALLS])(va_list args);

struct proc *
newproc();

/* Set up proc->label for return from fork. */
void
forklabel(struct proc *, struct ureg *);

void
procremove(struct proc *);

void
procinitstack(struct proc *);

int
setlabel(struct label *);

int
gotolabel(struct label *);

void
schedule(void);

void
procsinit(void);

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

struct page *
newpage(void *);

void
freepage(struct page *);

struct segment *
newseg(int, void *, size_t);


/* Helper */

void
memmove(void *n, void *o, size_t);
