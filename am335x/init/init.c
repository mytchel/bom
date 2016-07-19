#include "../include/types.h"
#include "../include/std.h"
#include "../include/stdarg.h"

/* Must not excede one page in size and all changabel variables 
 * must be on the stack. */

char
getc(void);

void
puts(const char *);

void
printf(const char *, ...);

int
task1(int fd)
{
	int i, r;
	int pid = getpid();
	
	printf("%i: task 1 started with fd %i\n", pid, fd);
	while (true) {
		r = read(fd, (void *) &i, sizeof(int));
		if (r <= 0) {
			printf("%i: failed to read\n", pid);
			break;
		}
		
		printf("%i: read %i\n", pid, i);
	}
	
	printf("%i: exiting\n", pid);

	return 1;
}

int
task2(int fd)
{
	int i;
	int pid = getpid();
	printf("%i: task 2 started with fd %i\n", pid, fd);
	
	for (i = 0; i < 4; i++) {
		sleep(2000);
		
		printf("%i: writing %i\n", pid, i);

		if (write(fd, (void *) &i, sizeof(int)) <= 0) {
			printf("%i: write failed\n", pid);
			break;
		}
	}
	
	printf("%i: exiting\n", pid);

	return 2;
}

int
task3(void)
{
	int fds[2], r;
	char buf[32];
	int pid = getpid();
	
	printf("%i: task 3 started\n", pid);
	
	if (pipe(fds) == -1) {
		printf("%i: pipe failed\n", pid);
		return ERR;
	}

	bind(fds[0], "/com", 0);
	
	while (true) {
		r = read(fds[1], (void *) buf, sizeof(buf));
		if (r <= 0) {
			printf("%i: read failed\n", pid);
			break;
		}
		
		buf[r] = 0;
		printf("%i: read: '%s'\n", pid, buf);
	}
	
	printf("%i: exiting\n", pid);
	return 3;
}

int
task4(void)
{
	int fd;
	int pid = getpid();
	char data[32] = {'h', 'i', 0};
	
	printf("%i: task 4 started\n", pid);
	
	fd = open("/com", O_RDONLY);
	if (fd == -1) {
		printf("open failed\n");
		return 3;
	}
	
	printf("%i: fd = %i\n", pid, fd);
	
	if (write(fd, data, sizeof(data)) > 0) {
		printf("%i: wrote\n", pid);
	} else {
		printf("%i: write failed.\n", pid);
	}
	
	close(fd);
	
	return 4;
}

int
main(void)
{
	int f, fds[2];

	printf("Hello from true user space!\n");
	
	if (pipe(fds) == -1) {
		printf("pipe failed\n");
		return ERR;
	}
	
	sleep(1000);
	
	printf("Fork once\n");
	f = fork(FORK_cfgroup | FORK_cmem);
	if (!f) {
		close(fds[1]);
		return task1(fds[0]);
	}
	
	sleep(1000);
	
	printf("Fork twice\n");
	f = fork(FORK_cfgroup | FORK_cmem);
	if (!f) {
		close(fds[0]);
		return task2(fds[1]);
	}
	
	sleep(1000);
/*
	printf("Fork thrice\n");
	f = fork(FORK_cfgroup | FORK_cmem);
	if (!f) {
		close(fds[0]);
		close(fds[1]);
		return task3();
	}
	
	sleep(1000);
	
	printf("Fork frice\n");
	f = fork(FORK_cfgroup | FORK_cmem);
	if (!f) {
		close(fds[0]);
		close(fds[1]);
		return task4();
	}
*/	
	printf("tasks initiated\n");

	sleep(1000);

	printf("main task exiting\n");
	
	return 0;
}
