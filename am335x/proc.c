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

void
dumpregs(struct label *r)
{
  int i;
  for (i = 0; i < 13; i++) {
    printf("r%i  = 0x%h\n", i, r->regs[i]);
  }
  
  printf("sp   = 0x%h\n", r->sp);
  printf("lr   = 0x%h\n", r->lr);
  printf("psr  = 0b%b\n", r->psr);
  printf("pc   = 0x%h\n", r->pc);
}

void
forkfunc(struct proc *p, int (*func)(void *), void *arg)
{
  int i;
  for (i = 0; i < 13; i++)
    p->label.regs[i] = i;

  p->label.psr = MODE_SVC;
  p->label.sp = (uint32_t) p->kstack->pa + PAGE_SIZE;

  p->label.pc = (uint32_t) func;
  p->label.regs[0] = (uint32_t) arg;
}

reg_t
forkchild(struct proc *p)
{
  intrstate_t i;
  uint32_t s, d;
  
  i = setintr(INTR_OFF);
  
  if (setlabel(&p->label)) {
    setintr(i);
    return 0;
  }

  setintr(i);
  
  s = p->label.sp;
  d = up->kstack->pa + PAGE_SIZE - s;

  p->label.sp = p->kstack->pa + PAGE_SIZE - d;
  
  memmove((void *) p->label.sp, (void *) s, d);

  i = setintr(INTR_OFF);
  procready(p);
  setintr(i);

  return p->pid;
}

void
readyexec(struct label *ureg, void *entry,
	  int argc, char *argv[])
{
  char **largv;
  size_t l;
  int i;
  
  memset(ureg, 0, sizeof(struct label));

  ureg->psr = MODE_USR;
  ureg->pc = (uint32_t) entry;
  ureg->sp = USTACK_TOP;

  ureg->regs[0] = argc;

  ureg->sp -= sizeof(char *) * argc;
  
  ureg->regs[1] = ureg->sp;
  largv = (char **) ureg->sp;

  for (i = 0; i < argc; i++) {
    l = strlen(argv[i]) + 1;

    ureg->sp -= l;
    ureg->sp -= ureg->sp % sizeof(void *);
    
    largv[i] = (char *) ureg->sp;
    memmove((void *) ureg->sp, argv[i], l);
  }
}
