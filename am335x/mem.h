#ifndef __MEM_H
#define __MEM_H

#define L1_FAULT	0
#define L1_COARSE	1
#define L1_SECTION	2
#define L1_FINE		3

#define L2_FAULT	0
#define L2_LARGE	1
#define L2_SMALL	2
#define L2_TINY		3

struct fblock {
	struct fblock *next;
	uint32_t size;
};

void
mmuinvalidate(void);

void
mmuenable(void);

void
mmudisable(void);

void
imap(uint32_t start, uint32_t end);
	
void
memoryinit(void);

void
mmuempty1(void);

int
pageinit(void);

int
heapinit(void);

int
mmuinit(void);


#endif
