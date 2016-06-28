#ifndef __FNS_H
#define __FNS_H

void
dumpregs(struct ureg *);

void
uregret(struct ureg *);

void
droptouser(void *);

uint32_t
fsrstatus(void);

void *
faultaddr(void);

#endif
