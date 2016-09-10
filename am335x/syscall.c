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

