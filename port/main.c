#include "dat.h"
#include "../port/com.h"
#include "../port/syscall.h"
#include "../port/std.h"

static void
task1_func(void *arg)
{
	int j, i = 0;
	kprintf("task1 started\n");
	while (i < 100) {
		for (j = 0; j < 1000; j++);
		kprintf("task 1 i = %i\n", i++);
	}
	
	kprintf("task1 finished\n");
	exit(0);
}

static void
task2_func(void *arg)
{
	int i = 0, j, fd = -1;
	kprintf("task2 started\n");
	while (i < 100) {
		for (j = 0; j < 1000000; j++);
		kprintf("task2 %i\n", i++);
		fd = open("test", 0);
		kprintf("fd = %i\n", fd);
	}
	
	exit(1);
}

static void
task3_func(void *arg)
{
	int j, i = 0, c = 0;
	kprintf("task3 started\n");
	while (i < 10) {
		for (j = 0; j < 100000; j++);
		kprintf("task3 child %i: %i\n", c, i++);
			/*
		if (fork() == 0) {
			kprintf("child\n");
			c++;
		}
		*/
	}
	
	exit(2);
}

static void
task4_func(void *arg)
{
	int i, j = 0;
	reg_t *addr = (reg_t *) 1;
	kprintf("task4 started\n");
	while (j++ < 10) {
		kprintf("accessing 0x%h\n", addr);

		i = (int) (*addr);
		kprintf("got 0x%h\n", i);
		
		addr = addr + 100;
	}
	
	exit(3);
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

	kprintf("tasks initiated\nmain exiting...\n");
	
	exit(0);
}
