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

    return ERR;
  }

  if (head.e_class != 1) {
    return ERR;
  }

  if ((r = fileread(f, &hdr, sizeof(hdr))) != sizeof(hdr)) {
    return r;
  }

  mnew = mgroupnew();
  if (mnew == nil) {
    return ERR;
  }
  
  plprev = nil;
  
  for (i = 0; i < hdr.e_phnum; i++) {

    if ((r = fileseek(f, hdr.e_phoff
		      + i * hdr.e_phentsize,
		      SEEK_SET)) != OK) {

      mgroupfree(mnew);
      return r;
    }

    if ((r = fileread(f, &phdr, sizeof(phdr))) != sizeof(phdr)) {
      mgroupfree(mnew);
      return r;
    }

    if ((r = fileseek(f, phdr.p_offset, SEEK_SET)) != OK) {
      mgroupfree(mnew);
      return r;
    }

    s = 0;
    while (s < phdr.p_memsz) {
      pg = getrampage();
      if (pg == nil) {
	mgroupfree(mnew);
	return ERR;
      }

      if (phdr.p_filesz > s) {
	if (phdr.p_filesz - s > PAGE_SIZE) {
	  ss = PAGE_SIZE;
	} else {
	  ss = phdr.p_filesz - s;
	}

	if ((r = fileread(f, pg->pa, ss)) != ss) {
	  pagefree(pg);
	  mgroupfree(mnew);
	  return r;
	}
      } else {
	ss = 0;
      }
    
      if (ss < PAGE_SIZE) {
	memset(pg->pa + ss, 0, PAGE_SIZE - ss);
      }
    
      pl = wrappage(pg, (void *) (phdr.p_vaddr + s), true, true);
      if (pl == nil) {
	pagefree(pg);
	mgroupfree(mnew);
	return ERR;
      }

      if (plprev == nil) {
	mnew->pages = pl;
      } else {
	plprev->next = pl;
      }

      plprev = pl;

      s += PAGE_SIZE;
    }
  }

  chanfree(f);
  
  nmmu = mmunew();
  if (nmmu == nil) {
    mgroupfree(mnew);
    return ERR;
  }

  mmufree(up->mmu);
  up->mmu = nmmu;

  mgroupfree(up->mgroup);
  up->mgroup = mnew;

  readyexec(&ureg, (void *) hdr.e_entry, argc, argv);
 
  setintr(INTR_OFF);

  mmuswitch(up);
  
  droptouser(&ureg, up->kstack->pa + PAGE_SIZE);

  setintr(INTR_ON);
  return ERR;
}
