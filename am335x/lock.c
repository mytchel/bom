#include "head.h"

void
lock(int *l)
{
  disableintr();
	
  while (*l > 0) {
    schedule();
  }
	
  (*l)++;
  enableintr();
}

void
unlock(int *l)
{
  disableintr();
  (*l)--;
  enableintr();
}
