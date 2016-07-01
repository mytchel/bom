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

struct fgroup *
copyfgroup(struct fgroup *fo)
{
	int i;
	struct fgroup *f;
	
	f = kmalloc(sizeof(struct fgroup));
	if (f == nil) {
		return nil;
	}

	f->files = kmalloc(sizeof(struct pipe*) * fo->nfiles);
	if (f->files == nil) {
		kfree(f);
		return nil;
	}
	
	f->nfiles = fo->nfiles;
	for (i = 0; i < f->nfiles; i++) {
		f->files[i] = fo->files[i];
		if (f->files[i] != nil)
			f->files[i]->refs++;
	}
	
	f->refs = 1;
	
	return f;

}
