#include "p_modes.h"

.section .text

.global vector_table_init
vector_table_init:
	@ Shift vector table to 0x00000000
	@ See page 514 of ARM System Developers Guide
	mrc p15, 0, r0, c1, c0, 0	@ read CP15s c1 register into r0
	bic r0, r0, #(1 << 13) 		@ set V flag to 0 (disable high vectors)
	mcr p15, 0, r0, c1, c0, 0	@ write r0 to CP15s c1
	
	ldr r0, =vector_table
	mcr p15, 0, r0, c12, c0, 0	@ set vector base address, does this work?
	
	bx lr

.balign 32
vector_table:
	ldr pc, =_start
	ldr pc, =undefined_instruction
	ldr pc, =swi_ex
	ldr pc, =prefetch_abort
	ldr pc, =data_abort
	b . @ not assigned
	ldr pc, =irq_ex
	b . @ fiq is not used


@ r0 should be the address to a proc_machine structure.
.macro restore_proc
	ldr r1, =user_regs
	str r0, [r1]

	@ load process psr, sp, pc, lr
	ldmia r0!, {r1, r2, r3, r4}
	
	mov lr, r3

	msr spsr, r1 @ store user cpsr in spsr
	mrs r5, cpsr
	cps #MODE_SYS
	mov sp, r2
	mov lr, r4
	msr cpsr, r5 @ switch back to previous cpsr

	@ load user regs
	ldmia r0, {r0 - r12}
.endm


.global start_proc
start_proc:
	restore_proc
	cps #MODE_USR
	mov pc, lr
	

.macro save_proc
	push {r0 - r12}

	@ get address of current process regs
	ldr r0, =user_regs
	ldr r0, [r0]
	
	mrs r1, cpsr @ save current cpsr
	mrs r2, spsr @ save user cpsr

	@ get user sp and lr
	cps #MODE_SYS
	mov r3, sp
	mov r4, lr
	msr cpsr, r1 @ switch back to current mode

	@ store user cpsr, sp, pc
	stmia r0!, {r2, r3, lr}

	@ store user lr and regs
	pop {r5 - r12}
	stmia r0!, {r4 - r12}
	pop {r5 - r9}
	stmia r0, {r5 - r9}
.endm


@ Return from exceptions offset is given on page
@ 157 of Cortex A Series Programmer Guide

swi_ex:
	add lr, lr, #4
	save_proc

	ldr r0, =swi_msg
	bl puts
			
@	ldr r8, =syscall_table
@	ldr lr, =1f 
@	ldr pc, [r8, r0, lsl #2]

	bl schedule
	restore_proc
	sub lr, lr, #4
	movs pc, lr

irq_ex:
	save_proc
	bl intc_irq_handler
	bl schedule	
	restore_proc
	subs pc, lr, #4
	

undefined_instruction:
	ldr r0, =inst_msg
	bl puts
	b . @ freeze


prefetch_abort:
	save_proc
	mov r1, #0
	bl abort_handler
	bl schedule
	restore_proc
	subs pc, lr, #4


data_abort:
	save_proc
	mov r1, #1
	bl abort_handler
	bl schedule
	restore_proc
	subs pc, lr, #4


.section .data
user_regs:
	.word 0
	

.section .rodata
inst_msg: .asciz "undefined instruction. hanging...\n"
swi_msg: .asciz "syscall, ignoring...\n"
