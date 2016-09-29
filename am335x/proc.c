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

void
dumpregs(struct ureg *r)
{
  int i;
  for (i = 0; i < 13; i++)
    printf("r%i  = 0x%h\n", i, r->regs[i]);
  printf("sp   = 0x%h\n", r->sp);
  printf("lr   = 0x%h\n", r->lr);
  printf("type = %i\n", r->type);
  printf("pc   = 0x%h\n", r->pc);
  printf("psr  = 0b%b\n", r->psr);
}

void
forkfunc(struct proc *p, int (*func)(void *), void *arg)
{
  int i;

  for (i = 0; i < 9; i++)
    p->label.regs[i] = 0;
	
  p->label.psr = MODE_SVC;
  p->label.pc = (uint32_t) &forkfunc_loader;
  p->label.sp = (uint32_t)
    forkfunc_preloader((void *)
		       (p->kstack + KSTACK),
		       arg, func);
}

void
forkchild(struct proc *p, struct ureg *ureg)
{
  struct ureg *nreg;
  int i;

  for (i = 0; i < 9; i++)
    p->label.regs[i] = 0;

  p->inkernel = false;

  /* SVC with interrupts disabled */
  p->label.psr = MODE_SVC | (1 << 7);
  p->label.pc = (uint32_t) &userreturn;
  p->label.sp = (uint32_t) 
    (p->kstack + KSTACK)
    - sizeof(struct ureg);
	
  nreg = (struct ureg *) p->label.sp;
  memmove(nreg, ureg, sizeof(struct ureg));
  nreg->regs[0] = 0;
}
