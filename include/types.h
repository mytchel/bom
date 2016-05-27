#ifndef __TYPES
#define __TYPES

/* 7.18.1.1 Exact-width integer types */
typedef	signed char		int8_t;
typedef	unsigned char		uint8_t;
typedef	short			int16_t;
typedef	unsigned short		uint16_t;
typedef	int			int32_t;
typedef	unsigned int		uint32_t;
typedef	long long		int64_t;
typedef	unsigned long long	uint64_t;

/* 7.18.1.4 Integer types capable of holding object pointers */
typedef	long			intptr_t;
typedef	unsigned long		uintptr_t;

/* Register size */
typedef long			register_t;

/* Standard system types */
typedef double			double_t;
typedef float			float_t;
typedef long			ptrdiff_t;
typedef	unsigned long		size_t;
typedef	long			ssize_t;
typedef	char *			va_list;

#endif
