/*
 *   Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <libc.h>
#include <stdarg.h>
#include <fs.h>
#include <string.h>

#if DEBUG == 1
#define debug(...)  printf(__VA_ARGS__)
#else
#define debug(...) {}
#endif

struct lock {
  int lock, listlock;
  struct proc *waiting;
};
  
struct page {
  void *pa, *va;
  size_t size;
  bool cachable;
  struct page *next;
  struct page *from;
};

enum { SEG_rw, SEG_ro };

struct segment {
  int refs;
  int type;
  struct page *pages;
};

enum { CHAN_pipe, CHAN_file, CHAN_max };

struct chan {
  int refs;
  struct lock lock;
	
  int type;
  int mode;
  struct path *path;
	
  void *aux;
};

struct chantype {
  int (*read)(struct chan *, uint8_t *, size_t);
  int (*write)(struct chan *, uint8_t *, size_t);
  int (*close)(struct chan *);
};

struct path {
  uint8_t *s;
  struct path *prev, *next;
};

struct fgroup {
  int refs;
  struct lock lock;
  struct chan **chans;
  size_t nchans;
};

struct binding {
  int refs;
  struct lock lock;
	
  struct path *path;
  struct chan *in, *out;
  uint32_t rootfid;

  int nreqid;
  struct proc *waiting; /* List of procs waiting. */
  struct proc *srv; /* Kernel proc that handles responses */
};

struct binding_list {
  struct binding *binding;
  struct binding_list *next;
};

struct ngroup {
  int refs;
  struct lock lock;
  struct binding_list *bindings;
};

enum {
  PROC_unused, /* Not really a state */
  PROC_suspend,
  PROC_ready,
  PROC_sleeping,
  PROC_waiting,
};

enum { Sstack, Stext, Sdata, Sheap, Smax };

struct proc {
  struct proc *next; /* Next in schedule queue */
	
  struct label label;
  uint8_t kstack[KSTACK];

  struct ureg *ureg;

  bool inkernel;
  int state;
  int pid;
  struct proc *parent;
  struct path *dot;
	
  int faults;
  uint32_t quanta;
  uint32_t sleep; /* in ticks */

  struct fgroup *fgroup;
  struct ngroup *ngroup;

  struct segment *segs[Smax];
  struct page *mmu;
	
  struct proc *wnext; /* Next in wait queue */
  void *aux;
  union {
    int rid;
    int intr;
  } waiting;
};


/****** Initialisation ******/

void
initroot(void);

void
initheap(void *, size_t);


/****** General Functions ******/

void
initlock(struct lock *);

void
lock(struct lock *);

void
unlock(struct lock *);

/* Procs */

struct proc *
newproc(void);

void
initproc(struct proc *);

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

/* Segments / Proc Memory */

struct segment *
newseg(int);

void
freeseg(struct segment *);

struct segment *
copyseg(struct segment *, bool);

bool
fixfault(void *);

void *
kaddr(struct proc *, void *);

/* Get an unused page. */
struct page *
newpage(void *);

/* Get specific pages from page list. */
struct page *
getpages(struct page *, void *, size_t *);

void
freepage(struct page *);

/* Channels */

struct chan *
newchan(int, int, struct path *);

void
freechan(struct chan *);

/* Paths */

struct path *
strtopath(const uint8_t *);

uint8_t *
pathtostr(struct path *, int *);

struct path *
realpath(struct path *, const uint8_t *);

struct path *
copypath(struct path *);

void
freepath(struct path *);

/* Fgroup */

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

/* Ngroup */

struct ngroup *
newngroup(void);

struct ngroup *
copyngroup(struct ngroup *);

void
freengroup(struct ngroup *);

struct binding *
newbinding(struct path *, struct chan *, struct chan *);

void
freebinding(struct binding *);

struct binding *
findbinding(struct ngroup *, struct path *, int);

/* IPC */

int
mountproc(void *);

bool
newpipe(struct chan **, struct chan **);

int
piperead(struct chan *, uint8_t *, size_t);

int
pipewrite(struct chan *, uint8_t *, size_t);

int
pipeclose(struct chan *);

struct chan *
fileopen(struct path *, uint32_t, uint32_t, int *);

int
fileremove(struct path *);

/* Debug */

void
printf(const char *, ...);

void
panic(const char *, ...);


/****** Syscalls ******/


reg_t sysexit(va_list);
reg_t sysfork(va_list);
reg_t syssleep(va_list);
reg_t sysgetpid(va_list);
reg_t sysgetmem(va_list);
reg_t sysrmmem(va_list);
reg_t syswaitintr(va_list);
reg_t syspipe(va_list);
reg_t sysread(va_list);
reg_t syswrite(va_list);
reg_t sysclose(va_list);
reg_t sysbind(va_list);
reg_t sysopen(va_list);
reg_t sysremove(va_list);


/****** Machine Implimented ******/

void
dumpregs(struct ureg *);

void
puts(const char *);

/* Number of ticks since last call. */
uint32_t
ticks(void);

uint32_t
tickstoms(uint32_t);

uint32_t
mstoticks(uint32_t);

/* Set systick interrupt to occur in ... ms */
void
setsystick(uint32_t);

int
setlabel(struct label *);

int
gotolabel(struct label *);

void
forkfunc(struct proc *, int (*func)(void *), void *);

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

void *
pagealign(void *);

bool
procwaitintr(struct proc *, int);

void
disableintr(void);

void
enableintr(void);

/* If address contains 0, store 1 and return 1,
 * else return 0.
 */
bool
testandset(int *addr);


/****** Global Variables ******/


extern struct proc *current;

extern reg_t (*syscalltable[NSYSCALLS])(va_list);

extern struct chantype devpipe;
extern struct chantype devfile;
extern struct chantype *chantypes[CHAN_max];

extern struct page pages, iopages;

extern struct binding *rootbinding;
