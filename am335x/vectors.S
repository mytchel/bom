#include "p_modes.h"
#include "abort.h"

.section .text

.global vectortableinit
.global setlabel
.global gotolabel
.global forkret
.global droptouser

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
	sub sp, sp, #(4 * 17)
	stmia sp, {r0 - r12}
	
	add r0, sp, #(4 * 13)
	cps #MODE_SYS
	stmia r0!, {sp, lr}
	cps #MODE_SVC
	
	mrs r1, spsr
	stmia r0, {r1, lr}
	
	mov r0, sp
	
	bl syscall

	@ Goes on to do forkret	
forkret:
	mov r0, sp
	add sp, sp, #(4 * 15)
	ldmia sp!, {r2, lr}
	msr spsr, r2
	movs pc, lr
	cps #MODE_SYS
	ldmia r0, {r0 - r12, sp, lr}
	cps #MODE_SVC
	movs pc, lr


irq_ex:
	push {r0 - r12, lr}
	ldr r0, =irq_msg
	bl puts
	pop {r0 - r12, lr}
	subs pc, lr, #4
	

undefined_instruction:
	push {r0 - r12, lr}
	ldr r0, =uab_msg
	bl puts
	b .	


prefetch_abort:
	push {r0 - r12, lr}
	ldr r0, =pab_msg
	bl puts
	pop {r0 - r12, lr}
	b .


data_abort:
	push {r0 - r12, lr}
	ldr r0, =dab_msg
	bl puts
	pop {r0 - r12, lr}
	b . 	


@ Not used. Just return if this occurs.
@ Probably a bad idea to be hones.
fiq_ex:
	subs, pc, lr, #4


setlabel:
	stmia r0, {sp, lr}
	mov r0, #0
	bx lr
	

gotolabel:
	ldmia r0, {sp, lr}
	mov r0, #1
	bx lr


@ r0 is user stack pointer
droptouser:
	cpsie i
	cps #MODE_USR
	mov sp, r0
	mov pc, #0

.section .rodata
irq_msg: .asciz "irq\n"
dab_msg: .asciz "data abort\n"
pab_msg: .asciz "pr abort\n"
uab_msg: .asciz "undefined abort\n"
