/*	$OpenBSD: _types.h,v 1.17 2015/09/26 16:01:00 guenther Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)types.h	8.3 (Berkeley) 1/5/94
 *	@(#)ansi.h	8.2 (Berkeley) 1/4/94
 */

#ifndef _ARM_TYPES
#define _ARM_TYPES

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
