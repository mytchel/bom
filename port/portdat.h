#include "../include/syscalls.h"
#include "../include/stdarg.h"

struct page {
	void *pa, *va;
	size_t size;
	struct page *next;
};

enum { SEG_rw, SEG_ro };

struct segment {
	int refs;
	int type;
	void *base, *top;
	int size;
	struct page *pages;
};

enum { PIPE_none, PIPE_writing, PIPE_reading };

struct pipe {
	int refs;
	struct pipe *link;
	
	uint8_t action;
	struct proc *user;
	uint8_t *buf;
	size_t n;
};

struct path {
	char *s;
	struct path *next;
};

struct fgroup {
	int refs;
	struct pipe **pipes;
	int npipes;
};

struct ngroup {
	int refs;
	struct pipe *root;
};

enum {
	PROC_unused, /* Not really a state */
	PROC_suspend,
	PROC_ready,
	PROC_sleeping,
	PROC_waiting,
};

enum { Sstack, Stext, Sdata, Sbss, Smax };

struct proc {
	struct label label;
	char kstack[KSTACK];

	struct ureg *ureg;
	
	int state;
	int pid;
	struct proc *parent;
	
	int faults;
	uint32_t quanta;
	int sleep;

	struct fgroup *fgroup;
	struct ngroup *ngroup;

	struct segment *segs[Smax];
	struct page *mmu;
	
	struct proc *next;
};

void
initprocs(void);

struct proc *
newproc(void);

void
procremove(struct proc *);

void
procready(struct proc *);

void
procsleep(struct proc *, uint32_t);

void
procsuspend(struct proc *);

void
procwait(struct proc *);

void
schedule(void);

struct segment *
newseg(int, void *, size_t);

void
freeseg(struct segment *);

struct segment *
copyseg(struct segment *, bool);

bool
fixfault(void *);

void *
kaddr(struct proc *, void *);

void
initheap(void *, size_t);

void *
kmalloc(size_t);

void
kfree(void *);

void
memmove(void *, const void *, size_t);

struct pipe *
newpipe(void);

void
freepipe(struct pipe *);

struct path *
strtopath(const char *);

void
freepath(struct path *);

struct fgroup *
newfgroup(void);

void
freefgroup(struct fgroup *);

struct fgroup *
copyfgroup(struct fgroup *);

int
addpipe(struct fgroup *, struct pipe *);

struct pipe *
fdtopipe(struct fgroup *, int);

struct ngroup *
newngroup(void);

struct ngroup *
copyngroup(struct ngroup *);

void
freengroup(struct ngroup *);

void
kprintf(const char *, ...);


/****** Syscalls ******/


int sysexit(va_list);
int sysfork(va_list);
int syssleep(va_list);
int sysgetpid(va_list);
int syspipe(va_list);
int sysread(va_list);
int syswrite(va_list);
int sysclose(va_list);
int sysbind(va_list);
int sysopen(va_list);


/****** Machine Implimented ******/


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

int
setlabel(struct label *);

int
gotolabel(struct label *);

void
forkfunc(struct proc *, int (*func)(void));

void
forkchild(struct proc *, struct ureg *);

void
mmuswitch(struct proc *);

void
mmuenable(void);

void
mmudisable(void);

void
mmuputpage(struct page *, bool);

/* Find a unused page. */
struct page *
newpage(void *);

void
freepage(struct page *);


/****** Global Variables ******/


extern struct proc *current;

extern int (*syscalltable[NSYSCALLS])(va_list);
