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

#include "head.h"

reg_t sysexit(int code);
reg_t sysfork(int flags, struct label *regs);
reg_t sysexec(const char *path, int argc, char *argv[]);
reg_t syssleep(int ms);
reg_t sysgetpid(void);
reg_t syswait(int *status);
reg_t sysmmap(int type, void *addr, size_t *size);
reg_t sysmunmap(void *addr, size_t size);
reg_t syswaitintr(int intr);
reg_t syschdir(const char *path);
reg_t syspipe(int fds[2]);
reg_t sysread(int fd, void *buf, size_t len);
reg_t syswrite(int fd, void *buf, size_t len);
reg_t sysseek(int fd, size_t offset, int whence);
reg_t sysstat(const char *path, struct stat *stat);
reg_t sysclose(int fd);
reg_t sysmount(int out, int in, const char *path);
reg_t sysbind(const char *old, const char *new);
reg_t sysunbind(const char *path);
reg_t sysopen(const char *path, uint32_t mode, ...);
reg_t sysremove(const char *path);
reg_t syscleanpath(char *opath, char *cpath, size_t cpathlen);


void *syscalltable[NSYSCALLS] = {
  [SYSCALL_EXIT]              = (void *) &sysexit,
  [SYSCALL_FORK]              = (void *) &sysfork,
  [SYSCALL_EXEC]              = (void *) &sysexec,
  [SYSCALL_SLEEP]             = (void *) &syssleep,
  [SYSCALL_GETPID]            = (void *) &sysgetpid,
  [SYSCALL_WAIT]              = (void *) &syswait,
  [SYSCALL_CHDIR]             = (void *) &syschdir,
  [SYSCALL_MMAP]              = (void *) &sysmmap,
  [SYSCALL_MUNMAP]            = (void *) &sysmunmap,
  [SYSCALL_PIPE]              = (void *) &syspipe,
  [SYSCALL_READ]              = (void *) &sysread,
  [SYSCALL_WRITE]             = (void *) &syswrite,
  [SYSCALL_SEEK]              = (void *) &sysseek,
  [SYSCALL_CLOSE]             = (void *) &sysclose,
  [SYSCALL_STAT]              = (void *) &sysstat,
  [SYSCALL_MOUNT]             = (void *) &sysmount,
  [SYSCALL_BIND]              = (void *) &sysbind,
  [SYSCALL_UNBIND]            = (void *) &sysunbind,
  [SYSCALL_OPEN]              = (void *) &sysopen,
  [SYSCALL_REMOVE]            = (void *) &sysremove,
  [SYSCALL_CLEANPATH]         = (void *) &syscleanpath,
  [SYSCALL_WAITINTR]          = (void *) &syswaitintr,
};
