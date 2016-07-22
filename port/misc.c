#include "dat.h"

void *
memmove(void *dest, const void *src, size_t len)
{
	uint8_t *d;
	const uint8_t *s;
	
	d = dest;
	s = src;
	
	while (len-- > 0)
		*d++ = *s++;
		
	return dest;
}

void *
memset(void *b, int c, size_t len)
{
	uint8_t *bb = b;
	
	while (len-- > 0)
		*bb++ = c;
		
	return b;
}

bool
strcmp(const uint8_t *s1, const uint8_t *s2)
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
strlen(const uint8_t *s)
{
	size_t len = 0;
	
	while (*s++)
		len++;
	
	return len;
}

void
lock(int *l)
{
	while (*l)
		schedule();
	l++;
}

void
unlock(int *l)
{
	l--;
}
