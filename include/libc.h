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

#ifndef _LIBC_H_
#define _LIBC_H_

#include <types.h>
#include <syscalls.h>

extern int stdin, stdout, stderr;

int
exit(int code);

int
fork(int flags);

int
sleep(int ms);

int
getpid(void);

/* Memory system calls */

enum { MEM_ram, MEM_io };

/* 
 * Maps a number of pages necessary to contain size and 
 * returns the address. Sets size to the real size returned
 * which will be a multiple of page size. If addr is not null
 * then it should be the physical address of the memory wanted.
 * This is so you can request memory mapped IO.
 */
void *
getmem(int type, void *addr, size_t *size);

/*
 * Unmaps the pages starting at addr. 
 * Addr must be page aligned and should be something returned
 * from getmem. Size should be a multiple of a page.
 */
int
rmmem(void *addr, size_t size);

/* Blocks until interrupt intr occurs. */
int
waitintr(int intr);

/* Writes to fds[1] can be read from fds[0] */
int
pipe(int fds[2]);

int
read(int fd, void *buf, size_t len);

int
write(int fd, void *buf, size_t len);

int
seek(int fd, size_t offset, int whence);

int
close(int fd);

int
bind(int out, int in, const char *path);

int
open(const char *path, uint32_t mode, ...);

int
remove(const char *path);

void *
malloc(size_t size);

void
free(void *addr);

void *
memmove(void *dest, const void *src, size_t len);

void *
memset(void *dest, int c, size_t len);

void
printf(const char *, ...);

#define roundptr(x) (x % sizeof(void *) != 0 ? \
		     x + sizeof(void *) - (x % sizeof(void *)) : \
		     x)

/* If address contains 0, store 1 and return 1,
 * else return 0.
 */
bool
testandset(int *addr);

int
atomicinc(int *addr);

int
atomicdec(int *addr);

#endif
