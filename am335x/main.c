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

extern char *initcodetext, *initcodedata;
extern int initcodetextlen, initcodedatalen;

static void
initmainproc(void);

static void
initnullproc(void);

int
kmain(void)
{
  puts("  --  Bom Booting  --\n\n");

  initmemory();
  initintc();
  inittimers();
  initwatchdog();

  initnullproc();
  initroot();
  initmainproc();

  debug("\nStarting procs\n");
  
  schedule();
	
  /* Should never be reached. */
  return 0;
}

int
mainproc(void *arg)
{
  debug("Drop to user for inital proc (pid = %i)\n", current->pid);
  droptouser((void *) USTACK_TOP);
  return 0; /* Never reached. */
}

static struct page *
copysegment(struct segment *s, char **buf, size_t len)
{
  struct page *pg;
  size_t i, l;
	
  i = 0;
  pg = s->pages;
  while (true) {
    l = len > PAGE_SIZE ? PAGE_SIZE : len;
		
    memmove(pg->pa, (uint8_t *) buf + i, l);
		
    len -= l;	
    i += l;
		
    if (len > 0) {
      pg->next = newpage(pg->va + pg->size);
      pg = pg->next;
    } else {
      pg->next = nil;
      break;
    }
  }
	
  return pg;
}

void
initmainproc(void)
{
  struct proc *p;
  struct page *pg;
		
  p = newproc();
  if (p == nil) {
    printf("Failed to create main proc\n");
    return;
  }
	
  initproc(p);
  forkfunc(p, &mainproc, nil);

  p->segs[Sstack]->pages =
    newpage((void *) (USTACK_TOP - USTACK_SIZE));

  p->segs[Stext]->pages = newpage((void *) UTEXT);
  pg = copysegment(p->segs[Stext], &initcodetext, initcodetextlen);

  p->segs[Sdata]->pages = newpage(pg->va + pg->size);
  copysegment(p->segs[Sdata], &initcodedata, initcodedatalen);
	
  p->fgroup = newfgroup();
  p->ngroup = newngroup();
  p->parent = nil;
	
  procready(p);
}

int
nullproc(void *arg)
{
  while (true);
  return 0;
}

void
initnullproc(void)
{
  struct proc *p;
	
  p = newproc();
  if (p == nil) {
    printf("Failed to create null proc\n");
    return;
  }
	
  initproc(p);
  forkfunc(p, &nullproc, nil);
  procready(p);

  debug("null proc on pid %i\n", p->pid);
}
