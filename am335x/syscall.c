#include "head.h"
#include "fns.h"

void
syscall(struct ureg *ureg)
{
  unsigned int sysnum;

  sysnum = (unsigned int) ureg->regs[0];

  current->ureg = ureg;

  if (sysnum < NSYSCALLS) {
    va_list args = (va_list) ureg->sp;
    ureg->regs[0] = syscalltable[sysnum](args);
  } else {
    ureg->regs[0] = -1;
  }
}

