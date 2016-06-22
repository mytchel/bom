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


.macro restore_proc
	ldr r5, =user_regs
	ldr r5, [r5]

	@ load process psr, sp, pc, lr
	ldmia r5!, {r1, r2, r3, r4}
	
	mov lr, r3

	msr spsr, r1 @ store user cpsr in spsr
	mrs r6, cpsr
	cps #MODE_SYS
	mov sp, r2
	mov lr, r4
	msr cpsr, r6 @ switch back to previous cpsr

	@ load user regs
	ldmia r5, {r0 - r12}
.endm


.global start_proc
start_proc:
	restore_proc
	sub r1, lr, #4 
	cps #MODE_USR
	mov pc, r1
	

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

@ r0 - r7 are arguments, r8 the syscall number.
@ r9 - r12 are changeable.
swi_ex:
	push {lr}
	cmp r8, #0
	bgt 1f
	ldr lr, =3f
	b 2f
1:
	ldr lr, =4f 
2:
	ldr r9, =syscall_table
	ldr pc, [r9, r8, lsl #2]

3:
	@ Syscall was exit, requires different handling
	pop {lr} @ return stack to previous state
	
	@ restore new current processs
	restore_proc
	subs lr, lr, #4
	movs pc, lr
4:
	pop {lr}
	movs pc, lr

irq_ex:
	subs lr, lr, #4
	save_proc
	bl intc_irq_handler
	bl schedule	
	restore_proc
	movs pc, lr
	

fiq_ex:
	push {r0 - r12, lr}
	ldr r0, =fiq_msg
	bl puts
	pop {r0 - r12, lr}
	subs pc, lr, #4

	
undefined_instruction:
	mov r0, #ABORT_INSTRUCTION
	bl abort_handler
	restore_proc
	subs lr, lr, #4
	movs pc, lr


prefetch_abort:
	mov r0, #ABORT_PREFETCH
	bl abort_handler
	restore_proc
	subs pc, lr, #4


data_abort:
	mov r0, #ABORT_DATA
	bl abort_handler
	restore_proc
	add lr, lr, #4
	subs pc, lr, #8


.global enable_interrupts
enable_interrupts:
	cpsie if
	bx lr


.global disable_interrupts
disable_interrupts:
	cpsid if
	bx lr


.section .rodata
inst_msg: .asciz "undefined instruction. hanging...\n"
swi_msg: .asciz "syscall, ignoring...\n"
fiq_msg: .asciz "FIQ!!!!!\n"
