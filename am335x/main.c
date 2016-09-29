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

#include "head.h"
#include "fns.h"

extern char *initcodetext, *initcodedata;
extern int initcodetextlen, initcodedatalen;

static void
initmainproc(void);

static void
initnullproc(void);

int
main(void)
{
  puts("Bom Booting...\n");

  initmemory();
  initintc();
  inittimers();
  initwatchdog();

  initnullproc();
  initroot();
  initmainproc();

  puts("Starting procs...\n");
  
  schedule();
	
  /* Should never be reached. */
  return 0;
}

int
mainproc(void *arg)
{
  disableintr();
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
  while (true) {

  }

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
}
