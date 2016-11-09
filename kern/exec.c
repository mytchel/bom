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
readpheaders(struct chan *f, struct elf_header32 hdr,
	     struct mgroup *mnew)
{
  struct elf_pheader32 phdr;
  struct pagel *pl;
  size_t s, ss, off;
  int i, r;

  printf("read %i program headers\n", hdr.e_phnum);

  for (i = 0; i < hdr.e_phnum; i++) {

    if ((r = fileseek(f, hdr.e_phoff
		      + i * hdr.e_phentsize,
		      SEEK_SET)) != OK) {

      return r;
    }

    if ((r = fileread(f, &phdr, sizeof(phdr))) != sizeof(phdr)) {
      return r;
    }

    if ((r = fileseek(f, phdr.p_offset, SEEK_SET)) != OK) {
      return r;
    }

    printf("copy section of 0x%h bytes to 0x%h\n", phdr.p_memsz, phdr.p_vaddr);

    s = 0;
    for (pl = mnew->pages; pl != nil; pl = pl->next) {
      printf("check in page 0x%h ?\n", pl->va);
      if ((size_t) pl->va + PAGE_SIZE < phdr.p_vaddr) {
	continue;
      } else if ((size_t) pl->va < phdr.p_vaddr) {
	off = phdr.p_vaddr - (size_t) pl->va;
      } else {
	off = 0;
      }

      printf("yes, at offset 0x%h\n", off);
      
      if (phdr.p_filesz - s > PAGE_SIZE - off) {
	ss = PAGE_SIZE - off;
      } else {
	ss = phdr.p_filesz - s - off;
      }

      printf("copy %i bytes to 0x%h\n", ss, pl->va + off);
      
      if ((r = fileread(f, pl->p->pa + off, ss)) != ss) {
	return r;
      }
    
      s += PAGE_SIZE;

      if (s >= phdr.p_filesz) {
	break;
      }
    }
  }

  return OK;
}

static int
addpage(void *addr, struct pagel *prev, bool rw)
{
  struct pagel *pn;
  struct page *pg;

  printf("get page\n");
  pg = getrampage();
  if (pg == nil) {
    return ENOMEM;
  }

  printf("zero page\n");
  memset(pg->pa, 0, PAGE_SIZE);
    
  printf("put page at va 0x%h\n", addr);
  pn = wrappage(pg, addr, rw, true);
  if (pn == nil) {
    pagefree(pg);
    return ENOMEM;
  }

  pn->next = prev->next;
  prev->next = pn;

  return OK;
}

static int
readsheaders(struct chan *f, struct elf_header32 hdr,
	     struct mgroup *mnew)
{
  struct elf_sheader32 shdr;
  struct pagel *pl;
  struct page *pg;
  bool rw = true;
  size_t s;
  int i, r;

  printf("read %i sections headers\n", hdr.e_shnum);
  for (i = 0; i < hdr.e_shnum; i++) {
    if ((r = fileseek(f, hdr.e_shoff + i * hdr.e_shentsize,
		      SEEK_SET)) != OK) {
      return r;
    }

    if ((r = fileread(f, &shdr, sizeof(shdr))) != sizeof(shdr)) {
      return r;
    }

    if (shdr.type != PROGBITS) {
      continue;
    } else if (!(shdr.flags & SHF_ALLOC)) {
      continue;
    }
    
    printf("alloc 0x%h bytes at 0x%h\n", shdr.size, shdr.addr);

    if (mnew->pages == nil) {
      printf("put first page at va 0x%h\n", shdr.addr);

      pg = getrampage();
      if (pg == nil) {
	return ENOMEM;
      }

      memset(pg->pa, 0, PAGE_SIZE);
    
      mnew->pages = wrappage(pg, (void *) shdr.addr, rw, true);
      if (mnew->pages == nil) {
	pagefree(pg);
	return ENOMEM;
      }

      s = PAGE_SIZE;
    } else {
      s = 0;
    }

    for (pl = mnew->pages;
	 pl != nil && s < shdr.size;
	 pl = pl->next) {

      printf("check page 0x%h for pos 0x%h\n", pl->va, shdr.addr + s);

      if (pl->next != nil && (reg_t) pl->next->va < shdr.addr + s) {
	printf("skip\n");
	if ((reg_t) pl->va > shdr.addr) {
	  printf("and add to s\n");
	  s += PAGE_SIZE;
	}

	continue;
      } else if ((reg_t) pl->va <= shdr.addr + s &&
		 (reg_t) pl->va + PAGE_SIZE >= shdr.addr + shdr.size) {
	printf("dont need any more\n");
	break;
      }
      
      
      printf("add page\n");
      if ((r = addpage((void *) (shdr.addr + s), pl, rw)) != OK) {
	return r;
      }
      
      s += PAGE_SIZE;
    }
  }

  for (pl = mnew->pages; pl != nil; pl = pl->next) {
    printf("page at 0x%h\n", pl->va);
  }
  
  return OK;
}

int
kexec(struct chan *f, int argc, char *argv[])
{
  struct elf_header_head head;
  struct elf_header32 hdr;
  struct mgroup *mnew;
  struct mmu *nmmu;
  struct label ureg;
  int r;

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

  r = readsheaders(f, hdr, mnew);
  if (r != OK) {
    mgroupfree(mnew);
    return r;
  }
  
  r = readpheaders(f, hdr, mnew);
  if (r != OK) {
    mgroupfree(mnew);
    return r;
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
  
  droptouser(&ureg, up->kstack->pa + PAGE_SIZE);

  setintr(INTR_ON);
  return ERR;
}
