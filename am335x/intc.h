#ifndef __ARM_INTC
#define __ARM_INTC

void intc_init(void);

void intc_irq_handler(void);

void intc_add_handler(uint32_t irqn, void (*func)(uint32_t));

#endif
