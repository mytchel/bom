#include "dat.h"
#include "fns.h"

extern char *initcodetext, *initcodedata;
extern int initcodetextlen, initcodedatalen;

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
	
	schedule();
	
	/* Should never be reached. */
	return 0;
}

int
mainproc(void *arg)
{
	droptouser((void *) USTACK_TOP);
	return 0; /* Never reached. */
}

static struct page *
copysegment(struct segment *s, char **buf, size_t len)
{
	struct page *pg;
	size_t i, l;
	
	i = 0;
	pg = s->pages;
	while (true) {
		l = len > PAGE_SIZE ? PAGE_SIZE : len;
		
		memmove(pg->pa, (uint8_t *) buf + i, l);
		
		len -= l;	
		i += l;
		
		if (len > 0) {
			pg->next = newpage(pg->va + pg->size);
			pg = pg->next;
		} else {
			pg->next = nil;
			break;
		}
	}
	
	return pg;
}

void
initmainproc(void)
{
	struct proc *p;
	struct page *pg;
		
	p = newproc();
	
	forkfunc(p, &mainproc, nil);

	p->segs[Sstack]->pages = newpage((void *) (USTACK_TOP - USTACK_SIZE));

	p->segs[Stext]->pages = newpage((void *) UTEXT);
	pg = copysegment(p->segs[Stext], &initcodetext, initcodetextlen);

	p->segs[Sdata]->pages = newpage(pg->va + pg->size);
	copysegment(p->segs[Sdata], &initcodedata, initcodedatalen);
	
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
