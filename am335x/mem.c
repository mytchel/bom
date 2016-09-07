#include "head.h"
#include "fns.h"

static void
addpages(struct page *, uint32_t, uint32_t);

extern uint32_t *_heap_start;
extern uint32_t *_heap_end;
extern uint32_t *_ram_start;
extern uint32_t *_ram_end;
extern uint32_t *_kernel_start;
extern uint32_t *_kernel_end;

struct page pages = {0};
struct page iopages = {0};

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

	addpages(&iopages, 0x47400000, 0x47410000);
	addpages(&iopages, 0x44C00000, 0x44F00000);
	addpages(&iopages, 0x48000000, 0x48300000);
	addpages(&iopages, 0x4A100000, 0x4A110000);

	initmmu();

	/* Give kernel unmapped access to all of ram. */	
	imap(&_ram_start, &_ram_end, AP_RW_NO);

	/* Give kernel access to most IO, hope it doesnt touch it. */
	imap((void *) 0x44000000, (void *) 0x50000000, AP_RW_NO);
	
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
			p[i].from = pages;
			start += PAGE_SIZE;
		}
		
		p[i].pa = (void *) start;
		p[i].va = 0;
		p[i].size = PAGE_SIZE;
		p[i].next = pages->next;
		p[i].from = pages;
		start += PAGE_SIZE;
		
		pages->next = p;
	}
}

void *
pagealign(void *addr)
{
	uint32_t x = (uint32_t) addr;
	return (void *) PAGE_ALIGN(x);
}
