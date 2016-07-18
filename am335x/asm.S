#include "trap.h"

.section .text

.global _start
.global setlabel
.global gotolabel
.global uregret
.global droptouser
.global fsrstatus
.global faultaddr
.global mmuloadttb
.global mmuenable
.global mmudisable
.global mmuinvalidate

_start:
	cpsid if

	ldr r0, = _kernel_bss_start
	ldr r1, = _kernel_bss_end
	mov r2, #0
	mov r3, #0
1:
	stmia r0!, {r2, r3}
	cmp r0, r1
	blt 1b

	@ Disable cache
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #(1<<12)
	bic r0, r0, #(1<<2)
	mcr p15, 0, r0, c1, c0, 0
	
	cps #MODE_IRQ
	ldr sp, =_irq_stack_top
	cps #MODE_ABORT
	ldr sp, =_abort_stack_top
	cps #MODE_UND
	ldr sp, =_undef_stack_top
	cps #MODE_SVC
	ldr sp, =_svc_stack_top
	
	bl vectortableinit
	
	b kmain


setlabel:
	stmia r0, {r4 - r12, sp, lr}
	mov r0, #0
	mov pc, lr
	

gotolabel:
	ldmia r0, {r4 - r12, sp, lr}
	mov r0, #1
	mov pc, lr


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
	cps #MODE_SYS
	mov sp, r0
	mov lr, #0
	cps #MODE_SVC
	mov r1, #MODE_USR
	msr spsr, r1
	
	mov r0, #0
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
	movs pc, #0


fsrstatus:
	mrc p15, 0, r0, c5, c0, 0
	bx lr
	
	
faultaddr:
	mrc p15, 0, r0, c6, c0, 0
	bx lr


mmuloadttb:
	push {lr}
	mcr p15, 0, r0, c2, c0, 0
	@ set domain mode to client
	mov r0, #1
	mcr p15, 0, r0, c3, c0, 0
	pop {pc}
	

mmuinvalidate:
	mcr p15, 0, r1, c8, c7, 0
	bx lr
	

mmuenable:
	push {r1, lr}
	bl mmuinvalidate
	mrc p15, 0, r1, c1, c0, 0
	orr r1, r1, #1
	mcr p15, 0, r1, c1, c0, 0
	pop {r1, pc}
	

mmudisable:
	push {r1, lr}
	mrc p15, 0, r1, c1, c0, 0
	bic r1, r1, #1
	mcr p15, 0, r1, c1, c0, 0
	pop {r1, pc}
	