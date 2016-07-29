#include "../include/types.h"
#include "../include/libc.h"
#include "../include/fs.h"

struct dir *
walkresponsetodir(uint8_t *buf, uint32_t len)
{
	struct dir *d;
	int fi, i = 0;
	
	if (len < sizeof(struct dir))
		return nil;

	d = (struct dir *) buf;
	i += sizeof(struct dir);
	
	if (d->nfiles == 0)
		return d;

	if (i >= len) return nil;
	d->files = (struct file **) &buf[i];
	i += sizeof(struct file *) * d->nfiles; 

	for (fi = 0; fi < d->nfiles; fi++) {
		if (i % sizeof(void *) > 0)
			i += i % sizeof(void *);
		
		if (i >= len) return nil;
		d->files[fi] = (struct file *) &buf[i];
		i += sizeof(struct file);

		if (i >= len) return nil;
		d->files[fi]->name = &buf[i];
		i += sizeof(uint8_t) * d->files[fi]->lname;
	}
	
	return d;
}

uint8_t *
dirtowalkresponse(struct dir *dir, uint32_t *size)
{
	uint8_t *buf;
	struct file *f;
	uint32_t i, fi;
	
	i = sizeof(struct dir);
	i += sizeof(struct file *) * dir->nfiles;
	for (fi = 0; fi < dir->nfiles; fi++) {
		if (i % sizeof(void *) > 0)
			i += i % sizeof(void *);
		
		i += sizeof(struct file);
		i += sizeof(uint8_t) * dir->files[fi]->lname;
	}
	
	*size = i;

	buf = malloc(i);
	if (buf == nil) {
		*size = 0;
		return nil;
	}
	
	i = 0;
	memmove(&buf[i], (const void *) dir, sizeof(struct dir));
	i += sizeof(struct dir);
	i += sizeof(struct file *) * dir->nfiles;
	
	for (fi = 0; fi < dir->nfiles; fi++) {
		if (i % sizeof(void *) > 0)
			i += i % sizeof(void *);
		
		f = dir->files[fi];
		
		memmove(&buf[i], (const void *) f, sizeof(struct file));
		i += sizeof(struct file);
		memmove(&buf[i], (const void *) f->name, sizeof(uint8_t) * f->lname);
		i += sizeof(uint8_t) * f->lname;
	}
	
	return buf;
}
