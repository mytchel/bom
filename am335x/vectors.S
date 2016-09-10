#
#    Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
	b uregret
	

irq_ex:
	push {r0 - r4}
	mov r0, #ABORT_INTERRUPT
	b switch
	

undefined_instruction:
	push {r0 - r4}
	mov r0, #ABORT_INSTRUCTION
	b switch


prefetch_abort:
	push {r0 - r4}
	mov r0, #ABORT_PREFETCH
	b switch


data_abort:
	push {r0 - r4}
	mov r0, #ABORT_DATA
	b switch


switch:
	mrs r1, spsr
	mov r2, sp
	mov r3, lr
	
	cps #MODE_SVC
	sub sp, sp, #(4 * 3)
	stmia sp, {r0, r1, r3}
	sub r0, sp, #(4 * 2)

	ands r3, r1, #0xf
	beq _user

	@ kernel mode interrupted
	
	add sp, sp, #(4 * 3)
	stmia r0, {sp, lr}

	sub r0, r0, #(4 * 8)
	stmia r0, {r5 - r12}
	sub r0, r0, #(4 * 5)
	ldmfd r2, {r1 - r5}
	stmia r0, {r1 - r5}

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
	
	sub r0, r0, #(4 * 8)
	stmia r0, {r5 - r12}
	sub r0, r0, #(4 * 5)
	ldmfd r2, {r1 - r5}
	stmia r0, {r1 - r5}

	mov sp, r0
	bl trap
	b uregret


@ Not used. But just in case kick and scream.
fiq_ex:
	ldr r0, =fiq_msg
	bl puts
	b .


.section .rodata
fiq_msg: .asciz "fiq!\nHanging\n"
