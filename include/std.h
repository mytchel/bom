#ifndef __STD_H
#define __STD_H

int
exit(int);

int
fork(void);

int
getpid(void);

int
open(const char *path, int mode);

int
close(int fd);

int
read(int fd, char *buf, size_t n);

int
write(int fd, char *buf, size_t n);

#endif
