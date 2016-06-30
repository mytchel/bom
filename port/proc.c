#include "dat.h"
#include "../port/com.h"
#include "../include/std.h"

struct proc *current;

static struct proc procs[MAX_PROCS];
static uint32_t nextpid;

void
procsinit(void)
{
	int i;
	
	kprintf("procs init\n");

	nextpid = 1;

	for (i = 0; i < MAX_PROCS; i++)
		procs[i].state = PROC_stopped;

	current = procs;
	procs->next = nil;
}

void
schedule(void)
{
	struct proc *p;

	if (setlabel(&current->label)) {
		return;
	}
	
	p = current;
	do {
		p = p->next;
		if (p == nil) {
			/* Go to start of list or break if empty. */
			if ((p = procs->next) == nil)
				break;
		}

		if (p->state == PROC_ready) {
			break;
		}
	} while (p != current);
	
	/* No processes to run. */
	if (p == nil || p->state != PROC_ready) {
		schedule();
	}

	current = p;
	mmuswitch(current);
	setsystick(current->quanta);
	gotolabel(&current->label);
}

struct proc *
newproc()
{
	int i;
	struct proc *p, *pp;
	
	p = nil;
	for (i = 1; i < MAX_PROCS; i++) {
		if (procs[i].state == PROC_stopped) {
			p = &procs[i];
			break;
		}
	}
	
	if (p == nil)
		return nil;
	
	p->state = PROC_sleeping;
	p->ureg = nil;
	p->pid = nextpid++;
	p->faults = 0;
	p->quanta = 10;
	p->parent = nil;
			
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

	/* Make procs place as useable. */	
	p->state = PROC_stopped;
}

void
procready(struct proc *p)
{
	p->state = PROC_ready;
}
