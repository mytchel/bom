/*
 *
 * Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _SYSCALLS_H_
#define _SYSCALLS_H_

#define SYSCALL_EXIT            1
#define SYSCALL_FORK            2
#define SYSCALL_EXEC            3 
#define SYSCALL_WAIT            4
#define SYSCALL_SLEEP           5 
#define SYSCALL_GETPID          6 
#define SYSCALL_CHDIR           7 

#define SYSCALL_PIPE	        8

#define SYSCALL_READ	        9
#define SYSCALL_WRITE	       10	
#define SYSCALL_SEEK	       11	
#define SYSCALL_CLOSE	       12	

#define SYSCALL_STAT	       13	
#define SYSCALL_OPEN           14 
#define SYSCALL_REMOVE	       15
#define SYSCALL_DUP	       23
#define SYSCALL_DUP2	       24

#define SYSCALL_MOUNT          16
#define SYSCALL_BIND           17
#define SYSCALL_UNBIND         18

#define SYSCALL_CLEANPATH      19

#define SYSCALL_MMAP           20
#define SYSCALL_MUNMAP         21

#define SYSCALL_WAITINTR       22

#define NSYSCALLS              25

#endif
