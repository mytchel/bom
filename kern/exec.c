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

#include "head.h"
#include <elf.h>

int
kexec(struct chan *f, int argc, char *argv[])
{
  struct elf_header_head head;
  struct elf_header32 hdr;
  struct elf_pheader32 phdr;
  int r, i;

  if ((r = fileread(f, &head, sizeof(head))) != sizeof(head)) {
    return r;
  }

  if (head.e_ident[0] != 0x7F ||
      head.e_ident[1] != 'E' ||
      head.e_ident[2] != 'L' ||
      head.e_ident[3] != 'F') {

    printf("not an elf file.\n");
    return ERR;
  }

  if (head.e_class != 1) {
    printf("is not 32 bit.\n");
    return ERR;
  }

  if ((r = fileread(f, &hdr, sizeof(hdr))) != sizeof(hdr)) {
    return r;
  }

  printf("entry at 0x%h\n", hdr.e_entry);
  printf("pheader at 0x%h\n", hdr.e_phoff);
  printf("sheader at 0x%h\n", hdr.e_shoff);

  if ((r = fileseek(f, hdr.e_phoff, SEEK_SET)) != OK) {
    return r;
  }

  printf("read %i program headers\n", hdr.e_phnum);
  for (i = 0; i < hdr.e_phnum; i++) {
    if ((r = fileread(f, &phdr, sizeof(phdr))) != sizeof(phdr)) {
      printf("failed to read phdr\n");
      return r;
    }

    printf("pheader %i\n", i);
    printf("type = %i\n", phdr.p_type);
    printf("offset = 0x%h\n", phdr.p_offset);
    printf("vaddr = 0x%h\n", phdr.p_vaddr);
    printf("fsize = 0x%h\n", phdr.p_filesz);
    printf("msize = 0x%h\n", phdr.p_memsz);
    printf("flags = 0x%h\n", phdr.p_flags);
    printf("align = %i\n", phdr.p_align);
  }

  return ERR;
}
