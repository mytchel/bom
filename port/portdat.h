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

enum { CHAN_pipe, CHAN_file, CHAN_max };

struct chan {
	int refs;
	int lock;
	
	int type;
	int flags;
	struct path *path;
	
	void *aux;
};

struct chantype {
	int (*read)(struct chan *, void *, int);
	int (*write)(struct chan *, void *, int);
	int (*close)(struct chan *);
};

struct path {
	int refs;
	int lock;
	char *s;
	struct path *next;
};

struct fgroup {
	int refs;
	int lock;
	struct chan **chans;
	size_t nchans;
};

struct binding {
	struct path *path;
	struct chan *chan;
	struct binding *next;
};

struct ngroup {
	int refs;
	int lock;
	struct binding *bindings;
};

enum {
	PROC_unused, /* Not really a state */
	PROC_suspend,
	PROC_ready,
	PROC_sleeping,
	PROC_waiting,
};

enum { Sstack, Stext, Sdata, Smax };

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


/****** Initialisation ******/


void
initprocs(void);


/****** General Functions ******/


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

struct chan *
newchan(int, int, struct path *);

void
freechan(struct chan *);

struct path *
strtopath(const char *);

char *
pathtostr(struct path *);

void
freepath(struct path *);

size_t
pathmatches(struct path *, struct path *);

size_t
pathelements(struct path *);

struct fgroup *
newfgroup(void);

void
freefgroup(struct fgroup *);

struct fgroup *
copyfgroup(struct fgroup *);

int
addchan(struct fgroup *, struct chan *);

struct chan *
fdtochan(struct fgroup *, int);

struct ngroup *
newngroup(void);

struct ngroup *
copyngroup(struct ngroup *);

void
freengroup(struct ngroup *);

void *
memmove(void *, const void *, size_t);

void *
memset(void *, int, size_t);

bool
strcmp(const char *, const char *);

size_t
strlen(const char *);

void
lock(int *);

void
unlock(int *);

bool
newpipe(struct chan **, struct chan **);

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

extern struct chantype devpipe;
extern struct chantype devfile;
extern struct chantype *chantypes[CHAN_max];
