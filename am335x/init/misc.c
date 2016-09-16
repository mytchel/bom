#include <types.h>

int
__bitfield(uint32_t *src, int start, int len)
{
	uint8_t *sp;
	uint32_t dst, mask;
	int shift, bs, bc;

	if (start < 0 || len < 0 || len > 32)
		return 0;

	dst = 0;
	mask = len % 32 ? UINT_MAX >> (32 - (len % 32)) : UINT_MAX;
	shift = 0;

	while (len > 0) {
		sp = (uint8_t *)src + start / 8;
		bs = start % 8;
		bc = 8 - bs;
		if (bc > len)
			bc = len;
		dst |= (*sp >> bs) << shift;
		shift += bc;
		start += bc;
		len -= bc;
	}

	dst &= mask;
	return (int)dst;
}

