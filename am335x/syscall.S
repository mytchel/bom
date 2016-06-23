#include "../port/syscall.h"

.section .text
.global exit
exit:
	mov r8, #SYSCALL_EXIT
	svc 0


.global fork
fork:
	push {r8 - r12, lr}
	mov r8, #SYSCALL_FORK
	svc 0
	pop {r8 - r12, pc}
	

.global getpid
getpid:
	push {r8 - r12, lr}
	mov r8, #SYSCALL_GETPID
	svc 0
	pop {r8 - r12, pc}
		

.global open
open:
	push {r8 - r12, lr}
	mov r8, #SYSCALL_OPEN
	svc 0
	pop {r8 - r12, pc}


.global close
close:
	push {r8 - r12, lr}
	mov r8, #SYSCALL_OPEN
	svc 0
	pop {r8 - r12, pc}


.global read
read:
	push {r8 - r12, lr}
	mov r8, #SYSCALL_READ
	svc 0
	pop {r8 - r12, pc}


.global write
write:
	push {r8 - r12, lr}
	mov r8, #SYSCALL_WRITE
	svc 0 
	pop {r8 - r12, lr}

