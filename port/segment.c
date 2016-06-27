#include "dat.h"

struct segment *
newseg(int type, void *base, size_t size)
{
	struct segment *s;
	
	s = kmalloc(sizeof(struct segment));
	if (s == nil) {
		return nil;
	}
	
	s->type = type;
	s->base = base;
	s->top = base + size;
	
	return s;
}
