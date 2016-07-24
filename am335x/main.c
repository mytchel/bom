#include "dat.h"
#include "fns.h"

extern char *initcode;
extern int initcodelen;

static void
initmainproc(void);

static void
initnullproc(void);

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
mainproc(void *arg)
{
	kprintf("Drop to user\n");
	
	droptouser((void *) USTACK_TOP);
	return 0; /* Never reached. */
}

void
initmainproc(void)
{
	struct proc *p;
	struct segment *s;
	struct page *pg;
	size_t l, codelen, ci;
	
	p = newproc();
	
	forkfunc(p, &mainproc, nil);

	s = newseg(SEG_rw);
	p->segs[Sstack] = s;
	s->pages = newpage((void *) (USTACK_TOP - USTACK_SIZE));

	s = newseg(SEG_ro);
	p->segs[Stext] = s;
	s->pages = newpage((void *) UTEXT);

	codelen = initcodelen;
	ci = 0;
	
	pg = s->pages;
	while (true) {
		l = codelen > PAGE_SIZE ? PAGE_SIZE : codelen;
		
		memmove(pg->pa, (uint8_t *) &initcode + ci, l);
			
		codelen -= l;
		ci += l;
		
		if (codelen > 0) {
			pg->next = newpage(pg->va + pg->size);
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
nullproc(void *arg)
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

	forkfunc(p, &nullproc, nil);

	procready(p);
}
