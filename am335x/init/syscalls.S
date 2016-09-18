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

_start:
	bl main
	b exit

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
	push {r0, r1, r2}
	mov r0, #SYSCALL_GETMEM
	svc 0
	add sp, sp, #(4 * 3)
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
	push {r0, r1, r2}
	mov r0, #SYSCALL_OPEN
	svc 0
	add sp, sp, #(4 * 3)
	pop {pc}


remove:
	push {lr}
	push {r0}
	mov r0, #SYSCALL_REMOVE
	svc 0
	add sp, sp, #4
	pop {pc}

waitintr:
	push {lr}
	push {r0}
	mov r0, #SYSCALL_WAITINTR
	svc 0
	add sp, sp, #4
	pop {pc}
	
	
