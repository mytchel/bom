#include "p_modes.h"

.section .text

.global setlabel
.global gotolabel
.global uregret
.global droptouser


setlabel:
	stmia r0, {sp, lr}
	mov r0, #0
	bx lr
	

gotolabel:
	ldmia r0, {sp, lr}
	mov r0, #1
	bx lr


@ user register return. Not you regret.
uregret:
	bl mmuenable
	
	mov r0, sp
	add sp, sp, #(4 * 15)
	@ r1 is type, can be ignored
	ldmia sp!, {r1, r2, lr}
	msr spsr, r2
	cps #MODE_SYS
	ldmia r0, {r0 - r12, sp, lr}
	cps #MODE_SVC
	movs pc, lr


@ r0 is user stack pointer
droptouser:
	cpsie i
	cps #MODE_USR
	ldr lr, =exit
	mov sp, r0
	mov pc, #0x00008000

