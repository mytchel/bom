.section .text
.global syscall
syscall:
	push {r4 - r12, lr}
	stmfd sp, {sp}
	svc 0
	ldmfd sp, {sp}
	pop {r4 - r12, lr}
	mov pc, lr

