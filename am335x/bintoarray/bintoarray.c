#include <stdio.h>
#include <unistd.h>

int
main(int argc, char *argv[])
{
	char b;
	int l = 0;
	
	printf("char %s[] = {", argv[1]);
	while (read(0, &b, sizeof(char))) {
		printf("%i,", b);
		l++;
	}
	
	printf("};\n");
	
	printf("int %slen = %i;\n", argv[1], l);
	
	return 0;
}
