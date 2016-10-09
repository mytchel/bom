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
#define SYSCALL_SLEEP           3
#define SYSCALL_GETPID          4
#define SYSCALL_CHDIR           5

#define SYSCALL_PIPE	        6	

#define SYSCALL_READ	        7	
#define SYSCALL_WRITE	        8	
#define SYSCALL_SEEK	        9	
#define SYSCALL_CLOSE	       10	

#define SYSCALL_STAT	       11	
#define SYSCALL_OPEN           12 
#define SYSCALL_REMOVE	       13
#define SYSCALL_BIND           14
#define SYSCALL_CLEANPATH      15

#define SYSCALL_GETMEM         16
#define SYSCALL_RMMEM          17

#define SYSCALL_WAITINTR       18

#define NSYSCALLS              19

#define OK		0
/* General error */
#define ERR		-1
/* Pipe connection closed somewhere along the line. */
#define ELINK		-2
/* Pipe/File mode wrong for operation. */
#define EMODE		-3
/* No such file. */
#define ENOFILE		-4
/* Function not implimented */
#define ENOIMPL		-5
/* No memory */
#define ENOMEM		-6
/* End of file */
#define EOF		-7
/* For file systems, no child with that name found. */
#define ENOCHILD        -8
/* Directory not empty */
#define ENOTEMPTY       -9

/* Should processor aspects be shared with the child
 * rather than copied. */
#define FORK_smem	(1<<0)
#define FORK_sfgroup	(1<<1)
#define FORK_sngroup	(1<<2)

#define O_RDONLY	(1<<0)
#define O_WRONLY	(1<<1)
#define O_RDWR		(O_RDONLY|O_WRONLY)
#define O_CREATE	(1<<2)
#define O_DIR           (1<<3) /* File must be dir */

#define SEEK_SET        1
#define SEEK_CUR        2
#define SEEK_END        3

#endif
