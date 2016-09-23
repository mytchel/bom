#
#    Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
#
#    This program is free software; you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation; either version 2 of the License, or
#    (at your option) any later version.
#
#    This program is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with this program; if not, write to the Free Software
#    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
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
	
