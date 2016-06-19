#ifndef __STD
#define __STD

void
exit(int);

int
open(const char *path, int mode);

int
read(int fd, char *buf, size_t n);

int
write(int fd, char *buf, size_t n);

int
fork(void);

#endif
