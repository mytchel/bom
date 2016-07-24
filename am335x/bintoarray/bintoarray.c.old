#include <stdio.h>

int
main(int argc, char *argv[])
{
	char b;
	int l = 0;
	
	FILE *f = fopen(argv[2], "r");
	if (!f) {
		return 1;
	}
		
	printf("char %s[] = {", argv[1]);
	while (fread(&b, 1, 1, f)) {
		printf("%i,", b);
		l++;
	}
	
	printf("};\n");
	
	printf("int %slen = %i;\n", argv[1], l);
	
	fclose(f);
	return 0;
}
