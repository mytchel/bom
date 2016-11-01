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

.global getcpsr
.global _start
.global setlabel
.global gotolabel
.global droptouser
.global fsrstatus
.global faultaddr
.global cachedisable
.global cacheenable
.global cacheflush
.global mmuloadttb
.global mmuenable
.global mmudisable
.global mmuinvalidate
.global setintr
.global testandset
.global atomicinc 
.global atomicdec


_start:
	ldr r0, = _kernel_bss_start
	ldr r1, = _kernel_bss_end
	mov r2, #0
	mov r3, #0
1:
	stmia r0!, {r2, r3}
	cmp r0, r1
	blt 1b

	bl cachedisable
	bl mmudisable

	mrc p15, 0, r0, c1, c0, 0
	orr r0, r0, #(1<<1) @ enable branch prediction
	mcr p15, 0, r0, c1, c0, 0
	
	cps #MODE_IRQ
	ldr sp, =_ex_stack_top
	cps #MODE_ABORT
	ldr sp, =_ex_stack_top
	cps #MODE_UND
	ldr sp, =_ex_stack_top
	cps #MODE_SVC
	ldr sp, =_ex_stack_top
	
	bl vectortableinit

	b main


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
	add r0, r0, #(4 * 4)
	stmia r0, {r1 - r12, lr}
	sub r0, r0, #(4 * 4)
	stmdb sp, {r1}
	mrs r1, cpsr
	stmia r0!, {r1, sp, lr}
	mov r1, #1
	str r1, [r0]
	mov r0, #0
	ldmdb sp, {r1}
	mov pc, lr
	

gotolabel:
	ldmia r0!, {r1, sp, lr}
	msr spsr, r1
	ldmia r0, {r0 - r12, pc}^


droptouser:
	mov r2, #(MODE_SYS|MODE_DI|MODE_DF)
	msr cpsr, r2

	ldmia r0!, {r1, sp, lr}

	mov r2, #(MODE_SVC|MODE_DI|MODE_DF)
	msr cpsr, r2

	msr spsr, r1
	ldmia r0, {r0 - r12, pc}^


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


@ this is horrible but i cant get anything
@ nicer to work
testandset:
	push {r1, r2, lr}
2:	ldrex r2, [r0]
	cmp r2, #0
	beq 1f
	strex r1, r2, [r0]
	cmp r1, #0
	bne 2b
	mov r0, #0
	pop {r1, r2, pc}

1:	mov r2, #1
	strex r1, r2, [r0]
	cmp r1, #0
	bne 2b
	mov r0, #1
	pop {r1, r2, pc}
	

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
