#ifndef __STD_H
#define __STD_H

int
exit(int);

int
fork(void);

int
getpid(void);

int
receive(int pid, char *buf, size_t n);

int
send(int pid, char *buf, size_t n);

#endif
