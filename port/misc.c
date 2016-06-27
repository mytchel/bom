#include "dat.h"

void
memmove(void *new, void *old, size_t l)
{
	size_t i;
	char *n, *o;
	
	n = new;
	o = old;
	
	for (i = 0; i < l; i++)
		*n++ = *o++;
}
