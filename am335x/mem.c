#include "dat.h"
#include "fns.h"

static void
initpages(void *, void *, void *, void *);

struct page_ref {
	void *addr;
	uint8_t users;
	struct page_ref *next;
};

extern uint32_t *_heap_start;
extern uint32_t *_heap_end;
extern uint32_t *_ram_start;
extern uint32_t *_ram_end;
extern uint32_t *_kernel_start;
extern uint32_t *_kernel_end;

static struct page_ref *pages, *firstfree;

void
initmemory(void)
{
	uint32_t ram_size, heap_size;

	ram_size = (uint32_t) &_ram_end - (uint32_t) &_ram_start;
	heap_size = (uint32_t) &_heap_end - (uint32_t) &_heap_start;

	kprintf("ram size      = %i MB\n", ram_size / 1024 / 1024);
	kprintf("heap size     = %i MB\n", heap_size / 1024 / 1024);

	initheap(&_heap_start, heap_size);
	
	initpages((void *) &_ram_start, (void *) &_ram_end,
		(void *) &_kernel_start, (void *) &_kernel_end);
		
	initmmu();

	/* Give kernel unmapped access to all of ram. */	
	imap(&_ram_start, &_ram_end, AP_RW_NO);
	/* Give everybody access to io. This will be changed. */
	imap((void *) 0x40000000, (void *) 0x4A400000, AP_RW_RW);
	
	mmuenable();
}

void
initpages(void *start, void *end,
	void *kernel_start, void *kernel_end)
{
	int i;
	void *addr;
	struct page_ref *prev;
	uint32_t ramsize, npages;
	
	start = (void *) ((uint32_t) start & PAGE_MASK);
	
	ramsize = (uint32_t) end - (uint32_t) start;
	npages = ramsize / PAGE_SIZE;
	
	kprintf("page table size = %i KB\n", 
		sizeof(struct page_ref) * npages / 1024);
	
	pages = kmalloc(sizeof(struct page_ref) * npages);
	if (pages == nil) {
		kprintf("Failed to allocate page table.\n");
		while (1);
	}
	
	prev = nil;
	for (i = 0; i < npages; i++) {
		addr = (void *) ((uint32_t) start + i * PAGE_SIZE);
		pages[i].addr = addr;
		
		if (kernel_start <= addr && addr <= kernel_end) {
			pages[i].users = 1;
		} else {
			pages[i].users = 0;
			if (prev)
				prev->next = &pages[i];
			else
				firstfree = &pages[i];
			prev = &pages[i];
		}
	}
	
	prev->next = nil;
}

struct page *
newpage(void *va)
{
	struct page *p;
	struct page_ref *r;
	
	if (firstfree == nil) {
		kprintf("No free pages!\n");
		return nil;
	}

	p = kmalloc(sizeof(struct page));
	if (p == nil)
		return nil;
	
	r = firstfree;
	firstfree = r->next;	

	r->users = 1;
	
	p->pa = r->addr;
	p->va = va;
	p->size = PAGE_SIZE;
	p->next = nil;

	return p;
}

void
untagpage(void *addr)
{
	uint32_t i;
	struct page_ref *r;
	
	i = (uint32_t) (addr - pages->addr) >> 12;
	r = &pages[i];
	
	r->users--;
	
	/* Add to free list. */
	if (r->users == 0) {
		r->next = firstfree;
		firstfree = r;
	}
}

void
tagpage(void *addr)
{
	uint32_t i;
	i = (uint32_t) (addr - pages->addr) >> 12;
	pages[i].users++;
}
