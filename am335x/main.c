#include "dat.h"
#include "fns.h"
#include "../port/com.h"

extern char *initcode;
extern int initcodelen;

static void
initmainproc();

static void
initnullproc();

int
main(void)
{
	puts("  --  Bom Booting  --\n\n");
	
	initwatchdog();
	initmemory();
	initintc();
	inittimers();

	initprocs();
	initnullproc();
	initmainproc();

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

	s = newseg(SEG_RW, (void *) (USTACK_TOP - USTACK_SIZE), USTACK_SIZE);
	p->segs[Sstack] = s;
	s->pages = newpage((void *) (USTACK_TOP - USTACK_SIZE));

	s = newseg(SEG_RO, (void *) UTEXT, PAGE_SIZE);
	p->segs[Stext] = s;
	s->pages = newpage((void *) UTEXT);

	memmove(s->pages->pa, &initcode, initcodelen);
	
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

