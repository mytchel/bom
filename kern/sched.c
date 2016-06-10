#include "types.h"
#include "proc.h"
#include "../include/kmalloc.h"
#include "../include/proc.h"
#include "../include/com.h"

void kmain(void *);

struct proc_link {
	struct proc proc;
	struct proc_machine machine;
	struct proc_link *next;
};

static struct proc_link *proc_list;
static struct proc_link *current;

struct proc_machine *machine_current;

void
scheduler_init(void)
{
	kprintf("scheduler_init\n");
	
	proc_list = kmalloc(sizeof(struct proc_link));
	
	proc_list->next = nil;

	proc_create(&kmain, nil);

	current = proc_list;
	
	machine_current = &current->machine;
}

void
schedule(void)
{
	struct proc_link *p;

	kprintf("schedule\n");
	p = current;
	do {
		kprintf("go to next\n");
		p = p->next;
		if (p == nil)
			p = proc_list->next;
		if (p == nil)
			break;
			
		kprintf("check state\n");

		if (p->proc.state == PROC_state_running) {
			break;
		} else if (p->proc.state == PROC_state_exiting) {
			continue;
		} else if (p->proc.state == PROC_state_sleeping) {
			continue;
		}
	} while (p != current);

	kprintf("found\n");	
	if (p == nil || 
		(p->proc.state != PROC_state_running &&
			p == current)) { /* No threads to run. */
		kprintf("but nothing to run\n");
		current = proc_list;
	}
	
	machine_current = &current->machine;
	kprintf("activate\n");
	activate_proc(machine_current);
}

struct proc *
proc_create(void (*func)(void *), void *arg)
{
	struct proc_link *p, *pp;

	p = kmalloc(sizeof(struct proc_link));
	p->next = nil;
	
	kprintf("adding task\n");
	
	p->proc.state = PROC_state_running;

	proc_init_regs(&p->machine, func, arg);		

	/* Add to list. */
	for (pp = proc_list; pp->next; pp = pp->next);
	pp->next = p;
	
	return &p->proc;
}

