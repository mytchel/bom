_start:
	mov r4, r0

	mov r1, #1
	mov r2, #2
	mov r3, #3
	mov r4, #4

	cmp r0, #1
	beq msg
	svc #0
	
msg:	
	mov r1, #1
	ldr r2, =s
	mov r3, #14
	push {r1, r2, r3}
	mov r0, #10
	svc #0

	push {r4}
	mov r0, #1
	svc #0
	
	
s:	.ascii "Hello, World.\n"
