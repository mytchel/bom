/*
 *
 * Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "../kern/head.h"
#include "fns.h"

static void
addrampages(uint32_t, uint32_t);

static void
addiopages(uint32_t, uint32_t);

extern uint32_t *_heap_start;
extern uint32_t *_heap_end;
extern uint32_t *_ram_start;
extern uint32_t *_ram_end;
extern uint32_t *_kernel_start;
extern uint32_t *_kernel_end;

static struct page *rampages = nil, *iopages = nil;

void
memoryinit(void)
{
  uint32_t heap_size;

  heap_size = (uint32_t) &_heap_end - (uint32_t) &_heap_start;

  heapinit(&_heap_start, heap_size);

  addrampages(PAGE_ALIGN((uint32_t) &_kernel_end + PAGE_SIZE - 1),
	      (uint32_t) &_ram_end);
  
  addrampages((uint32_t) &_ram_start,
	      PAGE_ALIGN((uint32_t) &_kernel_start));

  addiopages(0x47400000, 0x47404000); /* USB */
  addiopages(0x44E31000, 0x44E32000); /* DMTimer1 */
  addiopages(0x48042000, 0x48043000); /* DMTIMER3 */
  addiopages(0x48022000, 0x48023000); /* UART1 */
  addiopages(0x48024000, 0x48025000); /* UART2 */
  addiopages(0x44E07000, 0x44E08000); /* GPIO0 */
  addiopages(0x4804c000, 0x4804d000); /* GPIO1 */
  addiopages(0x481ac000, 0x481ad000); /* GPIO2 */
  addiopages(0x481AE000, 0x481AF000); /* GPIO3 */
  addiopages(0x44E09000, 0x44E0A000); /* UART0 */
  addiopages(0x48060000, 0x48061000); /* MMCHS0 */
  addiopages(0x481D8000, 0x481D9000); /* MMC1 */
  addiopages(0x47810000, 0x47820000); /* MMCHS2 */

  mmuinit();

  /* Give kernel unmapped access to all of ram. */	
  imap(&_ram_start, &_ram_end, AP_RW_NO, true);

  /* UART0 is given to both kernel and possibly users. This may change */
  /* UART0 */
  imap((void *) 0x44E09000, (void *) 0x44E0A000, AP_RW_NO, false);
  /* Watchdog */
  imap((void *) 0x44E35000, (void *) 0x44E36000, AP_RW_NO, false);
  /* DMTimer0 */
  imap((void *) 0x44E05000, (void *) 0x44E06000, AP_RW_NO, false);
  /* DMTIMER2 */
  imap((void *) 0x48040000, (void *) 0x48041000, AP_RW_NO, false);
  /* INTCPS */
  imap((void *) 0x48200000, (void *) 0x48201000, AP_RW_NO, false);

  mmuenable();
}

static void
initpages(struct page *p, struct page **from, size_t npages,
	  uint32_t start, bool forceshare)
{
  size_t i;

  for (i = 0; i < npages; i++) {
    p[i].refs = 0;
    p[i].pa = start;
    p[i].forceshare = forceshare;
    p[i].next = &p[i+1];
    p[i].from = from;

    start += PAGE_SIZE;
  }

  p[i-1].next = *from;
  *from = &p[0];
}

void
addrampages(uint32_t start, uint32_t end)
{
  struct page *new;
  uint32_t npages, size;

  size = end - start;
  npages = 0;

  new = (struct page *) start;

  while (size > (PAGE_ALIGN(sizeof(struct page) * npages)
		 + PAGE_SIZE * npages)) {
    npages++;
  }

  npages--;

  start = PAGE_ALIGN(start + sizeof(struct page) * npages +
		     PAGE_SIZE - 1);

  initpages(new, &rampages, npages, start, false);
}

void
addiopages(uint32_t start, uint32_t end)
{
  struct page *p;
  size_t npages;

  npages = (end - start) / PAGE_SIZE;

  p = malloc(sizeof(struct page) * npages);

  initpages(p, &iopages, npages, start, true);
}

struct page *
getrampage(void)
{
  struct page *p;
  intrstate_t i;

  i = setintr(INTR_OFF);
  
  p = rampages;
  rampages = p->next;
  p->next = nil;

  setintr(i);

  p->refs = 1;
  return p;
}

struct page *
getiopage(void *addr)
{
  struct page *p, *pp;
  intrstate_t i;

  i = setintr(INTR_OFF);
  
  pp = nil;
  for (p = iopages; p != nil; pp = p, p = p->next) {
    if (p->pa == (reg_t) addr) {
      if (pp == nil) {
	iopages = p->next;
      } else {
	pp->next = p->next;
      }
      
      p->refs = 1;
      break;
    }
  }

  setintr(i);
  return p;
}
