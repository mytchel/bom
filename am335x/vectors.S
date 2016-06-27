#include "p_modes.h"
#include "abort.h"

.section .text

.global vectortableinit
.global setlabel
.global gotolabel

vectortableinit:
	@ Shift vector table to 0x00000000
	@ See page 514 of ARM System Developers Guide
	mrc p15, 0, r0, c1, c0, 0	@ read CP15s c1 register into r0
	bic r0, r0, #(1 << 13) 		@ set V flag to 0 (disable high vectors)
	mcr p15, 0, r0, c1, c0, 0	@ 
	
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


@ Return from exceptions offset is given on page
@ 157 of Cortex A Series Programmer Guide

@ But r0 - r3 are args/return values.
@ r4 is syscall number.
@ Anything else is fair game.
swi_ex:
	mrs r5, spsr
	push {r5, lr}
	
	ldr lr, =1f
	ldr r5, =syscalltable
	ldr pc, [r5, r4, lsl #2]
1:
	pop {r5, lr}
	msr spsr, r5
	movs pc, lr


irq_ex:
	push {r0 - r12, lr}
	ldr r0, =irq_msg
	bl puts
	pop {r0 - r12, lr}
	subs pc, lr, #4
	
undefined_instruction:
	push {r0 - r12, lr}
	ldr r0, =uab_msg
	bl puts
	b .	

prefetch_abort:
	push {r0 - r12, lr}
	ldr r0, =pab_msg
	bl puts
	pop {r0 - r12, lr}
	b .


data_abort:
	push {r0 - r12, lr}
	ldr r0, =dab_msg
	bl puts
	pop {r0 - r12, lr}
	b . 	


@ FIQ is not used.
fiq_ex:
	b .


switch:
	mrs r1, spsr
	mov r2, lr
	mov r3, sp
	
	cps #MODE_SVC

	stmdb sp, {r0 - r5}

	@ get address of current process regs
	ldr r0, =current
	ldr r0, [r0]

	mrs r1, cpsr @ save current cpsr
	mrs r2, spsr @ save user cpsr
		
	cps #MODE_SVC
	mov r3, sp
	cps #MODE_SYS
	mov r4, sp
	mov r5, lr
	msr cpsr, r1
	
	@ psr, kernel sp, user sp, user lr, pc
	stmia r0!, {r2, r3, r4, r5, lr}
	
	add r1, r0, #(4 * 6)
	stmia r1, {r6 - r12}
	ldmdb sp, {r1 - r6}
	stmia r0, {r1 - r6}


setlabel:
	stmia r0, {sp, lr}
	mov r0, #0
	bx lr
	

gotolabel:
	ldmia r0, {sp, lr}
	mov r0, #1
	bx lr

.section .rodata
irq_msg: .asciz "irq\n"
dab_msg: .asciz "data abort\n"
pab_msg: .asciz "pr abort\n"
uab_msg: .asciz "undefined abort\n"
