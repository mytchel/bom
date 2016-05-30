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
	b . @ fiq is not used i think :) ????


@ Return from excecption offset is given on page
@ 157 of Cortex A Series Programmer Guide
swi_ex:
	@ save r0 and r1 registers so we can use them
	@ but dont update pointer 
	stmdb sp, {r0, r1}^
	@ pop r0 which is a pointer to the current processes
	@ pcb structure
	pop {r0}
	
	@ save spsr, r2 - r12, sp and lr to pcb
	mrs r1, spsr
	str r1, [r0]
	add r0, r0, #(4 * 3)
	stmia r0, {r2 - r12, sp, lr}^
	
	@ retrive r0 and r1
	sub sp, sp, #(4 * 3)
	pop {r2, r4}
	add sp, sp, #4
	@ save r0 and r1 to pcb
	sub r0, r0, #(4 * 2)
	stmia r0, {r2, r4}
	
	@ reload kernel registers and jump
	pop {r4 - r12, lr}
	msr cpsr, r12
	mov pc, lr


.global activate
activate:
	@ save kernel registers
	mrs r12, cpsr
	push {r4 - r12, lr}
	push {r0}

	@ change to user mode and set stack pointer
	ldr r1, [r0]
	msr cpsr, r1
	add r0, r0, #4
	ldmia r0, {r0 - r12, sp, lr}
	
	mov pc, lr
	

prefetch_abort:
	mov r1, #0
	bl abort_handler
	b . @ freeze 


data_abort:
	mov r1, #1
	bl abort_handler
	b . @ freeze 


irq_ex:
	push {lr}
	ldr r0, =irq_msg
	bl uart_puts
	pop {lr}
	subs pc, lr, #4	


.section .rodata
irq_msg: .asciz "irq intertupt\n"

sysmsg: .asciz "swi\n"
actmsg: .asciz "activateing\n"
