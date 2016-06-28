#include "p_modes.h"

.section .text

.global setlabel
.global gotolabel
.global uregret
.global droptouser
.global fsrstatus
.global faultaddr


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
	mov r1, #0
	mov r2, #0
	mov r3, #0
	mov r4, #0
	mov r5, #0
	mov r6, #0
	mov r7, #0
	mov r8, #0
	mov r9, #0
	mov r10, #0
	mov r11, #0
	mov r12, #0
	cpsie i
	cps #MODE_USR
	mov sp, r0
	mov lr, #0
	mov pc, #0x8000


fsrstatus:
	mrc p15, 0, r0, c5, c0, 0
	bx lr
	
	
faultaddr:
	mrc p15, 0, r0, c6, c0, 0
	bx lr