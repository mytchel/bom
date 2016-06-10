#ifndef __KMALLOC
#define __KMALLOC

void *
kmalloc(size_t size);

void
kfree(void *ptr);

#endif