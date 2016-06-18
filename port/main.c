#include "types.h"
#include "../include/com.h"
#include "../include/proc.h"

static void
task1_func(void *arg)
{
	int i = 0, j;
	kprintf("task1 started\n");
	while (1) {
		for (j = 0; j < 10000; j++);
		kprintf("task 1 i = %i\n", i++);
	}
}

static void
task2_func(void *arg)
{
	int i = 0, j;
	kprintf("task2 started\n");
	while (1) {
		for (j = 0; j < 100000; j++);
		kprintf("task2 %i\n", i++);
	}
}

static void
task3_func(void *arg)
{
	int j, i = 0;
	kprintf("task3 started\n");
	while (1) {
		for (j = 0; j < 1000000; j++);
		kprintf("swi\n");
		asm("swi 0");
		kprintf("task3 %i\n", i++);
		asm("swi 1");
	}
}

static void
task4_func(void *arg)
{
	int i, j;
	uint32_t *addr = (uint32_t *) 100000;
	kprintf("task4 started\n");
	addr = arg;
	while (true) {
		for (j = 0; j < 100000; j++);
		kprintf("accessing 0x%h\n", addr);
		i = (int) (*addr);
		kprintf("got 0x%h\n", i);
		addr = addr + 10;
	}
}


void
kmain(void *arg)
{
	kprintf("in kmain\n");
	
	kprintf("adding tasks\n");
	
	proc_create(&task1_func, nil);
	proc_create(&task2_func, nil);
	proc_create(&task3_func, nil);
	proc_create(&task4_func, nil);

	kprintf("tasks initiated\n");
	
	while (1) {
		int j;
		for (j = 0; j < 100000000; j++);
		kprintf("doing nothing\n");
	}
}
