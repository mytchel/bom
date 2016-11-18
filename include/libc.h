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
#include <err.h>

int
exit(int code) __attribute__((noreturn));

/* Should processor aspects be shared with the child
 * rather than copied. */
#define FORK_smem	(1<<0)
#define FORK_sfgroup	(1<<1)
#define FORK_sngroup	(1<<2)

int
fork(int flags);

int
exec(const char *path, int argc, char *argv[]);

int
sleep(int ms);

int
getpid(void);

int
wait(int *status);

/* Blocks until interrupt intr occurs. */
int
waitintr(int intr);

int
chdir(const char *dir);

/* Writes to fds[1] can be read from fds[0] */
int
pipe(int fds[2]);

#define EOF		0

int
read(int fd, void *buf, size_t len);

int
write(int fd, void *buf, size_t len);

#define SEEK_SET        1
#define SEEK_CUR        2

int
seek(int fd, size_t offset, int whence);

int
close(int fd);

int
mount(int out, int in, const char *path);

int
bind(const char *old, const char *new);

int
unbind(const char *path);


#define O_RDONLY	(1<<0)
#define O_WRONLY	(1<<1)
#define O_RDWR		(O_RDONLY|O_WRONLY)
#define O_CREATE	(1<<2)
#define O_TRUNC 	(1<<3)
#define O_APPEND 	(1<<4)
#define O_DIR           (1<<5) /* File must be dir */
#define O_FILE          (1<<6) /* File must be not be dir */

int
open(const char *path, uint32_t mode, ...);

int
dup(int old);

int
dup2(int old, int new);

int
remove(const char *path);

#define STDIN    0
#define STDOUT   1
#define STDERR   2

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
