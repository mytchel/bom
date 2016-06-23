#include "dat.h"
#include "../port/com.h"

void 
kmain(void *);

static void 
__idle__(void *);

struct proc_regs *user_regs;
struct proc *current;
struct proc procs[MAX_PROCS];
static uint32_t next_pid;

static bool adding;

void
scheduler_init(void)
{
	int i;
	
	kprintf("scheduler_init\n");

	next_pid = 1;
	adding = false;

	for (i = 0; i < MAX_PROCS; i++)
		procs[i].state = PROC_stopped;

	current = procs;
	procs->next = nil;	
	procs->state = PROC_running;
	proc_init_regs(procs, &__idle__, nil);

	proc_create(&kmain, nil);
}

void
schedule(void)
{
	struct proc *p;
	
	if (adding) {
		puts("adding, not resceduling\n");
		run_proc(current);
	}
	
	current->state = PROC_ready;
	
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

	p->state = PROC_running;

	mmu_switch(p);
	run_proc(p);
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
proc_create(void (*func)(void *), void *arg)
{
	struct proc *p;

	while (adding);
	adding = true;

	p = find_and_add_proc_space();
	if (p == nil) {
		kprintf("Max process count reached\n");
		return nil;
	}
	
	proc_init_regs(p, func, arg);
		
	p->pid = next_pid++;
	p->state = PROC_ready;

	kprintf("new proc created with pid %i\n", p->pid);

	adding = false;
		
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

void
__idle__(void *arg)
{
	while (true);
}

