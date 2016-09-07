#include "head.h"

void
lock(int *l)
{
  disableintr();
	
  while (*l > 0) {
    printf("lock taken, %i waiting\n", current->pid);
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
