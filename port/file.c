#include "dat.h"

int
fileread(struct chan *c, void *buf, int n)
{

	return ERR;
}

int
filewrite(struct chan *c, void *buf, int n)
{
	return ERR;

}

int
fileclose(struct chan *c)
{
	return ERR;
}

struct chantype devfile = {
	&fileread,
	&filewrite,
	&fileclose,
};
