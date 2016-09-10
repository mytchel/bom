/*
 *   Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

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

	addpages(&iopages, 0x47400000, 0x47404000); /* USB */
	addpages(&iopages, 0x47810000, 0x47820000); /* MMCHS2 */
	addpages(&iopages, 0x50000000, 0x51000000); /* GPMC */
	addpages(&iopages, 0x44E05000, 0x44E06000); /* DMTimer0 */
	addpages(&iopages, 0x44E07000, 0x44E08000); /* GPIO0 */
	addpages(&iopages, 0x44E09000, 0x44E0A000); /* UART0 */
	addpages(&iopages, 0x44E31000, 0x44E32000); /* DMTimer1 */
	addpages(&iopages, 0x44E35000, 0x44E36000); /* Watchdog */
	addpages(&iopages, 0x48022000, 0x48023000); /* UART1 */
	addpages(&iopages, 0x48024000, 0x48025000); /* UART2 */
	addpages(&iopages, 0x48040000, 0x48041000); /* DMTIMER2 */
	addpages(&iopages, 0x48042000, 0x48043000); /* DMTIMER3 */
	addpages(&iopages, 0x4804c000, 0x4804d000); /* GPIO1 */
	addpages(&iopages, 0x48060000, 0x48061000); /* MMCHS0 */
	addpages(&iopages, 0x481ac000, 0x481ad000); /* GPIO2 */
	addpages(&iopages, 0x481AE000, 0x481AF000); /* GPIO3 */
	addpages(&iopages, 0x481D8000, 0x481D9000); /* MMC1 */
	addpages(&iopages, 0x48200000, 0x48201000); /* INTCPS */

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
