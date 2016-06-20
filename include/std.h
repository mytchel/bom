#ifndef __STD
#define __STD

void
exit(int);

int
fork(void);

int
open(const char *path, int mode);

void
close(int fd);

int
read(int fd, char *buf, size_t n);

int
write(int fd, char *buf, size_t n);

#endif
