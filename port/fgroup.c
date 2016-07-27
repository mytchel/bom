#include "dat.h"

struct fgroup *
newfgroup(void)
{
	struct fgroup *f;
	
	f = malloc(sizeof(struct fgroup));
	if (f == nil) {
		return nil;
	}

	f->chans = malloc(sizeof(struct chan*));
	if (f->chans == nil) {
		free(f);
		return nil;
	}
	
	f->chans[0] = nil;
	f->nchans = 1;
	f->refs = 1;
	f->lock = 0;
	
	return f;
}

void
freefgroup(struct fgroup *f)
{
	int i;
	struct chan *c;
	
	lock(&f->lock);
	
	f->refs--;
	if (f->refs > 0) {
		unlock(&f->lock);
		return;
	}

	for (i = 0; i < f->nchans; i++) {
		c = f->chans[i];
		if (c != nil) {
			freechan(c);
		}
	}
	
	free(f->chans);
	free(f);
}

struct fgroup *
copyfgroup(struct fgroup *fo)
{
	int i;
	struct fgroup *f;
	
	lock(&fo->lock);
	
	f = malloc(sizeof(struct fgroup));
	if (f == nil) {
		unlock(&fo->lock);
		return nil;
	}

	f->chans = malloc(sizeof(struct chan*) * fo->nchans);
	if (f->chans == nil) {
		free(f);
		unlock(&fo->lock);
		return nil;
	}
	
	f->nchans = fo->nchans;
	for (i = 0; i < f->nchans; i++) {
		f->chans[i] = fo->chans[i];
		if (f->chans[i] != nil)
			f->chans[i]->refs++;
	}
	
	f->refs = 1;
	f->lock = 0;

	unlock(&fo->lock);
	return f;
}

int
addchan(struct fgroup *f, struct chan *chan)
{
	int i, fd;
	
	lock(&f->lock);
	
	for (i = 0; i < f->nchans; i++)
		if (f->chans[i] == nil)
			break;
	
	if (i < f->nchans) {
		fd = i;
	} else {
		/* Need to grow file table. */
		struct chan **chans;
		
		chans = malloc(sizeof(struct chan **) * f->nchans * 2);
		if (chans == nil) {
			unlock(&f->lock);
			return -1;
		}
		
		for (i = 0; i < f->nchans; i++)
			chans[i] = f->chans[i];

		fd = i++;

		while (i < f->nchans * 2)
			chans[i++] = nil;

		free(f->chans);
				
		f->chans = chans;
		f->nchans *= 2;
	}
	
	f->chans[fd] = chan;
	unlock(&f->lock);
	return fd;
}

struct chan *
fdtochan(struct fgroup *f, int fd)
{
	struct chan *c = nil;
	
	lock(&f->lock);
	
	if (fd < f->nchans)
		c = f->chans[fd];
	
	unlock(&f->lock);
	return c;
}
