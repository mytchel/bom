#include "dat.h"
#include "../port/com.h"
#include "../include/std.h"

int
main();

struct proc *current;

static struct proc procs[MAX_PROCS];
static uint32_t nextpid;

void
schedulerinit(void)
{
	int i;
	
	kprintf("scheduler init\n");

	nextpid = 1;

	for (i = 0; i < MAX_PROCS; i++)
		procs[i].state = PROC_stopped;

	kprintf("init main proc\n");
	
	current = procs;
	procs->next = nil;
	
	main();
	
	schedule();
}

void
schedule(void)
{
	struct proc *p;

	if (setlabel(&current->label)) {
		kprintf("return from setlabel\n");
		return;
	}
	
	kprintf("schedule\n");
	
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
		kprintf("gave up\n");
		schedule();
	}
	
	current = p;
	mmuswitch(current);
	gotolabel(&current->label);
}

static struct proc *
findfreeproc()
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
	
	p->state = PROC_scheduling;
	p->next = nil;
	
	for (pp = procs; pp->next; pp = pp->next);
	pp->next = p;
	
	return p;
}

struct proc *
newproc()
{
	struct proc *p;

	p = findfreeproc();
	if (p == nil) {
		kprintf("Max process count reached\n");
		return nil;
	}
	
	p->pid = nextpid++;
	p->state = PROC_ready;

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
