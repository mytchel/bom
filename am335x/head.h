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

#include "trap.h"

#define PAGE_SHIFT 	 12
#define PAGE_SIZE	 (1UL << PAGE_SHIFT)
#define PAGE_MASK	 (~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x)    (((x) + PAGE_SIZE - 1) & PAGE_MASK)

#define USTACK_TOP	 0x20000000
#define UTEXT		 0

#define TICKS_MIN        20
#define QUANTA_MAX      100
#define QUANTA_DEF       50
#define QUANTA_MIN       10

typedef enum {
  INTR_on  = (uint32_t) 0,
  INTR_off = (uint32_t) MODE_DI,
} intrstate_t;

struct label {
  uint32_t psr, sp, lr;
  uint32_t regs[13];
  uint32_t pc;
} __attribute__((__packed__));

struct mmu {
  struct pagel *pages;
};

