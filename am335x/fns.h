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

#ifndef __FNS_H
#define __FNS_H

#include "trap.h"

#define readl(a)	(*(volatile uint32_t*)(a))
#define readw(a)	(*(volatile uint16_t*)(a))
#define readb(a)	(*(volatile uint8_t*)(a))

#define writel(v, a)	(*(volatile uint32_t*)(a) = (uint32_t)(v))
#define writew(v, a)	(*(volatile uint16_t*)(a) = (uint16_t)(v))
#define writeb(v, a)	(*(volatile uint8_t*)(a) = (uint8_t)(v))

#define PAGE_SHIFT 	12
#define PAGE_SIZE	(1UL << PAGE_SHIFT)
#define PAGE_MASK	(~(PAGE_SIZE - 1))
#define PAGE_ALIGN(x) 	(((x) + PAGE_SIZE - 1) & PAGE_MASK)

#define AP_NO_NO	0
#define AP_RW_NO	1
#define AP_RW_RO	2
#define AP_RW_RW	3

void
uregret(struct ureg *);

void
droptouser(void *);

void *
forkfunc_preloader(void *, void *, void *);

void
forkfunc_loader(void);

uint32_t
fsrstatus(void);

void *
faultaddr(void);

void
intcaddhandler(uint32_t, bool (*)(uint32_t));

void
mmuinvalidate(void);

void
imap(void *, void *, int, bool);
	
void
mmuempty1(void);

void
mmuloadttb(uint32_t *);

/* Initialisation functions */

void
initintc(void);

void
initmemory(void);

void
initmmu(void);

void
initwatchdog(void);

void
inittimers(void);

#endif
