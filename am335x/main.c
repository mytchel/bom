#include "dat.h"
#include "../port/com.h"
#include "init.h"
#include "../include/std.h"

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
	struct page *pg;
	
	kprintf("init main proc\n");

	p = newproc();
	
	p->label.sp = (uint32_t) p->kstack + KSTACK;
	p->label.pc = (uint32_t) &mainproc;

	s = newseg(SEG_stack, (void *) (USTACK_TOP - USTACK_SIZE), USTACK_SIZE/PAGE_SIZE);
	p->segs[SEG_stack] = s;
	pg = newpage((void *) (USTACK_TOP - USTACK_SIZE));
	s->pages = pg;

	s = newseg(SEG_text, (void *) UTEXT, 1);
	p->segs[SEG_text] = s;
	pg = newpage((void *) UTEXT);
	s->pages = pg;

	kprintf("sizeof(initcode) = %i\n", sizeof(initcode));
	kprintf("page size = %i\n", pg->size);
	memmove(s->pages->pa, initcode, sizeof(initcode));
	
	p->state = PROC_ready;
}

