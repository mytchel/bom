#include "../port/syscall.h"

.section .text

.global sys_return
.global signal_return
.global exit
.global fork
.global getpid
.global mount
.global open
.global close
.global stat
.global read
.global write


sys_return:
	pop {r5, r6} @ Not needed
	pop {r4 - r12, lr}
	movs pc, lr

signal_return:
	mov r4, #SYSCALL_RETURN
	svc 0
	

exit:
	mov r4, #SYSCALL_EXIT
	svc 0


fork:
	push {r4 - r12, lr}
	mov r4, #SYSCALL_FORK
	svc 0
	pop {r4 - r12, pc}
	

getpid:
	push {r4 - r12, lr}
	mov r4, #SYSCALL_GETPID
	svc 0
	pop {r4 - r12, pc}
		

mount:
	mov r4, #SYSCALL_MOUNT
	svc 0


open:
	push {r4 - r12, lr}
	mov r4, #SYSCALL_OPEN
	svc 0
	pop {r4 - r12, pc}


close:
	push {r4 - r12, lr}
	mov r4, #SYSCALL_OPEN
	svc 0
	pop {r4 - r12, pc}


stat:
	push {r4 - r12, lr}
	mov r4, #SYSCALL_STAT
	svc 0
	pop {r4 - r12, pc}


read:
	push {r4 - r12, lr}
	mov r4, #SYSCALL_READ
	svc 0
	pop {r4 - r12, pc}


write:
	push {r4 - r12, lr}
	mov r4, #SYSCALL_WRITE
	svc 0 
	pop {r4 - r12, lr}

