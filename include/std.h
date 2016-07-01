#ifndef __STD_H
#define __STD_H

#include "syscalls.h"

/* Process Control */

int
exit(int);

int
fork(int);

int
sleep(int);

int
getpid(void);

/* File System */

int
pipe(int *);

int
close(int);

size_t
read(int, void *, size_t);

size_t
write(int, void *, size_t);

#endif
