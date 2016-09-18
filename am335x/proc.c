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
	for (i = 0; i < 8; i++)
		p->label.regs[i] = 0;
	
	p->label.psr = MODE_SVC;

	p->label.sp = (uint32_t) forkfunc_preloader(
		(void *) (p->kstack + KSTACK - sizeof(uint32_t)),
		arg, func);

	p->label.pc = (uint32_t) &forkfunc_loader;
}

void
forkchild(struct proc *p, struct ureg *ureg)
{
	struct ureg *nreg;

	p->label.pc = (uint32_t) &uregret;
	p->label.sp = (uint32_t) 
		p->kstack + KSTACK - sizeof(uint32_t) - sizeof(struct ureg);
	
	nreg = (struct ureg *) p->label.sp;
	memmove(nreg, ureg, sizeof(struct ureg));
	nreg->regs[0] = 0;
}
