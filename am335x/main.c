#include "dat.h"
#include "mem.h"
#include "../port/com.h"

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
initmainproc()
{
	struct proc *p;
	struct segment *s;
	struct page *pg;
	
	kprintf("init main proc\n");
	
	p = newproc();
	
	p->label.sp = (uint32_t) p->kstack + KSTACK;
	p->label.pc = (uint32_t) &kmain;

	kprintf("init stack segment\n");	
	s = newseg(SEG_stack, (void *) (0xa0000000-0x1000), 0x1000);
	p->segs[SEG_stack] = s;
	kprintf("get a page\n");
	pg = newpage((void *) (0xa0000000 - 0x1000));
	kprintf("page pa = 0x%h\n", (uint32_t) pg->pa);
	s->pages = pg;
	
	p->state = PROC_ready;
	
	kprintf("ok\n");
}

