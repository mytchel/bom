#ifndef __STD_H
#define __STD_H

#include "syscalls.h"

int
exit(int);

int
fork(void);

int
sleep(int);

int
getpid(void);

int
open(char *path, int mode);

int
stat(int fd);

int
close(int fd);

int
read(int fd, char *buf, size_t n);

int
write(int fd, char *buf, size_t n);

#endif
