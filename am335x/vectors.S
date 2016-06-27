#include "p_modes.h"
#include "abort.h"

.section .text

.global vectortableinit
.global setlabel
.global gotolabel
.global forkret

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
	sub sp, sp, #(4 * 17)
	stmia sp!, {r0 - r12}
	
	mov r0, sp
	cps #MODE_SYS
	stmia r0!, {sp, lr}
	cps #MODE_SVC
	
	mrs r1, spsr
	stmia r0!, {r1, lr}
	
	sub sp, r0, #(4 * 17)
	mov r0, sp
	
	bl syscall
	
	add sp, sp, #(4 * 15)
	ldmia sp!, {r0, lr}
	msr spsr, r0
	mov r0, sp
	cps #MODE_SYS
	ldmia r0, {r0 - r12, sp, lr}
	cps #MODE_SVC
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


setlabel:
	stmia r0, {sp, lr}
	mov r0, #0
	bx lr
	

gotolabel:
	ldmia r0, {sp, lr}
	mov r0, #1
	bx lr


forkret:
	mov r0, sp
	add sp, sp, #(4 * 15)
	pop {r0, lr}
	msr spsr, r0
	
	cps #MODE_SYS
	ldmia r0, {r0 - r12, sp, lr}
	cps #MODE_SVC
	mov pc, lr


.section .rodata
irq_msg: .asciz "irq\n"
dab_msg: .asciz "data abort\n"
pab_msg: .asciz "pr abort\n"
uab_msg: .asciz "undefined abort\n"
