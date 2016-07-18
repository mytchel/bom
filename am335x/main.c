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

	initprocs();
	initnullproc();
	initmainproc();
	
	kprintf("Start Proc 1\n");
	schedule();
	
	/* Should never be reached. */
	return 0;
}

int
mainproc(void)
{
	droptouser((void *) USTACK_TOP);
	return 0; /* Never reached. */
}

void
initmainproc(void)
{
	struct proc *p;
	struct segment *s;
	struct page *pg;
	
	p = newproc();
	
	forkfunc(p, &mainproc);

	s = newseg(SEG_rw, (void *) (USTACK_TOP - USTACK_SIZE), USTACK_SIZE);
	p->segs[Sstack] = s;
	s->pages = newpage((void *) (USTACK_TOP - USTACK_SIZE));
	

	s = newseg(SEG_ro, (void *) UTEXT, PAGE_SIZE);
	p->segs[Stext] = s;
	s->pages = newpage((void *) UTEXT);
	
	pg = s->pages;
	while (true) {
		memmove(pg->pa, &initcode,
			initcodelen > PAGE_SIZE ? PAGE_SIZE : initcodelen);
			
		initcodelen -= PAGE_SIZE;
		initcode += PAGE_SIZE;
		
		if (initcodelen > 0) {
			pg->next = newpage(pg->va + PAGE_SIZE);
			pg = pg->next;
		} else {
			pg->next = nil;
			break;
		}
	}
	
	p->fgroup = newfgroup();
	p->ngroup = newngroup();
	
	procready(p);
}

int
nullproc(void)
{
	while (true) {
		schedule();
	}
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

