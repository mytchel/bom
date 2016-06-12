#include "types.h"
#include "proc.h"
#include "../include/kmalloc.h"
#include "../include/proc.h"
#include "../include/com.h"

void start_proc(struct proc_machine *);

struct proc_machine *
sched(void);

void kmain(void *);

struct proc_link {
	struct proc proc;
	struct proc_machine machine;
	struct proc_link *next;
};

static struct proc_link *proc_list;
static struct proc_link *current;

static bool enabled;

void
scheduler_init(void)
{
	kprintf("scheduler_init\n");
	
	enabled = false;
	proc_list = kmalloc(sizeof(struct proc_link));
	proc_list->next = nil;

	proc_create(&kmain, nil);
	current = proc_list->next;
	enabled = true;
	kprintf("starting first proc\n");
	start_proc(&current->machine);
}

struct proc_machine *
schedule(void)
{
	struct proc_link *p;
	
	if (!enabled) 
		return &current->machine;

	kprintf("schedule\n");
	p = current;
	do {
		kprintf("go to next\n");
		p = p->next;
		if (p == nil) {
			/* Go to start of list or break if empty. */
			if ((p = proc_list->next) == nil)
				break;
		}
			
		kprintf("check state\n");

		if (p->proc.state == PROC_state_running) {
			break;
		}
	} while (p != current);

	kprintf("found\n");
	/* No processes to run. */
	if (p == nil || p->proc.state != PROC_state_running) {
		kprintf("but nothing to run\n");
		kprintf("do something!!!!\n");
	}
	
	current = p;
	
	return &current->machine;
}

struct proc *
proc_create(void (*func)(void *), void *arg)
{
	struct proc_link *p, *pp;
	
	if (enabled)
		enabled = false;
	
	kprintf("adding task\n");

	p = kmalloc(sizeof(struct proc_link));
	
	p->proc.state = PROC_state_running;

	proc_init_regs(&p->machine, func, arg);		

	/* Add to list. */
	for (pp = proc_list; pp->next; pp = pp->next);
	pp->next = p;
	p->next = nil;
	
	enabled = true;
	return &p->proc;
}

