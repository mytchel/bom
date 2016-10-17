/*
 *
 * Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <libc.h>
#include <syscalls.h>
#include <mem.h>
#include <types.h>
#include <stdarg.h>
#include <fs.h>
#include <fssrv.h>
#include <string.h>

#ifdef _am335x_
#include "../am335x/head.h"
#else
#error Need to set arch
#endif

struct lock {
  int lock;
  struct proc *holder;
  struct proc *wlist;
};
  
struct page {
  int refs;
  /* Not changable */
  void *pa;
  bool forceshare;
  struct page *next;
  struct page **from;
};

struct pagel {
  void *va;
  bool rw, c;
  struct page *p;
  struct pagel *next;
};

struct mgroup {
  int refs;
  struct lock lock;
  struct pagel *pages;
};

typedef enum { CHAN_pipe, CHAN_file, CHAN_max } chan_t;

struct chan {
  int refs;
  struct lock lock;
	
  chan_t type;
  int mode;
	
  void *aux;
};

struct chantype {
  int (*read)(struct chan *, uint8_t *, size_t);
  int (*write)(struct chan *, uint8_t *, size_t);
  int (*seek)(struct chan *, size_t, int);
  void (*close)(struct chan *);
};

struct path {
  char s[FS_NAME_MAX];
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
	
  struct chan *in, *out;

  int nreqid;
  struct proc *waiting; /* List of procs waiting. */
  struct proc *srv; /* Kernel proc that handles responses */
};

struct bindingl {
  struct binding *binding;

  struct path *path;
  uint32_t rootfid;

  struct bindingl *next;
};

struct ngroup {
  int refs;
  struct lock lock;
  struct bindingl *bindings;
};

typedef enum {
  PROC_oncpu,
  PROC_suspend,
  PROC_ready,
  PROC_sleeping,
  PROC_waiting,
  PROC_dead,
} procstate_t;

#define MIN_PRIORITY 100
#define NULL_PRIORITY (MIN_PRIORITY+1)

struct proc {
  struct proc *next; /* For list of procs in list */
  struct proc **list;

  size_t nchildren;
  int exitcode;
  struct proc *deadchildren;
  
  struct label *ureg;
  struct label label;

  procstate_t state;
  uint32_t pid;
  struct proc *parent;

  struct path *dot;
  struct chan *dotchan;

  int priority;
  uint32_t timeused;
  uint32_t cputime;

  struct page *kstack;
  struct pagel *mmu, *ustack;

  struct mgroup *mgroup;
  struct fgroup *fgroup;
  struct ngroup *ngroup;

  uint32_t sleep; /* in ticks */
  void *aux;
};


/****** Initialisation ******/

void
schedulerinit(void);

void
rootfsinit(void);

void
procfsinit(void);

void
heapinit(void *, size_t);


/****** General Functions ******/

void
lockinit(struct lock *);

void
lock(struct lock *);

void
unlock(struct lock *);

/* Procs */

struct proc *
procnew(unsigned int priority);

void
procfsaddproc(struct proc *);

void
procfsrmproc(struct proc *);

struct proc *
procwaitchildren(void);

/* These must all be called with interrupts disabled */

void
procexit(struct proc *, int code);

void
procfree(struct proc *);

void
procready(struct proc *);

void
procsuspend(struct proc *p);

void
procwait(struct proc *p, struct proc **wlist);

void
procsleep(struct proc *p, uint32_t ms);

void
procyield(struct proc *p);

void
procsetpriority(struct proc *p, unsigned int priority);

void
schedule(void);

/* Pages and Mgroup */

void
pagefree(struct page *);

void
pagelfree(struct pagel *);

struct pagel *
pagelcopy(struct pagel *);

struct pagel *
wrappage(struct page *p, void *va, bool rw, bool c);

struct mgroup *
mgroupnew(void);

struct mgroup *
mgroupcopy(struct mgroup *old);

void
mgroupaddpage(struct mgroup *m, struct page *p,
	      void *va, bool rw, bool c);

void
mgroupfree(struct mgroup *m);

/* Memory */

bool
fixfault(void *);

/* Finds the physical address of addr in p's address
 * space and checks if it exstends through to size. 
 * If size is 0, checks if it exstends through to a
 * 0 byte. If it failes any of these checks returns nil.
 */
void *
kaddr(struct proc *p, void *addr, size_t size);

/* Channels */

struct chan *
channew(int, int);

void
chanfree(struct chan *);

/* Paths */

struct path *
strtopath(const char *);

char *
pathtostr(struct path *, size_t *);

struct path *
realpath(struct path *, const char *);

struct path *
pathcopy(struct path *);

void
pathfree(struct path *);

/* Fgroup */

struct fgroup *
fgroupnew(void);

void
fgroupfree(struct fgroup *);

struct fgroup *
fgroupcopy(struct fgroup *);

int
fgroupaddchan(struct fgroup *, struct chan *);

struct chan *
fdtochan(struct fgroup *, int);

/* Ngroup */

struct ngroup *
ngroupnew(void);

void
ngroupfree(struct ngroup *);

struct ngroup *
ngroupcopy(struct ngroup *);

struct binding *
bindingnew(struct chan *out, struct chan *in);

void
bindingfree(struct binding *);

struct binding *
ngroupfindbinding(struct ngroup *n, struct path *path,
		  int depth, uint32_t *rootfid);

int
ngroupaddbinding(struct ngroup *n, struct binding *b,
	   struct path *p, uint32_t rootfid);

void
ngrouprmbinding(struct ngroup *n, struct binding *b);

/* IPC */

int
mountproc(void *);

bool
pipenew(struct chan **, struct chan **);

int
piperead(struct chan *, uint8_t *, size_t);

int
pipewrite(struct chan *, uint8_t *, size_t);

int
filestat(struct path *, struct stat *stat);

struct chan *
fileopen(struct path *, uint32_t, uint32_t, int *);

int
fileremove(struct path *);

int
kmountloop(struct chan *in, struct binding *b, struct fsmount *mount);

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
reg_t syswait(va_list);
reg_t sysgetmem(va_list);
reg_t sysrmmem(va_list);
reg_t syswaitintr(va_list);
reg_t syschdir(va_list);
reg_t syspipe(va_list);
reg_t sysread(va_list);
reg_t syswrite(va_list);
reg_t sysseek(va_list);
reg_t sysstat(va_list);
reg_t sysclose(va_list);
reg_t sysbind(va_list);
reg_t sysopen(va_list);
reg_t sysremove(va_list);
reg_t syscleanpath(va_list);


/****** Machine Implimented ******/

/* Type and psr are ignored. */
void
droptouser(struct label *) __attribute__((noreturn));

void
dumpregs(struct label *);

void
puts(const char *);

/* Number of ticks since last call. */
uint32_t
ticks(void);

/* Clear ticks counter */
void
cticks(void);

uint32_t
tickstoms(uint32_t);

uint32_t
mstoticks(uint32_t);

void
setsystick(uint32_t ticks);

int
setlabel(struct label *);

int
gotolabel(struct label *) __attribute__((noreturn));

int
nullprocfunc(void *);

void
forkfunc(struct proc *, int (*func)(void *), void *);

void
forkchild(struct proc *, struct label *);

void
mmuswitch(struct proc *);

void
mmuputpage(struct pagel *);

bool
procwaitintr(int);

struct page *
getrampage(void);

struct page *
getiopage(void *addr);

/* 

Can be macros and should be defined in head.h

void
disableintr(void);

void
enableintr(void);

*/

/****** Global Variables ******/

extern struct proc *up;

extern reg_t (*syscalltable[NSYSCALLS])(va_list);

extern struct chantype *chantypes[CHAN_max];

extern struct binding *rootfsbinding, *procfsbinding;
