.arm
.section .text

.global activate_task
activate_task:
	@ save kernel registers
	mrs r12, cpsr
	push {r4 - r12, lr}
	
	ldr r0, =current_task_pcb
	ldr r0, [r0]

	@ change to user mode and set stack pointer
	ldmia r0!, {r1}
	msr cpsr, r1
	ldmia r0, {r0 - r12, sp, lr}^
	
	movs pc, lr

.global save_task
save_task:
	ldr r0, =current_task_pcb
	
	mrs r1, spsr
	stmia r0!, {r1}
	@ get r0 - r11
	pop {r1, r12}
	stmia r0!, {r1 - r12}
	@ get r12, sp, lr
	pop {r1, r2, r3}
	stmia r0!, {r1, r2, r3}
	
	bx lr

