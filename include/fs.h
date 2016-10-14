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

#ifndef _FS_H_
#define _FS_H_

/* File names must be null terminated */
#define FS_NAME_MAX     255

#define ATTR_rd		(1<<0)
#define ATTR_wr		(1<<1)
#define ATTR_dir	(1<<2)

struct stat {
  uint32_t attr;

  /* If file is a directory then size is the number of 
   * files it contains. Else it is the size of the file in memory/on 
   * disk which may be different to the readable length.
   */
  uint32_t size;
};

int
stat(const char *path, struct stat *stat);

int
cleanpath(char *opath, char *cpath, size_t cpathlen);

#endif
