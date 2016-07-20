#ifndef __STD_H
#define __STD_H

#include "syscalls.h"

int
exit(int);

int
fork(int);

int
sleep(int);

int
getpid(void);



int
pipe(int *);

size_t
read(int, void *, size_t);

size_t
write(int, void *, size_t);

int
close(int);


int
bind(int, int, const char *);

int
open(const char *, int);

#endif
