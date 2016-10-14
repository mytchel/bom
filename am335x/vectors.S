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

.global vectortableinit

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
	sub sp, sp, #(4 * 14)
	stmia sp, {r0 - r12, lr}

	mrs r1, spsr
	sub r0, sp, #(4 * 3)
	cps #MODE_SYS
	stmia r0, {r1, sp, lr}
	cps #MODE_SVC
	
	mov sp, r0
	bl syscall
	add sp, sp, #(4 * 17)
	b droptouser
	

irq_ex:
	stmfd sp, {r0 - r4}
	mov r1, #ABORT_INTERRUPT
	b switch
	

undefined_instruction:
	stmfd sp, {r0 - r4}
	mov r1, #ABORT_INSTRUCTION
	b switch


prefetch_abort:
	stmfd sp, {r0 - r4}
	mov r1, #ABORT_PREFETCH
	b switch


data_abort:
	stmfd sp, {r0 - r4}
	mov r1, #ABORT_DATA
	b switch


switch:
	mrs r2, spsr
	mov r3, lr
	sub r4, sp, #(4 * 5)
	
	clrex
	
	cps #MODE_SVC

	sub r0, sp, #(4 * 9)
	stmia r0!, {r5 - r12}
	str r3, [r0]
	sub r0, r0, #(4 * 13)

	@ retrieve and store r0 - r4
	ldmfd r4, {r5 - r9}
	stmia r0, {r5 - r9}
	sub r0, r0, #(4 * 3)

	ands r3, r2, #0xf
	cmp r3, #0
	beq _user

	@ kernel mode interrupted

	stmia r0, {r2, sp, lr}

	mov sp, r0

	@ r0 -> interrupted label, r1 -> type
	bl trap

	ldmia r0!, {r1, sp, lr}
	msr spsr, r1
	ldmia r0, {r0 - r12, pc}^

_user:  @ user mode interrupted

	cps #MODE_SYS
	stmia r0, {r2, sp, lr}
	cps #MODE_SVC

	mov sp, r0
	@ r0 -> interrupted label, r1 -> type
	bl trap
	add sp, sp, #(4 * 17)
	b droptouser


@ Not used. But just in case kick and scream.
fiq_ex:
	ldr r0, =fiq_msg
	bl puts
	b .


.section .rodata
fiq_msg: .asciz "fiq!\nHanging\n"
