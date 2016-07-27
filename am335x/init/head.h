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

void *
memmove(void *, const void *, size_t);

void *
memset(void *b, int c, size_t len);

bool
strcmp(const uint8_t *s1, const uint8_t *s2);

size_t
strlen(const uint8_t *s);
