#include "p_modes.h"
#include "abort.h"

.section .text

.global vector_table_init
.global resume_proc
.global reschedule
.global handle_syscall


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


resume_proc:
	ldr r1, =current
	str r0, [r1]

	mrs r1, cpsr
	@ psr, svc sp, user sp, user lr, pc
	ldmia r0!, {r2, r3, r4, r5, lr}
	
	cps #MODE_SVC
	mov sp, r3
	cps #MODE_SYS
	mov sp, r4
	mov lr, r5
	msr cpsr, r1

	msr spsr, r2
	ldmia r0, {r0 - r12}
	movs pc, lr


.macro save_proc
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
.endm


reschedule:
	push {r0}
	mrs r0, cpsr
	msr spsr, r0
	pop {r0}
	save_proc
	bl schedule


@ r0 - r4 are arguments, r8 the syscall number.
@ r9 - r12 are changeable.
handle_syscall:
	push {lr}
	ldr lr, =1f
	ldr r9, =syscall_table
	ldr pc, [r9, r5, lsl #2]
1:
	ldr lr, =handle_syscall
	pop {pc}


@ Return from exceptions offset is given on page
@ 157 of Cortex A Series Programmer Guide

@ But r0 - r5 are args/return values.
@ Anything else is fair game.
swi_ex:
	pop {r10}
	
	mrs r9, spsr
	push {r9, lr}
	
	ldr lr, =1f
	mov pc, r10
	
1:
	pop {r9, r10}
	msr spsr, r9
	push {lr}
	movs pc, r10


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


.section .rodata
fiq_msg: .asciz "FIQ!\n"
swi_msg: .asciz "swi exit\n"
