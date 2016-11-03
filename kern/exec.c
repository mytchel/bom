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
  struct pagel *pl, *plprev;
  struct mgroup *mnew;
  struct mmu *nmmu;
  struct label ureg;
  struct page *pg;
  size_t s, ss;
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

  memset(&ureg, 0, sizeof(ureg));
  ureg.psr = MODE_USR;
  ureg.pc = hdr.e_entry;
  ureg.sp = USTACK_TOP - PAGE_SIZE;
  
  mnew = mgroupnew();
  if (mnew == nil) {
    return ERR;
  }
  
  plprev = nil;
  
  printf("read %i program headers\n", hdr.e_phnum);

  for (i = 0; i < hdr.e_phnum; i++) {

    if ((r = fileseek(f, hdr.e_phoff
		      + i * hdr.e_phentsize,
		      SEEK_SET)) != OK) {

      mgroupfree(mnew);
      return r;
    }

    if ((r = fileread(f, &phdr, sizeof(phdr))) != sizeof(phdr)) {
      printf("failed to read phdr\n");
      mgroupfree(mnew);
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
    
    if ((r = fileseek(f, phdr.p_offset, SEEK_SET)) != OK) {
      mgroupfree(mnew);
      return r;
    }

    s = 0;
    while (s < phdr.p_filesz) {
      printf("get page for va 0x%h\n", phdr.p_vaddr + s);
      
      pg = getrampage();
      if (pg == nil) {
	mgroupfree(mnew);
	return ERR;
      }

      if (phdr.p_filesz - s > PAGE_SIZE) {
	ss = PAGE_SIZE;
      } else {
	ss = phdr.p_filesz - s;
      }

      printf("read ss bytes of page\n", ss);
      
      if ((r = fileread(f, pg->pa, ss)) != ss) {
	pagefree(pg);
	mgroupfree(mnew);
	return r;
      }

      if (ss < PAGE_SIZE) {
	printf("set rest of page zero\n");
	memset(pg->pa + ss, 0, PAGE_SIZE - ss);
      }
    
      printf("wrap page\n");
      pl = wrappage(pg, (void *) (phdr.p_vaddr + s), true, true);
      if (pl == nil) {
	pagefree(pg);
	mgroupfree(mnew);
	return ERR;
      }

      printf("add to list\n");
      if (plprev == nil) {
	mnew->pages = pl;
      } else {
	plprev->next = pl;
      }

      plprev = pl;

      printf("move on to next page\n");
      s += ss;
    }
  }

  printf("good, now free mgroup\n");
  
  nmmu = mmunew();
  if (nmmu == nil) {
    mgroupfree(mnew);
    return ERR;
  }

  mmufree(up->mmu);
  up->mmu = nmmu;

  mgroupfree(up->mgroup);
  up->mgroup = mnew;
 
  setintr(INTR_OFF);

  printf("switch mmu, may work?\n");

  mmuswitch(up);
  
  printf("drop to user\n");

  droptouser(&ureg, up->kstack->pa + PAGE_SIZE);

  setintr(INTR_ON);
  return ERR;
}
