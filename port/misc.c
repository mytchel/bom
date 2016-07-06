#include "dat.h"

void
memmove(void *new, const void *old, size_t l)
{
	size_t i;
	char *n;
	const char *o;
	
	n = new;
	o = old;
	
	for (i = 0; i < l; i++)
		n[i] = o[i];
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
