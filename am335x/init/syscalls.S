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
.global waitintr

.word 0x0

_start:
	bl main
	@ fall through to exit

exit:
	push {r0}
	mov r0, #SYSCALL_EXIT
	svc 0
	add sp, sp, #4
	bx lr


fork:
	push {r0}
	mov r0, #SYSCALL_FORK
	svc 0
	add sp, sp, #4
	bx lr


sleep:
	push {r0}
	mov r0, #SYSCALL_SLEEP
	svc 0
	add sp, sp, #4
	bx lr
	
	
getpid:
	mov r0, #SYSCALL_GETPID
	svc 0
	bx lr


getmem:
	push {r0, r1, r2}
	mov r0, #SYSCALL_GETMEM
	svc 0
	add sp, sp, #(4 * 3)
	bx lr


rmmem:
	push {r0, r1}
	mov r0, #SYSCALL_RMMEM
	svc 0
	add sp, sp, #(4 * 2)
	bx lr
			

pipe:
	push {r0}
	mov r0, #SYSCALL_PIPE
	svc 0
	add sp, sp, #4
	bx lr


read:
	push {r0, r1, r2}
	mov r0, #SYSCALL_READ
	svc 0
	add sp, sp, #(4 * 3)
	bx lr


write:
	push {r0, r1, r2}
	mov r0, #SYSCALL_WRITE
	svc 0 
	add sp, sp, #(4 * 3)
	bx lr


close:
	push {r0}
	mov r0, #SYSCALL_CLOSE
	svc 0
	add sp, sp, #4
	bx lr


bind:
	push {r0, r1,  r2}
	mov r0, #SYSCALL_BIND
	svc 0
	add sp, sp, #(4 *3)
	bx lr

open:
	push {r0, r1, r2}
	mov r0, #SYSCALL_OPEN
	svc 0
	add sp, sp, #(4 * 3)
	bx lr


remove:
	push {r0}
	mov r0, #SYSCALL_REMOVE
	svc 0
	add sp, sp, #4
	bx lr

	
waitintr:
	push {r0}
	mov r0, #SYSCALL_WAITINTR
	svc 0
	add sp, sp, #4
	bx lr
	
