#include "../include/types.h"
#include "../include/std.h"
#include "../include/stdarg.h"
#include "../include/fs.h"

#include "head.h"

int
pmount(void)
{
	int pid = getpid();
	int in;
	int p1[2], p2[2];
	uint8_t buf[512];
	struct request req;
	
	printf("%i: set up pipes\n", pid);
	
        if (pipe(p1) == ERR) {
        	printf("%i: p1 pipe failed\n", pid);
        	return -3;
        } else if (pipe(p2) == ERR) {
        	printf("%i: p2 pipe failed\n", pid);
        	return -3;
        }
	
	printf("%i: bind\n", pid);

	if (bind(p1[1], p2[0], "/") < 0) {
		printf("%i: bind failed\n", pid);
		return -3;
	}

	printf("%i: close p1[1]\n", pid);
	close(p1[1]);
	printf("%i: close p2[0]\n", pid);	
	close(p2[0]);
	
	in = p1[0];

	while (true) {
		printf("%i: read requests\n", pid);
		
		if (read(in, &req, sizeof(struct request)) < 0) {
			printf("%i: failed to read in\n", pid);
			break;
		}
		
		printf("%i : Got request with rid: %i\n", pid, req.rid);
		
		if (req.n > 0) {
			req.buf = buf;
			if (read(in, req.buf, req.n) != req.n) {
				printf("%i: failed to read buf\n", pid);
				break;
			}
		}
		
		switch (req.type) {
		case REQ_open:
			printf("%i : should open file '%s'\n", pid, req.buf);
		}
	}

	return 3;
}

int
pfile_open(void)
{
	int pid = getpid();
	int fd;
	
	fd = open("/com", O_WRONLY);
	if (fd == -1) {
		printf("%i: open /com failed\n", pid);
		return -4;
	}
	
	while (true) {
		write(fd, "Hello\n", sizeof(char) * 7);
		sleep(5000);
	}
	
	return 4;
}
