#include "dat.h"
#include "fns.h"

static void
addpages(void *, void *);

extern uint32_t *_heap_start;
extern uint32_t *_heap_end;
extern uint32_t *_ram_start;
extern uint32_t *_ram_end;
extern uint32_t *_kernel_start;
extern uint32_t *_kernel_end;

static struct page *pages = nil;

void
initmemory(void)
{
	uint32_t ram_size, heap_size;

	ram_size = (uint32_t) &_ram_end - (uint32_t) &_ram_start;
	heap_size = (uint32_t) &_heap_end - (uint32_t) &_heap_start;

	kprintf("ram size      		= %i MB\n", ram_size / 1024 / 1024);
	kprintf("init heap size		= %i MB\n", heap_size / 1024 / 1024);

	initheap(&_heap_start, heap_size);

	addpages((void *) &_ram_start, (void *) PAGE_ALIGN((uint32_t) &_kernel_start));
	addpages((void *) PAGE_ALIGN((uint32_t) &_kernel_end), (void *) &_ram_end);
		
	initmmu();

	/* Give kernel unmapped access to all of ram. */	
	imap(&_ram_start, &_ram_end, AP_RW_NO);
	/* Give everybody access to io. This will be changed. */
	imap((void *) 0x40000000, (void *) 0x4A400000, AP_RW_RW);
	
	mmuenable();
}

void
addpages(void *start, void *end)
{
	struct page *p;
	uint32_t i, size, npages;
	
	size = (uint32_t) end - (uint32_t) start;
	npages = size / PAGE_SIZE;
	
	p = kmalloc(sizeof(struct page) * npages);
	
	p[0].pa = start;
	p[0].va = 0;
	p[0].size = PAGE_SIZE;
	
	for (i = 1; i < npages; i++) {
		p[i].pa = (void *) ((uint32_t) start + i * PAGE_SIZE);
		p[i].size = PAGE_SIZE;
		p[i-1].next = &p[i];
	}

	p[i-1].next = pages;
	
	pages = p;
}

struct page *
newpage(void *va)
{
	struct page *p;
	
	p = pages;
	if (p == nil) {
		kprintf("No free pages!\n");
		return nil;
	}

	pages = p->next;
	p->next = nil;
	p->va = va;
	return p;
}

void
freepage(struct page *p)
{
	p->next = pages;
	pages = p;
}
