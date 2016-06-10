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


.macro restore_task
.endm

.global activate_proc
activate_proc:
	ldmia r0!, {r1, sp, lr}
	movs pc, lr

	@ change to user mode and set stack pointer
	ldr r0, =machine_current
	ldr r0, [r0]
	add r0, r0, #(4 * 2)
	ldr lr, [r0]
	
@	ldmia r0!, {r1}
@	msr cpsr, r1
@	ldmia r0, {sp, lr}
@	pop {r0 - r12}

	movs pc, lr

.macro save_current_task
	@ save user stack point and r0 to stack
	@ I think.
	push {r0}
	push {sp}^ 
	pop {r0} @ pop user stack pointer
	stmdb r0!, {r1 - r12}
	pop {r1} @ retrieve r0
	stmdb r0!, {r1} @ save r0
	
	ldr r1, =machine_current
	ldr r1, [r1]

	@ store cpsr
	mrs r2, spsr
	stmia r1!, {r2}
	stmia r1!, {r0, lr}
.endm

@ Return from excecption offset is given on page
@ 157 of Cortex A Series Programmer Guide
swi_ex:
	push {r4 - r12, lr}

	ldr r0, =swi_msg
	bl puts
			
@	ldr r8, =syscall_table
@	ldr lr, =1f 
@	ldr pc, [r8, r0, lsl #2]

	pop {r4 - r12, pc}^

irq_ex:
	push {lr}

	save_current_task
	bl intc_irq_handler

	pop {lr}
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
swi_msg: .asciz "syscall, ignoring...\n"
