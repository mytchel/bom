#include "../include/types.h"
#include "../include/libc.h"
#include "../include/stdarg.h"
#include "../include/fs.h"

char
getc(void);

void
putc(char);

void
puts(const char *);

void
printf(const char *, ...);

int
ppipe0(int fd);

int
ppipe1(int fd);

int
pmount(void);

int
pfile_open(void);
