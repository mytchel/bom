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

#define PAGE_SHIFT 	 12
#define PAGE_SIZE	 (1UL << PAGE_SHIFT)
#define PAGE_MASK	 (~(PAGE_SIZE - 1))
#define PAGE_ALIGN_UP(x) (((x) + PAGE_SIZE - 1) & PAGE_MASK)
#define PAGE_ALIGN_DN(x) (((x) - PAGE_SIZE + 1) & PAGE_MASK)

#define disableintr()   __asm__("cpsid i")
#define enableintr()    __asm__("cpsie i")

#define USTACK_TOP	0x20000000
#define UTEXT		0

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
