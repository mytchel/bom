#include "../include/syscall.h"

.section .text
.global exit
exit:
	mov r1, r0
	mov r0, #SYSCALL_EXIT
	svc 0


.global fork
fork:
	mov r0, #SYSCALL_FORK
	svc 0
	bx lr
	

.global open
open:
	push {r0 - r1}
	pop {r1 - r2}
	mov r0, #SYSCALL_OPEN
	svc 0
	bx lr


.global close
close:
	mov r1, r0
	mov r0, #SYSCALL_OPEN
	svc 0
	bx lr


.global read
read:
	push {r0 - r2}
	pop {r1 - r3}
	mov r0, #SYSCALL_READ
	svc 0
	bx lr


.global write
write:
	push {r0 - r2}
	pop {r1 - r3}
	mov r0, #SYSCALL_WRITE
	svc 0 
	bx lr

