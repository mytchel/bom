#include "p_modes.h"
#include "abort.h"

.section .text

.global vector_table_init
vector_table_init:
	@ Shift vector table to 0x00000000
	@ See page 514 of ARM System Developers Guide
	mrc p15, 0, r0, c1, c0, 0	@ read CP15s c1 register into r0
	bic r0, r0, #(1 << 13) 		@ set V flag to 0 (disable high vectors)
	mcr p15, 0, r0, c1, c0, 0	@ 
	
	ldr r0, =vector_table
	mcr p15, 0, r0, c12, c0, 0	@ set vector base address
	
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
	ldr pc, =fiq_ex


.global run_proc
run_proc:
	ldr r1, =user_regs
	str r0, [r1]

	@ load process psr, sp, pc
	ldmia r0!, {r1, r2, r3, r4, r5}
	
	mov sp, r3
	
	mrs r6, cpsr
	cps #MODE_SYS
	mov sp, r5
	mov lr, r5
	msr cpsr, r6

	msr spsr, r2
	msr cpsr, r1
	@ load user regs
	ldmia r0, {r0 - r12, pc}


.macro save_proc
	push {r0 - r12}

	@ get address of current process regs
	ldr r0, =user_regs
	ldr r0, [r0]
	
	mrs r1, cpsr @ save current cpsr
	mrs r2, spsr @ save user cpsr
	
	stmia r0!, {r1, r2, sp}

	@ get user sp and lr
	cps #MODE_SYS
	mov r3, sp
	mov r4, lr
	msr cpsr, r1 @ switch back to current mode

	@ store spsr, kernel sp, user sp, user lr
	stmia r0!, {r3, r4}

	@ store regs and lr
	pop {r5 - r9}
	stmia r0!, {r5 - r9}
	pop {r5 - r9}
	stmia r0, {r5 - r9}
	pop {r5 - r7}
	stmia r0, {r5 - r7, lr}
.endm


@ Return from exceptions offset is given on page
@ 157 of Cortex A Series Programmer Guide

@ r0 - r7 are arguments, r8 the syscall number.
@ r9 - r12 are changeable.
swi_ex:
	push {lr}
	
	ldr lr, =1f 
	ldr r9, =syscall_table
	ldr pc, [r9, r8, lsl #2]
1:
	pop {lr}
	movs pc, lr

irq_ex:
	subs lr, lr, #4
	save_proc
	bl intc_irq_handler
	bl schedule	
	

fiq_ex:
	sub lr, lr, #4
	push {r0 - r12, lr}
	ldr r0, =fiq_msg
	bl puts
	pop {r0 - r12, pc}

	
undefined_instruction:
	mov r0, #ABORT_INSTRUCTION
	bl abort_handler


prefetch_abort:
	mov r0, #ABORT_PREFETCH
	bl abort_handler


data_abort:
	mov r0, #ABORT_DATA
	bl abort_handler


.global enable_interrupts
enable_interrupts:
	cpsie i
	bx lr


.global disable_interrupts
disable_interrupts:
	cpsid i
	bx lr


.section .data
user_regs: .word 0

.section .rodata
fiq_msg: .asciz "FIQ!\n"
