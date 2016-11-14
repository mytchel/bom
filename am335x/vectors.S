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


vectortableinit:
	@ Shift vector table to 0x00000000
	@ See page 514 of ARM System Developers Guide
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #(1 << 13) 		@ set V flag to 0 (disable high vectors)
	mcr p15, 0, r0, c1, c0, 0
	
	ldr r0, =vectortable
	mcr p15, 0, r0, c12, c0, 0	@ set vector base address
	
	bx lr


.balign 32
vectortable:
	ldr pc, =_start
	ldr pc, =undefined_instruction
	ldr pc, =swi_ex
	ldr pc, =prefetch_abort
	ldr pc, =data_abort
	b . @ not assigned
	ldr pc, =irq_ex
	ldr pc, =fiq_ex


swi_ex:
	stmfd sp!, {r9, r10, r11, lr}

	mrs r9, spsr

	cpsid if, #MODE_SYS

	mov r10, sp
	mov r11, lr
	
	cpsie if, #MODE_SVC

	ldr lr, =syscalltable
	add r12, lr, r12, lsl #2
	ldr lr, =1f
	ldr pc, [r12]
	
1:
	cpsid if, #MODE_SYS

	mov sp, r10
	mov lr, r11
	
	cpsid if, #MODE_SVC

	msr spsr, r9
	ldmfd sp!, {r9, r10, r11, pc}^


irq_ex:
	stmfd sp, {r0 - r4}
	sub r0, lr, #4
	mov r1, #ABORT_INTERRUPT
	b switch
	

undefined_instruction:
	stmfd sp, {r0 - r4}
	sub r0, lr, #4
	mov r1, #ABORT_INSTRUCTION
	b switch


prefetch_abort:
	stmfd sp, {r0 - r4}
	sub r0, lr, #4
	mov r1, #ABORT_PREFETCH
	b switch


data_abort:
	stmfd sp, {r0 - r4}
	sub r0, lr, #8
	mov r1, #ABORT_DATA
	b switch


switch:
	mrs r2, spsr
	sub r4, sp, #(4 * 5)
	
	clrex

	cpsid if, #MODE_SVC

	sub sp, sp, #(4 * 9)
	stmia sp!, {r5 - r12}
	str r0, [sp]
	sub sp, sp, #(4 * 13)

	@ retrieve and store r0 - r4
	ldmfd r4, {r5 - r9}
	stmia sp, {r5 - r9}
	
	ands r3, r2, #0xf
	beq _user

	@ kernel mode interrupted

	push {r2, lr}

	@ r0 -> interrupted pc, r1 -> type
	bl trap

	pop {r2, lr}
	msr spsr, r2
	ldmia sp!, {r0 - r12, pc}^

_user:  @ user mode interrupted

	mov r3, sp
	cpsid if, #MODE_SYS
	stmfd r3!, {r2, sp, lr}
	cpsid if, #MODE_SVC
	mov sp, r3

	@ r0 -> interrupted pc, r1 -> type
	bl trap

	mov r3, sp
	cpsid if, #MODE_SYS
	ldmfd r3!, {r2, sp, lr}
	cpsid if, #MODE_SVC
	mov sp, r3

	msr spsr, r2
	ldmia sp!, {r0 - r12, pc}^


@ Not used. But just in case kick and scream.
fiq_ex:
	ldr r0, =fiq_msg
	bl puts
	b .


.section .rodata
fiq_msg: .asciz "fiq!\nHanging\n"
