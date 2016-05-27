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
	b . @ reset
	b . @ not assigned
	ldr pc, =swi_ex
	ldr pc, =prefetch_abort
	ldr pc, =data_abort
	b . @ not assigned
	ldr pc, =irq_ex
	ldr pc, =fiq_ex

@ Return from excecption offset is given on page
@ 157 of Cortex A Series Programmer Guide

swi_ex:
	ldr r0, =swi_msg
	bl uart_puts
	movs pc, lr

prefetch_abort:
	ldr r0, =prefetch_msg
	bl uart_puts
	b . @ freeze 

data_abort:
	ldr r0, =data_msg
	bl uart_puts	
	b . @ freeze 

irq_ex:
	ldr r0, =irq_msg
	bl uart_puts
	subs pc, lr, #4	

fiq_ex:
	ldr r0, =fiq_msg
	bl uart_puts
	subs pc, lr, #4	

.section .rodata
swi_msg: .asciz "swi intertupt"
prefetch_msg: .asciz "prefetch abort intertupt"
data_msg: .asciz "data abort intertupt"
irq_msg: .asciz "irq intertupt"
fiq_msg: .asciz "fiq intertupt"
