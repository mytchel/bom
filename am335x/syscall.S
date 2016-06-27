#include "../port/syscall.h"

.section .text

.global exit
.global fork
.global yield
.global getpid
.global mount
.global open
.global close
.global stat
.global read
.global write


exit:
	mov r4, #SYSCALL_EXIT
	svc 0


fork:
	push {r4 - r12, lr}
	mov r4, #SYSCALL_FORK
	svc 0
	pop {r4 - r12, pc}


yield:
	push {r4 - r12, lr}
	mov r4, #SYSCALL_YIELD
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

