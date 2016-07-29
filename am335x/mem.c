#include "dat.h"
#include "fns.h"

static void
addpages(struct page *, uint32_t, uint32_t);

extern uint32_t *_heap_start;
extern uint32_t *_heap_end;
extern uint32_t *_ram_start;
extern uint32_t *_ram_end;
extern uint32_t *_kernel_start;
extern uint32_t *_kernel_end;

static struct page pages = { 0, 0, 0, nil};
static struct page iopages = { 0, 0, 0, nil};

void
initmemory(void)
{
	uint32_t ram_size, heap_size;

	ram_size = (uint32_t) &_ram_end - (uint32_t) &_ram_start;
	heap_size = (uint32_t) &_heap_end - (uint32_t) &_heap_start;

	printf("ram size\t\t= %i MB\n", ram_size / 1024 / 1024);

	initheap(&_heap_start, heap_size);

	addpages(&pages, PAGE_ALIGN((uint32_t) &_kernel_end), (uint32_t) &_ram_end);
	addpages(&pages, (uint32_t) &_ram_start, PAGE_ALIGN((uint32_t) &_kernel_start));
	addpages(&iopages, 0x44000000, 0x45000000);
			
	initmmu();

	/* Give kernel unmapped access to all of ram. */	
	imap(&_ram_start, &_ram_end, AP_RW_NO);
	/* Give kernel access to all io. This will be changed. */
	imap((void *) 0x40000000, (void *) 0x4A400000, AP_RW_RW);
	
	mmuenable();
}

void
addpages(struct page *pages, uint32_t start, uint32_t end)
{
	struct page *p;
	uint32_t i, npages;
	
	while (start < end) {
		npages = PAGE_SIZE / sizeof(struct page);
		if (npages * PAGE_SIZE > end - start) {
			npages = (uint32_t) (end - start) / PAGE_SIZE;
		}
		
		p = malloc(sizeof(struct page) * npages);
		if (p == nil) {
			printf("Failed to allocate pages for 0x%h to 0x%h\n", start, end);
			break;
		}
		
		for (i = 0; i < npages - 1; i++) {
			p[i].pa = (void *) start;
			p[i].va = 0;
			p[i].size = PAGE_SIZE;
			p[i].next = &p[i+1];
			start += PAGE_SIZE;
		}
		
		p[i].pa = (void *) start;
		p[i].va = 0;
		p[i].size = PAGE_SIZE;
		p[i].next = pages->next;
		start += PAGE_SIZE;
		
		pages->next = p;
	}
}

struct page *
newpage(void *va)
{
	struct page *p;

	p = pages.next;
	if (p == nil) {
		printf("No free pages!\n");
		return nil;
	}

	pages.next = p->next;
	p->next = nil;
	p->va = va;
	return p;
}

struct page *
getiopages(void *pa, size_t *size)
{
	struct page *start, *p, *pp;
	size_t s;
	
	pp = &iopages;
	for (p = pp->next; p != nil; pp = p, p = p->next) {
		if (p->pa == pa) {
			break;
		}
	}
	
	if (p == nil)
		return nil;
	
	start = p;
	s = 0;
	while (p != nil) {
		s += p->size;
		if (s >= *size) {
			break;
		} else {
			p = p->next;
		}
	}

	if (p != nil) {
		pp->next = p->next;
		p->next = nil;
	}
	
	*size = s;	
	return start;
}

void
freepage(struct page *p)
{
	p->next = pages.next;
	pages.next = p;
}

void *
pagealign(void *addr)
{
	uint32_t x = (uint32_t) addr;
	return (void *) PAGE_ALIGN(x);
}
