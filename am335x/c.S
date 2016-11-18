#
#  Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
#  
#  Permission is hereby granted, free of charge, to any person
#  obtaining a copy of this software and associated documentation
#  files (the "Software"), to deal in the Software without
#  restriction, including without limitation the rights to use,
#  copy, modify, merge, publish, distribute, sublicense, and/or
#  sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following
#  conditions:
#  
#  The above copyright notice and this permission notice shall be
#  included in all copies or substantial portions of the Software.
#  
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
#  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#  OTHER DEALINGS IN THE SOFTWARE
#

#include <syscalls.h>

.section .text

.global _start

@ System calls 
.global exit
.global fork
.global exec
.global sleep
.global getpid
.global wait
.global mmap 
.global munmap
.global pipe
.global read
.global write
.global seek
.global stat 
.global chdir
.global close
.global mount
.global bind
.global unbind
.global open
.global remove
.global dup
.global dup2
.global cleanpath
.global waitintr

@ Useful assembly functions
.global testandset
.global atomicinc
.global atomicdec

.word 0x0

_start:
	bl main
	@ fall through to exit

exit:
	push {r12, lr}
	mov r12, #SYSCALL_EXIT
	svc 0
	pop {r12, pc}


fork:
	push {r12, lr}
	mov r12, #SYSCALL_FORK
	svc 0
	pop {r12, pc}

	
exec:
	push {r12, lr}
	mov r12, #SYSCALL_EXEC
	svc 0
	pop {r12, pc}


sleep:
	push {r12, lr}
	mov r12, #SYSCALL_SLEEP
	svc 0
	pop {r12, pc}
	
	
getpid:
	push {r12, lr}
	mov r12, #SYSCALL_GETPID
	svc 0
	mov pc, lr
	pop {r12, pc}

wait:
	push {r12, lr}
	mov r12, #SYSCALL_WAIT
	svc 0
	pop {r12, pc}


mmap:
	push {r12, lr}
	mov r12, #SYSCALL_MMAP
	svc 0
	pop {r12, pc}


munmap:
	push {r12, lr}
	mov r12, #SYSCALL_MUNMAP
	svc 0
	pop {r12, pc}
			
chdir:
	push {r12, lr}
	mov r12, #SYSCALL_CHDIR
	svc 0
	pop {r12, pc}


pipe:
	push {r12, lr}
	mov r12, #SYSCALL_PIPE
	svc 0
	pop {r12, pc}


read:
	push {r12, lr}
	mov r12, #SYSCALL_READ
	svc 0
	pop {r12, pc}


write:
	push {r12, lr}
	mov r12, #SYSCALL_WRITE
	svc 0 
	pop {r12, pc}


seek:
	push {r12, lr}
	mov r12, #SYSCALL_SEEK
	svc 0 
	pop {r12, pc}

	
stat:
	push {r12, lr}
	mov r12, #SYSCALL_STAT
	svc 0
	pop {r12, pc}


close:
	push {r12, lr}
	mov r12, #SYSCALL_CLOSE
	svc 0
	pop {r12, pc}


mount:
	push {r12, lr}
	mov r12, #SYSCALL_MOUNT
	svc 0
	pop {r12, pc}


bind:
	push {r12, lr}
	mov r12, #SYSCALL_BIND
	svc 0
	pop {r12, pc}


unbind:
	push {r12, lr}
	mov r12, #SYSCALL_UNBIND
	svc 0
	pop {r12, pc}


open:
	push {r12, lr}
	mov r12, #SYSCALL_OPEN
	svc 0
	pop {r12, pc}


remove:
	push {r12, lr}
	mov r12, #SYSCALL_REMOVE
	svc 0
	pop {r12, pc}


dup:
	push {r12, lr}
	mov r12, #SYSCALL_DUP
	svc 0
	pop {r12, pc}

	
dup2:
	push {r12, lr}
	mov r12, #SYSCALL_DUP2
	svc 0
	pop {r12, pc}


cleanpath:
	push {r12, lr}
	mov r12, #SYSCALL_CLEANPATH
	svc 0
	pop {r12, pc}

	
waitintr:
	push {r12, lr}
	mov r12, #SYSCALL_WAITINTR
	svc 0
	pop {r12, pc}


@ this is horrible but i cant get anything
@ nicer to work
testandset:
	push {r1, r2, lr}
2:	ldrex r2, [r0]
	cmp r2, #0
	beq 1f
	strex r1, r2, [r0]
	cmp r1, #0
	bne 2b
	mov r0, #0
	pop {r1, r2, pc}

1:	mov r2, #1
	strex r1, r2, [r0]
	cmp r1, #0
	bne 2b
	mov r0, #1
	pop {r1, r2, pc}
	

atomicinc:
	push {r1, r2, lr}
1:
	ldrex r2, [r0]
	add r2, r2, #1
	strex r1, r2, [r0]
	cmp r1, #0
	bne 1b
	mov r0, r2
	pop {r1, r2, pc}

	
atomicdec:
	push {r1, r2, lr}
1:	
	ldrex r2, [r0]
	sub r2, r2, #1
	strex r1, r2, [r0]
	cmp r1, #0
	bne 1b
	mov r0, r2
	pop {r1, r2, pc}
