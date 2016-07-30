#include "../include/types.h"
#include "../include/libc.h"
#include "../include/stdarg.h"
#include "../include/fs.h"

extern int stdin, stdout, stderr;

bool
uartinit(void);

char
getc(void);

void
putc(char);

void
puts(const char *);

void
printf(const char *, ...);

size_t
sprintf(char *, size_t, const char *, ...);

int
ppipe0(int fd);

int
ppipe1(int fd);

int
pmount(void);

int
pfile_open(void);
