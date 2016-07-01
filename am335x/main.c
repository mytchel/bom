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
	
	s = newseg(SEG_RW, (void *) UTEXT + PAGE_SIZE, PAGE_SIZE);
	p->segs[Sdata] = s;
	s->pages = newpage((void *) UTEXT + PAGE_SIZE);

	/* Copies text segment (and rodata?), hopefully data segment has nothing that doesnt get
	 * initialised by a function */
	memmove(p->segs[Stext]->pages->pa, &initcode, initcodelen);
	
	p->fgroup = kmalloc(sizeof(struct fgroup));
	p->fgroup->files = kmalloc(sizeof(struct pipe*));
	p->fgroup->files[0] = nil;
	p->fgroup->nfiles = 1;
	
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

