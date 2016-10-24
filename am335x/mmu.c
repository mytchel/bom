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

#define L1X(va)		(va >> 20)
#define L2X(va)		((va >> 12) & ((1 << 8) - 1))

#define L1_FAULT	0
#define L1_COARSE	1
#define L1_SECTION	2
#define L1_FINE		3

#define L2_FAULT	0
#define L2_LARGE	1
#define L2_SMALL	2
#define L2_TINY		3

uint32_t ttb[4096] __attribute__((__aligned__(16*1024))) = { L1_FAULT };

void
mmuinit(void)
{
  int i;
  for (i = 0; i < 4096; i++)
    ttb[i] = L1_FAULT;

  mmuloadttb(ttb);
}

void
mmuswitch(struct proc *p)
{
  struct pagel *pl;

  mmuempty1();

  for (pl = p->mmu; pl != nil; pl = pl->next) {
    ttb[L1X((uint32_t) pl->va)] 
      = ((uint32_t) pl->p->pa) | L1_COARSE;
  }

  mmuinvalidate();
}

void
imap(void *start, void *end, int ap, bool cachable)
{
  uint32_t x = (uint32_t) start & ~((1 << 20) - 1);
  uint32_t mask = (ap << 10) | L1_SECTION;

  if (cachable) {
    mask |= (7 << 12) | (1 << 3) | (0 << 2);
  } else {
    mask |= (0 << 12) | (0 << 3) | (1 << 2);
  }
  
  while (x < (uint32_t) end) {
    /* Map section so everybody can see it.
     * This wil change. */
    ttb[L1X(x)] = x | mask;
    x += 1 << 20;
  }
}

void
mmuempty1(void)
{
  uint32_t i;

  for (i = 0; i < 4096; i++) {
    if ((ttb[i] & L1_COARSE) == L1_COARSE)
      ttb[i] = L1_FAULT;
  }
}

void
mmuputpage(struct pagel *p)
{
  struct pagel *pn;
  struct page *pg;
  uint32_t i;
  uint32_t tex, ap, c, b;
  uint32_t *l1, *l2;

  uint32_t x = (uint32_t) p->va;

  l1 = &ttb[L1X(x)];
	
  /* Add a l1 page if needed. */
  if (*l1  == L1_FAULT) {
    pg = getrampage();
    if (pg == nil) {
      panic("mmu failed to get page\n");
    }
    
    pn = wrappage(pg, (void *) (x & ~((1 << 20) - 1)),
		  true, true);
    if (pn == nil) {
      panic("mmu failed to wrap page\n");
    }
 
    pn->next = up->mmu;
    up->mmu = pn;

    for (i = 0; i < 256; i++)
      ((uint32_t *) pg->pa)[i] = L2_FAULT;
	
    *l1 = ((uint32_t) pg->pa) | L1_COARSE;
  }
	
  l2 = (uint32_t *) (*l1 & ~((1 << 10) - 1));

  if (p->rw) {
    ap = AP_RW_RW;
  } else {
    ap = AP_RW_RO;
  }
  
  if (p->c) {
    tex = 7;
    c = 1;
    b = 0;
    /*
    tex = 7;
    c = 1;
    b = 1;
    */
  } else {
    tex = 0;
    c = 0;
    b = 1;
  }
  
  l2[L2X(x)] = ((uint32_t) p->p->pa) | L2_SMALL | 
    (tex << 6) | (ap << 4) | (c << 3) | (b << 2);
	
  mmuinvalidate();
}
