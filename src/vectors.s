.arm
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
	@ store user stack on svc stack
	stm sp, {sp}^
	@ get user stack address and store in r0
	ldm sp, {r0}

	@ push registers to user stack
	stmfd r0!, {r4 - r12, lr}
	
	@ reload kernel registers and jump
	pop {r4 - r12, lr}

	bx lr

.global activate
activate:
	@ save kernel registers
	push {r4 - r12, lr}
	
	@ change to user mode and set stack pointer
	cps #16
	mov sp, r0
	
	pop {r4 - r12, lr}
	bx lr
	
prefetch_abort:
	mov r1, #0
	bl abort_handler

	b . @ freeze 

data_abort:
	mov r1, #1
	bl abort_handler
	
	b . @ freeze 

irq_ex:
	mov r8, lr
	ldr r0, =irq_msg
	bl uart_puts
	subs pc, r8, #4	

fiq_ex:
	mov r8, lr
	ldr r0, =fiq_msg
	bl uart_puts
	subs pc, r8, #4	

.section .rodata
irq_msg: .asciz "irq intertupt\n"
fiq_msg: .asciz "fiq intertupt\n"

sysmsg: .asciz "swi\n"
actmsg: .asciz "activateing\n"
