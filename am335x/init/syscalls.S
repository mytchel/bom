#include "../include/syscalls.h"

.section .text

.global _start
.global exit
.global fork
.global sleep
.global getpid
.global getmem
.global rmmem
.global pipe
.global read
.global write
.global close
.global bind
.global open
.global remove

_start:
	ldr lr, =exit
	bl main


exit:
	push {r0}
	mov r0, #SYSCALL_EXIT
	svc 0


fork:
	push {lr}
	push {r0}
	mov r0, #SYSCALL_FORK
	svc 0
	add sp, sp, #4
	pop {pc}


sleep:
	push {lr}
	push {r0}
	mov r0, #SYSCALL_SLEEP
	svc 0
	add sp, sp, #4
	pop {pc}
	
	
getpid:
	push {lr}
	mov r0, #SYSCALL_GETPID
	svc 0
	pop {pc}


getmem:
	push {lr}
	push {r0}
	mov r0, #SYSCALL_GETMEM
	svc 0
	add sp, sp, #4
	pop {pc}


rmmem:
	push {lr}
	push {r0, r1}
	mov r0, #SYSCALL_RMMEM
	svc 0
	add sp, sp, #(4 * 2)
	pop {pc}
			

pipe:
	push {lr}
	push {r0}
	mov r0, #SYSCALL_PIPE
	svc 0
	add sp, sp, #4
	pop {pc}


read:
	push {lr}
	push {r0, r1, r2}
	mov r0, #SYSCALL_READ
	svc 0
	add sp, sp, #(4 * 3)
	pop {pc}


write:
	push {lr}
	push {r0, r1, r2}
	mov r0, #SYSCALL_WRITE
	svc 0 
	add sp, sp, #(4 * 3)
	pop {pc}


close:
	push {lr}
	push {r0}
	mov r0, #SYSCALL_CLOSE
	svc 0
	add sp, sp, #4
	pop {pc}


bind:
	push {lr}
	push {r0, r1,  r2}
	mov r0, #SYSCALL_BIND
	svc 0
	add sp, sp, #(4 *3)
	pop {pc}

open:
	push {lr}
	push {r0, r1}
	mov r0, #SYSCALL_OPEN
	svc 0
	add sp, sp, #(4 * 2)
	pop {pc}


remove:
	push {lr}
	push {r0}
	mov r0, #SYSCALL_OPEN
	svc 0
	add sp, sp, #4
	pop {pc}

