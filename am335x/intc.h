#ifndef __INTC_H
#define __INTC_H

void intc_init(void);

void intc_irq_handler(void);

void intc_add_handler(uint32_t irqn, void (*func)(uint32_t));

#endif
