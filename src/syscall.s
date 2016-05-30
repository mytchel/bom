.arm
.global syscall
syscall:
	push {r4 - r12, lr}
	mov r12, sp
	svc 0
	