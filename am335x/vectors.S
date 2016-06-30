#include "trap.h"

.section .text

.global vectortableinit

vectortableinit:
	@ Shift vector table to 0x00000000
	@ See page 514 of ARM System Developers Guide
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #(1 << 13) 		@ set V flag to 0 (disable high vectors)
	mcr p15, 0, r0, c1, c0, 0
	
	ldr r0, =vectortable
	mcr p15, 0, r0, c12, c0, 0	@ set vector base address
	
	bx lr


.balign 32
vectortable:
	ldr pc, =_start
	ldr pc, =undefined_instruction
	ldr pc, =swi_ex
	ldr pc, =prefetch_abort
	ldr pc, =data_abort
	b . @ not assigned
	ldr pc, =irq_ex
	ldr pc, =fiq_ex


swi_ex:
	sub sp, sp, #(4 * 18)
	stmia sp, {r0 - r12}
	
	add r0, sp, #(4 * 13)
	cps #MODE_SYS
	stmia r0!, {sp, lr}
	cps #MODE_SVC
	
	mrs r1, spsr
	@ stored r0 doesnt matter, type is not used in syscalls.
	stmia r0, {r0, r1, lr}
	
	mov r0, sp
	
	bl syscall
	b uregret
	

irq_ex:
	push {r0 - r4}
	mov r0, #ABORT_INTERRUPT
	b switch
	

undefined_instruction:
	push {r0 - r4}
	mov r0, #ABORT_INSTRUCTION
	b switch


prefetch_abort:
	push {r0 - r4}
	mov r0, #ABORT_PREFETCH
	b switch


data_abort:
	push {r0 - r4}
	mov r0, #ABORT_DATA
	b switch


switch:
	mrs r1, spsr
	mov r2, sp
	mov r3, lr
	
	cps #MODE_SVC
	sub sp, sp, #(4 * 3)
	stmia sp, {r0, r1, r3}
	sub r0, sp, #(4 * 2)
	cps #MODE_SYS
	stmia r0, {sp, lr}
	cps #MODE_SVC
	
	sub r0, r0, #(4 * 8)
	stmia r0, {r5 - r12}
	sub r0, r0, #(4 * 5)
	ldmfd r2, {r1 - r5}
	stmia r0, {r1 - r5}
	
	mov sp, r0
	bl trap
	b uregret


@ Not used. But just incase kick and scream.
fiq_ex:
	ldr r0, =fiq_msg
	bl puts
	b .


.section .rodata
fiq_msg: .asciz "fiq!\nHanging\n"
