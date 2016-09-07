#include "head.h"
#include "fns.h"

void
syscall(struct ureg *ureg)
{
  unsigned int sysnum;

  sysnum = (unsigned int) ureg->regs[0];

  current->ureg = ureg;

  enableintr();
  
  if (sysnum < NSYSCALLS) {
    ureg->regs[0] = syscalltable[sysnum]((va_list) ureg->sp);
  } else {
    ureg->regs[0] = -1;
  }
}

