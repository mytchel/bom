#ifndef __STD_H
#define __STD_H

struct fs {
	int (*open)(const char *path, int mode);
	int (*close)(int fd);
	int (*stat)(int fd);
	int (*write)(int fd, char *buf, size_t size);
	int (*read)(int fd, char *buf, size_t size);
};

int
exit(int);

int
fork(void);

int
sleep(int);

int
getpid(void);

int
mount(struct fs *fs, char *path);

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
