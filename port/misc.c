#include "dat.h"
#include "../port/com.h"

void
panic(const char *str)
{
	puts(str);
	puts("Hanging\n");
	while (1);
}
