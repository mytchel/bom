#include "types.h"
#include "proc.h"
#include "../include/proc.h"
#include "../include/com.h"

void start_proc(struct proc_machine *);

struct proc_machine *
sched(void);

void kmain(void *);

static void __idle__(void *arg)
{
	while (true);
}

struct proc_link {
	bool used;
	struct proc proc;
	struct proc_machine machine;
	struct proc_link *next;
};

static struct proc_link procs[MAX_PROCS];
static struct proc_link *current;

static bool adding;

void
scheduler_init(void)
{
	int i;
	kprintf("scheduler_init\n");

	for (i = 0; i < MAX_PROCS; i++)	{
		procs[i].used = false;
		procs[i].next = nil;
	}

	adding = false;
	
	current = procs;
	
	procs[0].used = true;
	proc_init_regs(&procs[0].machine, &__idle__, nil);
	
	proc_create(&kmain, nil);
}

struct proc_machine *
schedule(void)
{
	struct proc_link *p;
	
	if (adding) {
		kprintf("proccess being added, not rescheduling.\n");
		return &current->machine;
	}
	
	p = current;
	do {
		p = p->next;
		if (p == nil) {
			/* Go to start of list or break if empty. */
			if ((p = procs->next) == nil)
				break;
		}

		if (p->proc.state == PROC_state_running) {
			break;
		}
	} while (p != current);

	/* No processes to run. */
	if (p == nil || p->proc.state != PROC_state_running) {
		kprintf("nothing to run\n");
		current = procs;
	} else {
		current = p;
	}
	
	return &current->machine;
}

static struct proc_link *
new_proc()
{
	struct proc_link *p, *pp;

	/* Find first unused proc_list struct. */
	for (p = procs; p->used == true; p++);
	p->used = true;
	p->next = nil;
	
	for (pp = current; 
		pp->next != nil; 
		pp = pp->next);
	
	pp->next = p;
	
	return p;
}

struct proc *
proc_create(void (*func)(void *), void *arg)
{
	struct proc_link *p;

	while (adding);
	adding = true;

	p = new_proc();
	
	proc_init_regs(&p->machine, func, arg);		
	p->proc.state = PROC_state_running;

	adding = false;
		
	return &p->proc;
}
