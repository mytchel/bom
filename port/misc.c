#include "dat.h"

void
memmove(void *new, void *old, size_t l)
{
	char *n, *o;
	size_t i;
	
	n = new;
	o = old;
	
	for (i = 0; i < l; i++)
		n[i] = o[i];
}
