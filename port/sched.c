#include "dat.h"
#include "../port/com.h"
#include "../include/std.h"

int
kmain(int, void *);

static int
__idle__(int, void *);

struct proc_regs *user_regs;
struct proc *current;
struct proc procs[MAX_PROCS];
static uint32_t next_pid;

void
scheduler_init(void)
{
	int i;
	
	kprintf("scheduler_init\n");

	next_pid = 1;

	for (i = 0; i < MAX_PROCS; i++)
		procs[i].state = PROC_stopped;

	kprintf("init __idle__ proc\n");
	
	current = procs;
	procs->next = nil;	
	procs->state = PROC_ready;
	proc_init_regs(procs, &exit, &__idle__, 0, nil);

	kprintf("init kmain proc\n");
	proc_create(&kmain, 0, nil);
}

void
schedule(void)
{
	struct proc *p;
	
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
		p = procs;
	}

	mmu_switch(p);
	resume_proc(p);
}

static struct proc *
find_and_add_proc_space()
{
	int i;
	struct proc *p, *pp;
	
	p = nil;
	for (i = 0; i < MAX_PROCS; i++) {
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
proc_create(int (*func)(int, void *), int argc, void *args)
{
	struct proc *p;

	p = find_and_add_proc_space();
	if (p == nil) {
		kprintf("Max process count reached\n");
		return nil;
	}
	
	proc_init_stack(p);
	proc_init_regs(p, &exit, func, argc, args);
		
	p->pid = next_pid++;
	p->state = PROC_ready;

	return p;
}

void
proc_remove(struct proc *p)
{
	struct proc *pp;
//	struct page *pgc, *pgn;

	/* Remove proc from list. */
	for (pp = procs; pp->next != p; pp = pp->next);
	pp->next = p->next;

	/* Free pages. */
	/*
	pgc = p->page;
	while (pgc) {
		pgn = pgc->next;
		kfree(pgc);
		pgc = pgn;
	}
	*/

	/* Make procs place as useable. */	
	p->state = PROC_stopped;
}

int
__idle__(int argc, void *args)
{
	while (true);
	return -1;
}

