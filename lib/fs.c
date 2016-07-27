#include "../include/types.h"
#include "../include/libc.h"
#include "../include/fs.h"

struct dir *
walkresponsetodir(uint8_t *buf, uint32_t len)
{
	struct dir *d;
	int i;
	
	d = (struct dir *) buf;
	
	buf += sizeof(struct dir);
	d->files = (struct file **) buf;
	buf += sizeof(struct file *) * d->nfiles; 

	for (i = 0; i < len && i < d->nfiles; i++) {
		d->files[i] = (struct file *) buf;
		buf += sizeof(struct file);
		d->files[i]->name = buf;
		buf += sizeof(uint8_t) * d->files[i]->lname;
	}
	
	return d;
}

uint8_t *
dirtowalkresponse(struct dir *dir, uint32_t *size)
{
	uint8_t *buf, *b;
	struct file *f;
	uint32_t i;
	
	*size = sizeof(struct dir);
	
	for (i = 0; i < dir->nfiles; i++) {
		*size += sizeof(struct file *) + sizeof(struct file);
		*size += sizeof(uint8_t) * dir->files[i]->lname;
	}
	
	buf = malloc(*size);
	if (buf == nil) {
		return nil;
	}
	
	b = buf;
	
	memmove(b, (const void *) dir, sizeof(struct dir));
	b += sizeof(struct dir);
	b += sizeof(struct file *) * dir->nfiles;
	
	for (i = 0; i < dir->nfiles; i++) {
		f = dir->files[i];
		memmove(b, (const void *) f, sizeof(struct file));
		b += sizeof(struct file);
		memmove(b, (const void *) f->name, sizeof(uint8_t) * f->lname);
		b += sizeof(uint8_t) * f->lname;
	}
	
	return buf;
}
