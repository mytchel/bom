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

reg_t
sysserv(void)
{
  struct addr *a;
  int i;

  a = addrnew();
  if (a == nil) {
    return ENOMEM;
  }

  for (i = 0; i < up->agroup->naddrs; i++) {
    if (cas(&up->agroup->addrs[i], nil, a)) {
      return i;
    }
  }

  addrfree(a);
  return ERR;
}

reg_t
sysunserv(int addr)
{
  struct addr *a;
  
  if (addr >= up->agroup->naddrs) {
    return ERR;
  } else if ((a = up->agroup->addrs[addr]) == nil) {
    return ERR;
  }

  addrfree(a);

  while (!cas(&up->agroup->addrs[addr], a, nil))
    ;

  return OK;
}

reg_t
sysmessage(int addr, void *umb, void *urb)
{
  struct message m;
  struct addr *a;
  void *mb, *rb;

  mb = kaddr(up, umb, MESSAGELEN);
  rb = kaddr(up, urb, MESSAGELEN);

  if (mb == nil || rb == nil) {
    return ERR;
  }

  if (addr >= up->agroup->naddrs) {
    return ERR;
  } else if ((a = up->agroup->addrs[addr]) == nil) {
    return ERR;
  }

  m.message = mb;
  m.reply = rb;

  return kmessage(a, &m);
 }

reg_t
sysrecv(int addr, uint32_t *mid, void *umb)
{
  struct message *m;
  struct addr *a;
  void *mb;
  
  mb = kaddr(up, umb, MESSAGELEN);
  if (mb == nil) {
    return ERR;
  }

  if (addr >= up->agroup->naddrs) {
    return ERR;
  } else if ((a = up->agroup->addrs[addr]) == nil) {
    return ERR;
  }

  m = krecv(a);
  if (m == nil) {
    return ERR;
  }

  *mid = m->mid;
  memmove(mb, m->message, MESSAGELEN);
  
  return OK;
}

reg_t
sysreply(int addr, uint32_t mid, void *urb)
{
  struct addr *a;
  void *rb;
  
  rb = kaddr(up, urb, MESSAGELEN);
  if (rb == nil) {
    return ERR;
  }

  if (addr >= up->agroup->naddrs) {
    return ERR;
  } else if ((a = up->agroup->addrs[addr]) == nil) {
    return ERR;
  }

  return kreply(a, mid, rb);
}


