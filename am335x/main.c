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
	
	initmemory();
	initintc();
	inittimers();
	initwatchdog();

	initprocs();
	initnullproc();
	initmainproc();

	schedule();
	
	/* Should never be reached. */
	return 0;
}

void
mainproc(void)
{
	droptouser((void *) USTACK_TOP);
}

void
initmainproc()
{
	struct proc *p;
	struct segment *s;
	
	p = newproc();
	
	p->label.sp = (uint32_t) p->kstack + KSTACK;
	p->label.pc = (uint32_t) &mainproc;

	s = newseg(SEG_RW, (void *) (USTACK_TOP - USTACK_SIZE), USTACK_SIZE);
	p->segs[Sstack] = s;
	s->pages = newpage((void *) (USTACK_TOP - USTACK_SIZE));

	s = newseg(SEG_RO, (void *) UTEXT, PAGE_SIZE);
	p->segs[Stext] = s;
	s->pages = newpage((void *) UTEXT);

	memmove(s->pages->pa, &initcode, initcodelen);
	
	procready(p);
}

void
nullproc()
{
	while (true)
		schedule();
}

void
initnullproc()
{
	struct proc *p;
	
	p = newproc();
	
	p->label.sp = (uint32_t) p->kstack + KSTACK;
	p->label.pc = (uint32_t) &nullproc;

	procready(p);
}

