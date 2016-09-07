#include "../include/libc.h"

#define UART0   	0x44E09000
#define UART0_LEN   	0x1000

struct uart_struct {
  uint32_t hr;
  uint32_t p1;
  uint32_t p2;
  uint32_t lcr;
  uint32_t p4;
  uint32_t lsr;
};

static volatile struct uart_struct *uart;

bool
uartinit(void)
{
  size_t size = UART0_LEN;
	
  uart = (struct uart_struct *)
    getmem(MEM_io, (void *) UART0, &size);

  if (uart == nil)
    return false;
  else
    return true;
}

size_t
getc(uint8_t *s, size_t len)
{
  size_t i;
	
  for (i = 0; i < len; i++) {
    while ((uart->lsr & (1 << 0)) == 0)
      ;
		
    s[i] = (uint8_t) (uart->hr & 0xff);
  }
	
  return i;
}

size_t
putc(uint8_t *s, size_t len)
{
  size_t i;
	
  for (i = 0; i < len; i++) {
    if (s[i] == '\n') {
      while ((uart->lsr & (1 << 5)) == 0)
	;
      uart->hr = '\r';
    }

    while ((uart->lsr & (1 << 5)) == 0)
      ;
    uart->hr = s[i];
  }
	
  return i;
}
