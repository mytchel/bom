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
	
	printf("%i: started with fd %i\n", pid, fd);
	while (true) {
		sleep(1000);
		printf("%i: read from pipe\n", pid);
		r = read(fd, (void *) &i, sizeof(int));
		if (r <= 0) {
			printf("%i: failed to read\n", pid);
			break;
		}
		
		printf("%i: read %i\n", pid, i);
		i *= 2;
		printf("%i: write %i\n", pid, i);
		r = write(fd, (void *) &i, sizeof(int));
		if (r <= 0) {
			printf("%i: failed to write\n", pid);
			break;
		}
	}
	
	printf("%i: exiting\n", pid);

	return 1;
}

int
task2(int fd)
{
	int i, j;
	int pid = getpid();
	printf("%i: started with fd %i\n", pid, fd);
	
	for (i = 0; i < 10; i++) {
		printf("%i: writing %i\n", pid, i);

		if (write(fd, (void *) &i, sizeof(int)) <= 0) {
			printf("%i: write failed\n", pid);
			break;
		}
		
		printf("%i: now read\n", pid);
		if (read(fd, (void *) &j, sizeof(int)) <= 0) {
			printf("%i: read failed\n", pid);
			break;
		}
		
		printf("%i: read %i\n", pid, j);
	}
	
	printf("%i: exiting\n", pid);

	return 2;
}

int
task3(void)
{
	char data[10];
	int fd;
	int pid = getpid();
	
	printf("%i: started\n", pid);
	
	fd = open("test", O_RDONLY);
	if (fd == -1) {
		printf("open failed\n");
		return 3;
	}
	
	printf("%i: fd = %i\n", pid, fd);
	
	if (read(fd, data, sizeof(data)) > 0) {
		data[9] = 0; /* to be sure. */
		printf("%i: read: '%s'\n", pid, data);
	} else {
		printf("%i: read failed.\n", pid);
	}
	
	close(fd);
	
	return 3;
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
		close(fds[0]);
		return task1(fds[1]);
	}
	
	sleep(1000);
	
	printf("Fork twice\n");
	f = fork(FORK_cfgroup | FORK_cmem);
	if (!f) {
		close(fds[1]);
		return task2(fds[0]);
	}
	
	sleep(1000);
	
	printf("Fork thrice\n");
	f = fork(FORK_cfgroup | FORK_cmem);
	if (!f) {
		close(fds[0]);
		close(fds[1]);
		return task3();
	}
	
	printf("tasks initiated\n");

	sleep(1000);

	printf("main task exiting\n");
	
	return 0;
}
