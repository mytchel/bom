#include "../include/syscall.h"

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
	push {r0}
	mov r0, #SYSCALL_EXIT
	svc 0


fork:
	push {lr}
	mov r0, #SYSCALL_FORK
	svc 0
	push {r0 - r12, lr}
	ldr r0, =fork_msg
	bl puts
	pop {r0 - r12, lr}
	pop {pc}


yield:
	push {lr}
	mov r0, #SYSCALL_YIELD
	svc 0
	pop {pc}
	
	
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

.section .rodata
fork_msg: .asciz "fork returned to 'user'\n"
