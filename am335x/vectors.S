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
	sub sp, sp, #(4 * 18)
	stmia sp, {r0 - r12}
	
	add r0, sp, #(4 * 13)
	cps #MODE_SYS
	stmia r0!, {sp, lr}
	cps #MODE_SVC
	
	mrs r1, spsr
	@ stored r0 doesnt matter, type is not used in syscalls.
	stmia r0, {r0, r1, lr}
	
	mov r0, sp

	bl syscall
	b droptouser
	

irq_ex:
	stmfd sp, {r0 - r4}
	mov r0, #ABORT_INTERRUPT
	b switch
	

undefined_instruction:
	stmfd sp, {r0 - r4}
	mov r0, #ABORT_INSTRUCTION
	b switch


prefetch_abort:
	stmfd sp, {r0 - r4}
	mov r0, #ABORT_PREFETCH
	b switch


data_abort:
	stmfd sp, {r0 - r4}
	mov r0, #ABORT_DATA
	b switch


switch:
	mrs r1, spsr
	mov r2, lr
	sub r3, sp, #(4 * 5)
	
	clrex
	
	cps #MODE_SVC

	@ store interrupted type, spsr, lr

	sub r4, sp, #(4 * 3)
	stmia r4, {r0, r1, r2}

	@ skip sp, lr, and store r5 - r12
	sub r0, r4, #(4 * 10)
	stmia r0, {r5 - r12}

	@ store r0 - r4
	sub r0, r0, #(4 * 5)
	ldmfd r3, {r2 - r6}
	stmia r0, {r2 - r6}
	add r0, r0, #(4 * 13)

	ands r2, r1, #0xf
	cmp r2, #0
	beq _user

	@ kernel mode interrupted

	@ store ksp, lr
	stmia r0, {sp, lr}

	@ set up arg and sp
	sub r0, r0, #(4 * 13)
	mov sp, r0

	bl trap

	add sp, sp, #(4 * 15)
	ldmia sp, {r1, r2, r3}
	msr spsr, r2
	str r3, [sp]
	sub sp, sp, #(4 * 15)

	ldmia sp, {r0 - r12, sp, lr, pc}^

_user:  @ user mode interrupted

	cps #MODE_SYS
	stmia r0, {sp, lr}
	cps #MODE_SVC

	sub r0, r0, #(4 * 13)
	mov sp, r0
	bl trap
	b droptouser


@ Not used. But just in case kick and scream.
fiq_ex:
	ldr r0, =fiq_msg
	bl puts
	b .


.section .rodata
fiq_msg: .asciz "fiq!\nHanging\n"
