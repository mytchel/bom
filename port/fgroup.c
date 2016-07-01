#include "dat.h"

struct fgroup *
newfgroup(void)
{
	struct fgroup *f;
	
	f = kmalloc(sizeof(struct fgroup));
	if (f == nil) {
		return nil;
	}

	f->files = kmalloc(sizeof(struct pipe*));
	if (f->files == nil) {
		kfree(f);
		return nil;
	}
	
	f->files[0] = nil;
	f->nfiles = 1;
	f->refs = 1;	
	
	return f;
}

void
freefgroup(struct fgroup *f)
{
	f->refs--;
	if (f->refs > 0)
		return;
	
	kfree(f->files);
	kfree(f);
}
