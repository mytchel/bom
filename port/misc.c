#include "dat.h"

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
