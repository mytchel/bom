#include "dat.h"
#include "fns.h"

extern char *initcode;
extern int initcodelen;

static void
initmainproc();

static void
initnullproc();

int
kmain(void)
{
	puts("  --  Bom Booting  --\n\n");

	initwatchdog();
	initmemory();
	initintc();
	inittimers();

	kprintf("set up procs\n");
	initprocs();
	initnullproc();
	initmainproc();
	
	kprintf("schedule!\n");
	schedule();
	
	/* Should never be reached. */
	return 0;
}

int
mainproc(void)
{
	kprintf("droptouser\n");
	droptouser((void *) USTACK_TOP);
	return 0; /* Never reached. */
}

void
initmainproc(void)
{
	struct proc *p;
	struct segment *s;
	
	p = newproc();
	
	forkfunc(p, &mainproc);

	s = newseg(SEG_ro, (void *) UTEXT, PAGE_SIZE);
	p->segs[Stext] = s;
	s->pages = newpage((void *) UTEXT);

	s = newseg(SEG_rw, (void *) UTEXT + PAGE_SIZE, PAGE_SIZE);
	p->segs[Sdata] = s;
	s->pages = newpage((void *) UTEXT + PAGE_SIZE);

	s = newseg(SEG_rw, (void *) (USTACK_TOP - USTACK_SIZE), USTACK_SIZE);
	p->segs[Sstack] = s;
	s->pages = newpage((void *) (USTACK_TOP - USTACK_SIZE));

	/* Copies text segment (and rodata?), hopefully data segment has nothing that doesnt get
	 * initialised by a function */
	memmove(p->segs[Stext]->pages->pa, &initcode, initcodelen);
	
	p->fgroup = newfgroup();
	p->ngroup = newngroup();
	
	procready(p);
}

int
nullproc(void)
{
	while (true)
		schedule();
	return 0;
}

void
initnullproc(void)
{
	struct proc *p;
	
	p = newproc();

	forkfunc(p, &nullproc);	

	procready(p);
}

