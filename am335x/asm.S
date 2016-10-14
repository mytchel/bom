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

.global _start
.global setlabel
.global gotolabel
.global forkfunc_preloader
.global forkfunc_loader
.global userreturn 
.global droptouser
.global fsrstatus
.global faultaddr
.global mmuloadttb
.global mmuenable
.global mmudisable
.global mmuinvalidate
.global testandset
.global atomicinc 
.global atomicdec

_start:
	cpsid if

	ldr r0, = _kernel_bss_start
	ldr r1, = _kernel_bss_end
	mov r2, #0
	mov r3, #0
1:
	stmia r0!, {r2, r3}
	cmp r0, r1
	blt 1b

	@ Disable cache
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #(1<<12)
	bic r0, r0, #(1<<2)
	bic r1, r1, #(1<<0)
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


setlabel:
	mrs r1, cpsr
	stmia r0, {r1, r4 - r12, sp, lr}
	mov r0, #0
	mov pc, lr
	

gotolabel:
	ldmia r0, {r1, r4 - r12, sp, lr}
	msr cpsr, r1
	mov r0, #1
	mov pc, lr


forkfunc_loader:
	ldr lr, =1f	
	pop {r0, pc}
1:
	ldr r0, =current
	cpsid i
	bl procremove
	bl schedule
	

userreturn:
	mov r0, sp
	add sp, sp, #(4 * 15)
	ldmia sp!, {r1, r2, lr}
	msr spsr, r2
	cps #MODE_SYS
	ldmia r0, {r0 - r12, sp, lr}
	cps #MODE_SVC
	movs pc, lr


@ r0 is user stack pointer
droptouser:
	cps #MODE_SYS
	mov sp, r0
	mov lr, #0

	cps #MODE_SVC

	mov r1, #MODE_USR
	msr spsr, r1
	
	mov r0, #0
	mov r1, #0
	mov r2, #0
	mov r3, #0
	mov r4, #0
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0
	mov r9, #0
	mov r10, #0
	mov r11, #0
	mov r12, #0
	movs pc, #4


fsrstatus:
	mrc p15, 0, r0, c5, c0, 0
	bx lr
	
	
faultaddr:
	mrc p15, 0, r0, c6, c0, 0
	bx lr


mmuloadttb:
	push {lr}
	mcr p15, 0, r0, c2, c0, 0
	@ set domain mode to client
	mov r0, #1
	mcr p15, 0, r0, c3, c0, 0
	pop {pc}
	

mmuinvalidate:
	mcr p15, 0, r1, c8, c7, 0
	bx lr
	

mmuenable:
	push {r1, lr}
	bl mmuinvalidate
	mrc p15, 0, r1, c1, c0, 0
	orr r1, r1, #1
	mcr p15, 0, r1, c1, c0, 0
	pop {r1, pc}
	

mmudisable:
	push {r1, lr}
	mrc p15, 0, r1, c1, c0, 0
	bic r1, r1, #1
	mcr p15, 0, r1, c1, c0, 0
	pop {r1, pc}


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
