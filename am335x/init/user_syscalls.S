#include "../include/syscall.h"

.section .text

.global _start
.global exit
.global fork
.global sleep
.global getpid
.global mount
.global open
.global close
.global stat
.global read
.global write

_start:
	ldr lr, =exit
	bl main


exit:
	push {r0}
	mov r0, #SYSCALL_EXIT
	svc 0


fork:
	push {lr}
	mov r0, #SYSCALL_FORK
	svc 0
	pop {pc}


sleep:
	push {r1, lr}
	push {r0}
	mov r0, #SYSCALL_SLEEP
	svc 0
	pop {r1}
	pop {r1, pc}
	
	
getpid:
	push {lr}
	mov r0, #SYSCALL_GETPID
	svc 0
	pop {pc}
		

mount:
	push {lr}
	mov r0, #SYSCALL_MOUNT
	svc 0
	pop {lr}


open:
	push {lr}
	mov r0, #SYSCALL_OPEN
	svc 0
	pop {pc}


close:
	push {lr}
	mov r0, #SYSCALL_OPEN
	svc 0
	pop {pc}


stat:
	push {lr}
	mov r0, #SYSCALL_STAT
	svc 0
	pop {pc}


read:
	push {lr}
	mov r0, #SYSCALL_READ
	svc 0
	pop {pc}


write:
	push {lr}
	mov r0, #SYSCALL_WRITE
	svc 0 
	pop {pc}

