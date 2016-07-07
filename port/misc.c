#include "dat.h"

void *
memmove(void *dest, const void *src, size_t len)
{
	char *d;
	const char *s;
	
	d = dest;
	s = src;
	
	while (len-- > 0)
		*d++ = *s++;
		
	return dest;
}

void *
memset(void *b, int c, size_t len)
{
	char *bb = b;
	
	while (len-- > 0)
		*bb++ = c;
		
	return b;
}

bool
strcmp(const char *s1, const char *s2)
{
	while (*s1 && *s2)
		if (*s1++ != *s2++)
			return false;
	
	if (*s1 == 0 && *s2 == 0)
		return true;
	else
		return false;
}

size_t
strlen(const char *s)
{
	size_t len = 0;
	
	while (*s++)
		len++;
	
	return len;
}
