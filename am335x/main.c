#include "dat.h"
#include "mem.h"
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

static void
initmainproc();

void
droptouser(uint32_t sp);

int
main(void)
{
	kprintf("in main\n");
	
	watchdoginit();
	memoryinit();
	intcinit();

	procsinit();
	initmainproc();

	schedule();
	
	/* Should never be reached. */
	return 0;
}

void
mainproc(void)
{
	int i;
	kprintf("in main proc\n");
	
	for (i = 0; i < 2; i++) {
		kprintf("yield for fun\n");
		yield();
	}
	
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

	kprintf("init stack segment\n");	
	s = newseg(SEG_stack, (void *) (USTACK_TOP - USTACK_SIZE), USTACK_SIZE/PAGE_SIZE);
	p->segs[SEG_stack] = s;
	pg = newpage((void *) (0xa0000000 - USTACK_SIZE));
	kprintf("stack page pa = 0x%h\n", (uint32_t) pg->pa);
	s->pages = pg;

	kprintf("init text segment\n");	
	s = newseg(SEG_text, (void *) UTEXT, 1);
	p->segs[SEG_text] = s;
	pg = newpage((void *) UTEXT);
	kprintf("text page pa = 0x%h\n", (uint32_t) pg->pa);
	s->pages = pg;

	kprintf("copy initcode\n");
	kprintf("initcode size = %i\n", sizeof(initcode));
	memmove(s->pages->pa, initcode, sizeof(initcode));
	kprintf("done\n");
	
	p->state = PROC_ready;
}

