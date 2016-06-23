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
	ldr r1, =current
	str r0, [r1]

	mrs r6, cpsr
	
	cps #MODE_SVC
	ldmia r0!, {r1, sp}
	msr cpsr, r6
	
	cps #MODE_SYS
	ldmia r0!, {sp, lr}
	msr cpsr, r6

	msr spsr, r1
	@ load user regs
	ldmia r0, {r0 - r12, lr}
	movs pc, lr


.global reschedule
reschedule:
	push {r0, r1}
	
	ldr r0, =current
	ldr r0, [r0]
	
	mrs r1, cpsr
	add sp, sp, #(4 * 2)
	stmia r0!, {r1, sp}
	sub sp, sp, #(4 * 2)

	cps #MODE_SYS
	stmia r0!, {sp, lr}
	msr cpsr, r1
	
	add r1, r0, #(4 * 2)
	stmia r1, {r2 - r12, lr}
	pop {r1, r2}
	stmia r0, {r1, r2}
	
	bl schedule


.macro save_proc
	push {r0 - r2}

	@ get address of current process regs
	ldr r0, =current
	ldr r0, [r0]

	mrs r1, cpsr @ save current cpsr
	mrs r2, spsr @ save user cpsr
	
	stmia r0!, {r2}
	
	add r0, r0, #4 @ skip assigning ksp

	cps #MODE_SYS
	stmia r0!, {sp, lr}
	msr cpsr, r1

	@ store regs and lr
	add r1, r0, #(4 * 3)
	stmia r1, {r3 - r12, lr}
	pop {r1 - r3}
	stmia r0, {r1 - r3}
.endm


@ Return from exceptions offset is given on page
@ 157 of Cortex A Series Programmer Guide

@ r0 - r7 are arguments, r8 the syscall number.
@ r9 - r12 are changeable.
swi_ex:
	mrs r9, spsr
	push {r9, lr}
	
	mov r9, #4
	mul r8, r9
	
	ldr r9, =syscall_table
	add r9, r8
	ldr lr, =1f
	ldr pc, [r9]
1:
	pop {r9, lr}
	msr spsr, r9
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
	cpsid if
	bx lr


.section .rodata
fiq_msg: .asciz "FIQ!\n"
swi_msg: .asciz "swi returning to caller\n"
.global lr_msg
lr_msg: .asciz "lr : 0x%h\n"
.global cpsr_msg
cpsr_msg: .asciz "cpsr : 0b%b\n"
