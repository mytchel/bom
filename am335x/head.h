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

#ifndef __DAT_H
#define __DAT_H

#include <types.h>

#define MAX_PROCS	512
#define KSTACK		4028

#define USTACK_TOP	(0x20000000 & PAGE_MASK)
#define USTACK_SIZE	PAGE_SIZE
#define UTEXT		0

#define MAX_MEM_SIZE	1024 * 1024 *32

struct label {
  uint32_t psr;
  uint32_t regs[9];
  uint32_t sp, pc;
};

struct ureg {
  uint32_t regs[13];
  uint32_t sp, lr;
  uint32_t type, psr, pc;
};

#include "../port/port.h"

#endif
