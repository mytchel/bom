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

	freepath(current->dot);	
	freefgroup(current->fgroup);
	freengroup(current->ngroup);
	
	procremove(current);
	schedule();
	
	/* Never reached. */
	return ERR;
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
	int flags;
	bool copy;
	
	flags = va_arg(args, int);

	p = newproc();
	if (p == nil)
		return ERR;

	p->parent = current;
	p->quanta = current->quanta;
	p->dot = copypath(current->dot);

	copy = flags & FORK_cmem;
	for (i = 0; i < Smax; i++) {
		if (current->segs[i] != nil) {
			p->segs[i] = copyseg(current->segs[i], copy || (i == Sstack));
			if (p->segs[i] == nil)
				return ERR;
		} else {
			p->segs[i] = nil;
		}
	}
	
	if (flags & FORK_cfgroup) {
		p->fgroup = copyfgroup(current->fgroup);
	} else {
		p->fgroup = current->fgroup;
		p->fgroup->refs++;
	}
	
	if (flags & FORK_cngroup) {
		p->ngroup = copyngroup(current->ngroup);
	} else {
		p->ngroup = current->ngroup;
		p->ngroup->refs++;
	}
	
	forkchild(p, current->ureg);
	procready(p);

	return p->pid;
}

int
sysgetpid(va_list args)
{
	return current->pid;
}
