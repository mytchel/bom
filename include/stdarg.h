#ifndef _STDARG_H_
#define _STDARG_H_

/* Suprisingly this seems to work. */

typedef char* va_list;

#define va_start(ap, last)	(ap = (va_list) &last + sizeof(last))
#define va_end(ap)		(ap = 0)
#define va_arg(ap, type)	((type) *((type *) ap)); ap += sizeof(type)

#endif 
