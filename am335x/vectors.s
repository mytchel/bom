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
	ldr pc, =undefined_instruction
	ldr pc, =swi_ex
	ldr pc, =prefetch_abort
	ldr pc, =data_abort
	b . @ not assigned
	ldr pc, =irq_ex
	b . @ fiq is not used i think ????


@ Return from excecption offset is given on page
@ 157 of Cortex A Series Programmer Guide
swi_ex:
	stmdb sp, {r0 - r12, sp, lr}^
	sub sp, sp, #(4 * 15)

	bl save_task
	
	ldr r8, =syscall_table
	ldr lr, =1f 
	ldr pc, [r8, r9, lsl #2]

1:
	pop {r4 - r12, lr}
	msr cpsr, r12
	movs pc, lr
	

undefined_instruction:
	ldr r0, =inst_msg
	bl puts
	b .


prefetch_abort:
	mov r1, #0
	bl abort_handler
	b . @ freeze 


data_abort:
	mov r1, #1
	bl abort_handler
	b . @ freeze 


irq_ex:
	ldr r0, =irq_msg
	bl puts
	b .
@	subs pc, lr, #4	


.section .rodata
irq_msg: .asciz "irq intertupt\n"
inst_msg: .asciz "undefined instruction. hanging...\n"
swi_msg: .asciz "syscall...\n"
