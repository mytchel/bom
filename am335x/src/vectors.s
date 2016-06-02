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


.global activate_first_task
activate_first_task:
	ldr r0, =current_task_ptr
	ldr r0, [r0]
	
	@ change to user mode and set stack pointer
	ldmia r0!, {r1}
	msr cpsr, r1
	ldmia r0, {r0 - r12, sp, lr}
	
	@ start task
	subs pc, lr, #4

@ Return from excecption offset is given on page
@ 157 of Cortex A Series Programmer Guide
swi_ex:
	push {r4 - r12, lr}
	mrs r12, spsr
	push {r12}
	
	ldr r8, =syscall_table
	ldr lr, =1f 
	ldr pc, [r8, r0, lsl #2]

1:
	pop {r12}
	msr cpsr, r12
	pop {r4 - r12, lr}
	movs pc, lr


.macro restore_current_task
	ldr r0, =current_task_ptr
	ldr r0, [r0]
	
	@ change to user mode and set stack pointer
	ldmia r0!, {r1}
	msr cpsr, r1
	ldmia r0, {r0 - r12, sp, lr}
.endm


.macro save_current_task
	push {r0, r1}
	
	ldr r0, =current_task_ptr
	ldr r0, [r0]

	@ store cpsr
	mrs r1, spsr
	stmia r0!, {r1}
	
	add r0, r0, #(4 * 2)
	stmia r0, {r2 - r12, sp}^
	add r0, r0, #(4 * 12)
	stmia r0, {lr}
	sub r0, r0, #(4 * 14)
	
	@ store r0, r1, r2
	pop {r1, r2}
	stmia r0, {r1, r2}
.endm


irq_ex:
	save_current_task
	bl intc_irq_handler
	restore_current_task
	subs pc, lr, #4
	

undefined_instruction:
	ldr r0, =inst_msg
	bl puts
	b . @ freeze


prefetch_abort:
	mov r1, #0
	bl abort_handler
	b . @ freeze 


data_abort:
	mov r1, #1
	bl abort_handler
	b . @ freeze 


.section .rodata
inst_msg: .asciz "undefined instruction. hanging...\n"
