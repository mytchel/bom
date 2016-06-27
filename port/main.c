#include "../port/com.h"
#include "../include/types.h"
#include "../include/std.h"

static int
task1_func(int argc, void *args)
{
	int i = 0, j;
	for (i = 0; i < 100000; i++) {
		for (j = 0; j < 0x1fffffff; j++);
		if (i % 1000 == 0)
			kprintf("task 1 i = %i\n", i);
	}
	
	kprintf("task1 finished\n");
	while (1)
		yield();
	return 1;
}
#if 0
static int
task2_func(int argc, void *args)
{
	int i;
	puts("task2 started\n");
	for (i = 0; i < 3; i++) {
		kprintf("open /\n");
		/*
		fd = open("/", 0);
		puts("ok\n");
		if (fd == -1) {
			kprintf("open failed\n");
		} else {
			kprintf("opened file %i\n", fd);
			close(fd);
		}
		*/
	}
	
	kprintf("task2 exiting\n");
	while (1)
		yield();
	return 2;
}

static int
task3_func(int argc, void *args)
{
	int i;
	for (i = 0; i < 100000; i++) {
		if (i % 3000 == 0)
			kprintf("task3 %i\n", i);
	}

	kprintf("task3 exiting\n");
	while (1)
		yield();
	return 3;
}

static int
task4_func(int argc, void *args)
{
	int i, j;
	int *addr = (int *) 1;
	
	kprintf("task4 started with pid %i\n", getpid());
	
	for (i = 0; i < 10; i++) {
		for (j = 0; j < 0x1fffffff; j++);
		kprintf("accessing 0x%h\n", addr);
		/*
		w = (int) (*addr);
		kprintf("got 0x%h\n", w);
		*/
		addr += 100;
	}

	kprintf("task4 exiting\n");
	while (1)
		yield();
	return 4;
}
#endif
int
kmain()
{
	uint32_t i, j, k;
	kprintf("adding tasks\n");
	
	if (!fork()) {
		task1_func(0, nil);
	}

	for (i = 0; i < 0xffffffff; i++)
		for (j = 0; j < 0xffffffff; j++)
			for (k = 0; k < 0xffffffff; k++);
	kprintf("timed out.\n");
	return kmain();
	
	/*
	if (!fork()) {
		task2_func(0, nil);
	}

	if (!fork()) {
		task3_func(0, nil);
	}

	if (!fork()) {
		task4_func(0, nil);
	}

	kprintf("tasks initiated\n");

	while (1) {
		yield();
	}

	return 0;
	*/
}
