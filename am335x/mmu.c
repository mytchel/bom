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
initmmu(void)
{
  int i;
  for (i = 0; i < 4096; i++)
    ttb[i] = L1_FAULT;

  mmuloadttb(ttb);
}

void
mmuswitch(struct proc *p)
{
  struct page *pg;

  mmuempty1();

  for (pg = p->mmu; pg != nil; pg = pg->next) {
    ttb[L1X((uint32_t) pg->va)] 
      = ((uint32_t) pg->pa) | L1_COARSE;
  }
	
  mmuinvalidate();
}

void
imap(void *start, void *end, int ap, bool cachable)
{
  uint32_t x = (uint32_t) start & ~((1 << 20) - 1);
  uint32_t mask = (ap << 10) | L1_SECTION;

  if (cachable) {
    mask |= (0 << 16) | (6 << 12) | (1 << 3) | (1 << 2);
  } else {
    mask |= (0 << 16) | (0 << 12) | (0 << 3) | (0 << 2);
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
mmuputpage(struct page *p, bool rw)
{
  struct page *pg;
  uint32_t i;
  uint32_t s, tex, ap, c, b;
  uint32_t *l1, *l2;

  uint32_t x = (uint32_t) p->va;

  l1 = &ttb[L1X(x)];
	
  /* Add a l1 page if needed. */
  if (*l1  == L1_FAULT) {
    pg = newpage((void *) (x & ~((1 << 20) - 1)));

    for (i = 0; i < 256; i++)
      ((uint32_t *) pg->pa)[i] = L2_FAULT;

    pg->next = current->mmu;
    current->mmu = pg;
		
    *l1 = ((uint32_t) pg->pa) | L1_COARSE;
  }
	
  l2 = (uint32_t *) (*l1 & ~((1 << 10) - 1));

  if (rw)
    ap = AP_RW_RW;
  else
    ap = AP_RW_RO;

  s = 1;
  if (p->cachable) {
    tex = 7;
    c = 1;
    b = 1;
  } else {
    tex = 0;
    c = 0;
    b = 1;
  }
  
  l2[L2X(x)] = ((uint32_t) p->pa) | L2_SMALL | 
    (s << 10) | (tex << 6) | (ap << 4) | (c << 3) | (b << 2);
	
  mmuinvalidate();
}
