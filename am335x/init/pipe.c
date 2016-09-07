#include "../include/libc.h"

int
ppipe0(int fd)
{
	int i, n, pid = getpid();
	
	printf("%i: ppipe0 started with fd %i\n", pid, fd);
	while (true) {
		sleep(100);
		printf("%i: wait for write\n", pid);
		n = read(fd, (void *) &i, sizeof(int));
		if (n != sizeof(int)) {
			printf("%i: failed to read. %i\n", pid, n);
			break;
		}
		
		sleep(100);
		printf("%i: read %i\n", pid, i);
	}
	
	printf("%i: exiting\n", pid);

	return 1;
}

int
ppipe1(int fd)
{
	int i, n, pid = getpid();
	printf("%i: ppipe1 started with fd %i\n", pid, fd);
	
	for (i = 0; i < 4; i++) {
		sleep(400);
		printf("%i: writing %i\n", pid, i);

		n = write(fd, (void *) &i, sizeof(int));
		if (n != sizeof(int)) {
			printf("%i: write failed. %i\n", pid, n);
			break;
		}
	}
	
	printf("%i: exiting\n", pid);

	return 2;
}
