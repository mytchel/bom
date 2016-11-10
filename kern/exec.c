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

static int
addpages(struct mgroup *m, reg_t addr, size_t size, bool rw)
{
  struct pagel *head, *pn, *pp;
  struct page *pg;
  size_t s;

  pg = getrampage();
  if (pg == nil) {
    return ENOMEM;
  }

  pp = head = wrappage(pg, addr, rw, true);
  if (head == nil) {
    pagefree(pg);
    return ENOMEM;
  }

  s = PAGE_SIZE;

  while (s < size) {
    pg = getrampage();
    if (pg == nil) {
      return ENOMEM;
    }

    pn = wrappage(pg, addr + s, rw, true);
    if (pn == nil) {
      pagefree(pg);
      return ENOMEM;
    }

    pp->next = pn;
    pp = pn;
    s += PAGE_SIZE;
  }

  insertpages(m, head, addr, s, false);
  return OK;
}

static int
readpheader(struct chan *f, struct elf_pheader32 *phdr,
	     struct mgroup *mnew)
{
  struct pagel *pl;
  size_t s, ss;
  reg_t addr;
  int r;

  if (phdr->type != PHT_LOAD) {
    return OK;
  }

  if ((r = fileseek(f, phdr->offset, SEEK_SET)) != OK) {
    return r;
  }

  addr = PAGE_ALIGN(phdr->vaddr);

  if ((r = addpages(mnew, addr, phdr->memsz, true)) != OK) {
    return r;
  }
    
  s = 0;
  pl = mnew->pages;
  while (pl != nil && s < phdr->filesz) {
    if (pl->va < addr) {
      continue;
    }

    ss = PAGE_SIZE;
    if (ss > phdr->filesz - s) {
      ss = phdr->filesz - s;
      memset((void *) (pl->p->pa + ss),
	     0, PAGE_SIZE - ss);
    }

    if ((r = fileread(f, (void *) pl->p->pa, ss)) != ss) {
      return r;
    }

    s += PAGE_SIZE;
    pl = pl->next;
  }

  while (pl != nil && s < phdr->memsz) {
    memset((void *) pl->p->pa, 0, PAGE_SIZE);
    s += PAGE_SIZE;
    pl = pl->next;
  }

  return OK;
}

int
kexec(struct chan *f, int argc, char *argv[])
{
  struct elf_header_head head;
  struct elf_pheader32 phdr;
  struct elf_header32 hdr;
  struct mgroup *mnew;
  struct mmu *nmmu;
  struct label ureg;
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

  for (i = 0; i < hdr.e_phnum; i++) {
    if ((r = fileseek(f, hdr.e_phoff
		      + i * hdr.e_phentsize,
		      SEEK_SET)) != OK) {
      return r;
    }

    if ((r = fileread(f, &phdr, sizeof(phdr))) != sizeof(phdr)) {
      return r;
    }

    if ((r = readpheader(f, &phdr, mnew)) != OK) {
      mgroupfree(mnew);
      return r;
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

  mmuswitch();
  
  droptouser(&ureg, (void *) (up->kstack->pa + PAGE_SIZE));

  setintr(INTR_ON);
  return ERR;
}
