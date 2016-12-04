/*
 *
 * Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
 * 
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 * 
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef _MEM_H_
#define _MEM_H_

/* Memory system calls */

#define MEM_ram     (1<<0)
#define MEM_io      (1<<1)
#define MEM_file    (1<<2)
#define MEM_rw      (1<<3)

/* 
 * Maps a number of pages necessary to contain size and 
 * returns the address. Sets size to the real size returned
 * which will be a multiple of page size. If addr is not null
 * then it should be the physical address of the memory wanted.
 * This is so you can request memory mapped IO.
 */
void *
mmap(int flags, size_t len, int fd, size_t offset, void *addr);

/*
 * Unmaps the pages starting at addr. 
 * Addr must be page aligned and should be something returned
 * from getmem. Size should be a multiple of a page.
 */
int
munmap(void *addr, size_t size);

void *
malloc(size_t size);

void
free(void *addr);

void *
memmove(void *dest, const void *src, size_t len);

void *
memset(void *dest, int c, size_t len);

uint16_t
intcopylittle16(uint8_t *src);

uint32_t
intcopylittle32(uint8_t *src);

uint16_t
intcopybig16(uint8_t *src);

uint32_t
intcopybig32(uint8_t *src);

void
intwritelittle16(uint8_t *dest, uint16_t v);

uint32_t
intwritelittle32(uint8_t *dest, uint32_t v);

#endif
