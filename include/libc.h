/*
 *   Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef __STD_H
#define __STD_H

#include <types.h>
#include <syscalls.h>

extern int stdin, stdout, stderr;

/* Process system calls */

int
exit(int code);

int
fork(int flags);

int
sleep(int ms);

int
getpid(void);

/* Memory system calls */

enum { MEM_heap, MEM_io };

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

/* File operation system calls. */

int
pipe(int fds[2]);

int
read(int fd, void *buf, size_t len);

int
write(int fd, void *buf, size_t len);

int
close(int fd);

/* File system system calls. */

int
bind(int out, int in, const char *path);

int
open(const char *path, uint32_t mode, ...);

int
remove(const char *path);


/* Helper functions */

void *
malloc(size_t size);

void
free(void *addr);

void
memmove(void *dest, const void *src, size_t len);

void
memset(void *dest, int c, size_t len);

bool
strcmp(const uint8_t *s1, const uint8_t *s2);

size_t
strlen(const uint8_t *s);

void
printf(const char *, ...);

#endif
