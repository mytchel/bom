#ifndef __STD_H
#define __STD_H

#include "types.h"
#include "syscalls.h"

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
