#include "dat.h"
#include "../port/com.h"
#include "../include/std.h"

extern char *initcode;
extern int initcodelen;

int
kmain(void);

void
watchdoginit(void);

void
memoryinit(void);

void
intcinit();

void
systickinit();

static void
initmainproc();

static void
initnullproc();

void
droptouser(uint32_t sp);

int
main(void)
{
	kprintf("  --  Bom Booting  --\n\n");
	
	watchdoginit();
	memoryinit();
	intcinit();
	systickinit();

	procsinit();
	initnullproc();
	initmainproc();

	schedule();
	
	/* Should never be reached. */
	return 0;
}

void
mainproc(void)
{
	kprintf("in main proc\n");
	kprintf("drop to user\n");
	droptouser(USTACK_TOP);
}

void
initmainproc()
{
	struct proc *p;
	struct segment *s;
	
	kprintf("init main proc\n");

	p = newproc();
	
	p->label.sp = (uint32_t) p->kstack + KSTACK;
	p->label.pc = (uint32_t) &mainproc;

	s = newseg(SEG_RW, (void *) (USTACK_TOP - USTACK_SIZE), USTACK_SIZE/PAGE_SIZE);
	p->segs[Sstack] = s;
	s->pages = newpage((void *) (USTACK_TOP - USTACK_SIZE));

	s = newseg(SEG_RO, (void *) UTEXT, 1);
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

