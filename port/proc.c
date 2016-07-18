#include "dat.h"

static struct proc *
nextproc(void);

static uint32_t nextpid;
static struct proc procs[MAX_PROCS];

struct proc *current;

void
initprocs(void)
{
	int i;
	for (i = 0; i < MAX_PROCS; i++)
		procs[i].state = PROC_unused;
	procs[0].next = nil;
	
	nextpid = 1;
	current = procs;
}

void
schedule(void)
{
	struct proc *p;
	
	if (setlabel(&current->label)) {
		return;
	}
	
	p = nextproc();
	current = p;
	mmuswitch(current);
	setsystick(current->quanta);
	gotolabel(&current->label);
}

struct proc *
nextproc(void)
{
	struct proc *p;
	uint32_t ms;

	ms = tickstoms(ticks());

	for (p = procs->next; p != nil; p = p->next) {
		if (p->state == PROC_sleeping) {
			p->sleep -= ms;
			if (p->sleep <= 0)
				p->state = PROC_ready;
		}
	}
	
	p = current;
	do {
		p = p->next;
		if (p == nil) {
			p = procs->next;
		}
		
		if (p->state == PROC_ready) {
			break;
		}
	} while (p != current);
	
	return p;
}
	
struct proc *
newproc(void)
{
	int i;
	struct proc *p, *pp;
	
	p = nil;
	for (i = 1; i < MAX_PROCS; i++) {
		if (procs[i].state == PROC_unused) {
			p = &procs[i];
			break;
		}
	}
	
	if (p == nil)
		return nil;
	
	p->parent = nil;
	p->state = PROC_suspend;
	p->pid = nextpid++;
	
	p->faults = 0;
	p->quanta = 40;
	
	p->ureg = nil;
	
	p->mmu = nil;
	for (i  = 0; i < Smax; i++)
		p->segs[i] = nil;
	
	for (pp = procs; pp->next; pp = pp->next);
	pp->next = p;
	p->next = nil;
	
	return p;
}

void
procremove(struct proc *p)
{
	struct proc *pp;

	/* Remove proc from list. */
	for (pp = procs; pp->next != p; pp = pp->next);
	pp->next = p->next;

	p->state = PROC_unused;
}

void
procready(struct proc *p)
{
	p->state = PROC_ready;
}

void
procsleep(struct proc *p, uint32_t ms)
{
	p->state = PROC_sleeping;
	p->sleep = ms;
	
	if (p == current)
		schedule();
}

void
procsuspend(struct proc *p)
{
	p->state = PROC_suspend;
}

void
procwait(struct proc *p)
{
	p->state = PROC_waiting;
}
