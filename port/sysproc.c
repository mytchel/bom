#include "dat.h"

int
sysexit(va_list args)
{
	int i;
	struct segment *s;
	
	int code = va_arg(args, int);
	
	kprintf("pid %i exited with status %i\n", 
		current->pid, code);

	for (i = 0; i < Smax; i++) {
		s = current->segs[i];
		if (s != nil)
			freeseg(s);
	}
	
	current->fgroup->refs--;
	if (current->fgroup->refs == 0) {
		kfree(current->fgroup->files);
		kfree(current->fgroup);
	}
	
	procremove(current);
	schedule();
	
	/* Never reached. */
	return SYS_ERR;
}

int
syssleep(va_list args)
{
	int ms;
	ms = va_arg(args, int);

	if (ms <= 0) {
		schedule();
		return 0;
	}

	procsleep(current, ms);
		
	return 0;
}

int
sysfork(va_list args)
{
	int i;
	struct proc *p;
	/*
	int flags;
	
	flags = va_arg(args, int);
	*/
	p = newproc();
	if (p == nil)
		return -1;

	for (i = 0; i < Smax; i++) {
		if (current->segs[i] != nil) {
			p->segs[i] = copyseg(current->segs[i], true);
			if (p->segs[i] == nil)
				return -1;
		}
	}

	forkchild(p, current->ureg);
	
	p->parent = current;
	p->quanta = current->quanta;
	
	p->fgroup = current->fgroup;
	p->fgroup->refs++;

	procready(p);
	return p->pid;
}

int
sysgetpid(va_list args)
{
	return current->pid;
}
