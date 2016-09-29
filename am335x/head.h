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

#ifndef _HEAD_H_
#define _HEAD_H_

#include <types.h>

#define disableintr()   __asm__("cpsid i")
#define enableintr()    __asm__("cpsie i")

#define MAX_PROCS	512
#define KSTACK		4028

#define USTACK_TOP	(0x20000000 & PAGE_MASK)
#define USTACK_SIZE	PAGE_SIZE
#define UTEXT		0

#define CHUNK_POWER_MAX 10

#define MAX_MEM_SIZE	1024 * 1024 * 32

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
