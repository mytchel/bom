#
#  Copyright (c) <2016> Mytchel Hammond <mytchel@openmailbox.org>
#  
#  Permission is hereby granted, free of charge, to any person
#  obtaining a copy of this software and associated documentation
#  files (the "Software"), to deal in the Software without
#  restriction, including without limitation the rights to use,
#  copy, modify, merge, publish, distribute, sublicense, and/or
#  sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following
#  conditions:
#  
#  The above copyright notice and this permission notice shall be
#  included in all copies or substantial portions of the Software.
#  
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
#  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#  OTHER DEALINGS IN THE SOFTWARE
#

#include "trap.h"

.section .text

.global setintr
.global setlabel
.global gotolabel
.global droptouser
.global funcloader
.global fsrstatus
.global faultaddr
.global cachedisable
.global cacheenable
.global cacheflush
.global mmuloadttb
.global mmuenable
.global mmudisable
.global mmuinvalidate
.global cas
.global cas2
.global atomicinc 
.global atomicdec


setintr:
	push {r1, lr}

	mrs r1, cpsr

	cmp r0, #MODE_DI

	@ disable intr
	orreq r0, r1, #MODE_DI

	@ enable intr
	bicne r0, r1, #MODE_DI
	
	msr cpsr, r0
	
	and r0, r1, #MODE_DI

	pop {r1, pc}


setlabel:
	mrs r1, cpsr
	stmia r0, {r1, sp}
	add r0, r0, #(4 * 7)
	stmia r0, {r4 - r12, lr}

	mov r0, #0
	mov pc, lr
	

gotolabel:
	ldmia r0, {r1, sp}
	add r0, r0, #(4 * 7)
	ldmia r0, {r4 - r12, lr}

	mov r0, #1
	msr cpsr, r1
	mov pc, lr


droptouser:
	mov sp, r1

	cpsid if, #MODE_SYS

	ldmia r0!, {r1, sp, lr}

	cpsid if, #MODE_SVC

	msr spsr, r1
	ldmia r0, {r0 - r12, lr}
	movs pc, lr


funcloader:
	ldr lr, =1f
	ldmia sp!, {r0, pc}
1:	cpsid if
	mov r1, r0
	ldr r0, =up
	bl procexit
	bl schedule

	
fsrstatus:
	mrc p15, 0, r0, c5, c0, 0
	mov pc, lr
	
	
faultaddr:
	mrc p15, 0, r0, c6, c0, 0
	mov pc, lr


cachedisable:
	@ Disable cache and mmu
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #(1<<2) @ unified cache
	mcr p15, 0, r0, c1, c0, 0
	mov pc, lr

	
cacheenable:
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #(1<<2) @ unified cache
	mcr p15, 0, r0, c1, c0, 0
	mov pc, lr
	
	
cacheflush:
	mov r0, #0
	mcr p15, 0, r0, c7, c7, 0
	mov pc, lr


mmuloadttb:
	push {lr}
	mcr p15, 0, r0, c2, c0, 0

	@ set domain mode to client
	mov r0, #1
	mcr p15, 0, r0, c3, c0, 0

	pop {pc}
	

mmuinvalidate:
	mcr p15, 0, r0, c8, c7, 0
	mov pc, lr
	

mmuenable:
	push {lr}
	bl mmuinvalidate
	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #1
	mcr p15, 0, r0, c1, c0, 0
	pop {pc}
	

mmudisable:
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #1
	mcr p15, 0, r0, c1, c0, 0
	mov pc, lr

@bool
@cas(void *addr, void *old, void *new);
cas:
	ldrex r3, [r0]
	cmp r3, r1
	movne r0, #0
	movne pc, lr
	
	strex r3, r2, [r0]
	cmp r3, #1
	moveq r0, #0
	movne r0, #1
	
	mov pc, lr



@bool
@cas2(void *addr1, void *addr2,
@     void *old1, void *old2,
@     void *new1, void *new2);

cas2:
	mov pc, #0xa0000000


atomicinc:
	push {r1, r2, lr}
1:
	ldrex r2, [r0]
	add r2, r2, #1
	strex r1, r2, [r0]
	cmp r1, #0
	bne 1b
	mov r0, r2
	pop {r1, r2, pc}

	
atomicdec:
	push {r1, r2, lr}
1:	
	ldrex r2, [r0]
	sub r2, r2, #1
	strex r1, r2, [r0]
	cmp r1, #0
	bne 1b
	mov r0, r2
	pop {r1, r2, pc}
