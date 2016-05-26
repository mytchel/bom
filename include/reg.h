#ifndef __REG_H_
#define __REG_H_

#define __REG_TYPE	volatile uint32_t
#define __REG		__REG_TYPE *

#define USARTx_size	(0x90)
#define USART0		((__REG) 0x44E09000)

#endif
