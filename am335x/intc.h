#ifndef __INTC_H
#define __INTC_H

void intcinit(void);

void intcirqhandler(void);

void intcaddhandler(uint32_t, void (*)(uint32_t));

#endif
