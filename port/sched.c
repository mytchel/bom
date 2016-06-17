#include "types.h"
#include "mem.h"
#include "../include/proc.h"
#include "../include/com.h"

void kmain(void *);

static void __idle__(void *arg)
{
	while (true);
}

static struct proc *procs, *current;

static bool adding;
static uint32_t next_pid;

void
scheduler_init(void)
{
	kprintf("scheduler_init\n");

	adding = false;
	next_pid = 0;

	current = procs = kmalloc(sizeof(struct proc));
	procs->next = nil;	
	procs->state = PROC_running;
	proc_init_regs(procs, &__idle__, nil);
	
	proc_create(&kmain, nil);
}

struct proc_machine *
schedule(void)
{
	struct proc *p;
	
	if (adding) {
		return &(current->machine);
	}
	
	p = current;
	do {
		p = p->next;
		if (p == nil) {
			/* Go to start of list or break if empty. */
			if ((p = procs->next) == nil)
				break;
		}

		if (p->state == PROC_running) {
			break;
		}
	} while (p != current);

	/* No processes to run. */
	if (p == nil || p->state != PROC_running) {
		kprintf("nothing to run\n");
		current = procs;
	} else {
		current = p;
	}
	
	return &(current->machine);
}

struct proc *
proc_create(void (*func)(void *), void *arg)
{
	struct proc *p, *pp;

	kprintf("proc_create\n");
	
	while (adding);
	adding = true;

	p = kmalloc(sizeof(struct proc));
	if (p == nil) {
		kprintf("Run out of memory\n");
		adding = false;
		return nil;
	}
	
	kprintf("init regs\n");
	
	p->next = nil;
	p->state = PROC_running;
	p->pid = next_pid++;
	proc_init_regs(p, func, arg);
	
	for (pp = procs; pp->next; pp = pp->next);
	pp->next = p;

	kprintf("added\n");
	
	adding = false;
		
	return p;
}

void
proc_remove(struct proc *p)
{
	struct proc *pp;
	
	for (pp = procs; pp->next != p; pp = pp->next);
	pp->next = p->next;
}
