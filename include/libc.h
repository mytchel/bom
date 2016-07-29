#ifndef __STD_H
#define __STD_H

#include "syscalls.h"

int
exit(int code);

int
fork(int flags);

int
sleep(int ms);

int
getpid(void);


/* 
 * Maps a number of pages necessary to contain size and 
 * returns the address. Sets size to the real size returned
 * which will be a multiple of page size.
 */
void *
getmem(size_t *size);

/*
 * Unmaps the pages starting at addr. 
 * Addr must be page aligned and should be something returned
 * from getmem. Size should be a multiple of a page.
 */
int
rmmem(void *, size_t);


int
pipe(int *fds);

int
read(int fd, void *buf, size_t len);

int
write(int fd, void *buf, size_t len);

int
close(int fd);


int
bind(int out, int in, const char *path);

int
open(const char *path, uint32_t mode, ...);

int
remove(const char *path);


void *
malloc(size_t);

void
free(void *);

void
memmove(void *dest, const void *src, size_t len);

void
memset(void *b, int c, size_t len);

bool
strcmp(const uint8_t *s1, const uint8_t *s2);

size_t
strlen(const uint8_t *s);

#endif
