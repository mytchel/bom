/*
 *   Copyright (C) 2016	Mytchel Hammond <mytchel@openmailbox.org>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <libc.h>
#include <stdarg.h>
#include <string.h>
#include <fs.h>

#define UART0   	0x44E09000
#define UART0_LEN   	0x1000

struct uart_struct {
  uint32_t hr;
  uint32_t p1;
  uint32_t p2;
  uint32_t lcr;
  uint32_t p4;
  uint32_t lsr;
};

static volatile struct uart_struct *uart;

bool
uartinit(void)
{
  size_t size = UART0_LEN;
	
  uart = (struct uart_struct *)
    getmem(MEM_io, (void *) UART0, &size);

  if (uart == nil)
    return false;
  else
    return true;
}

static size_t
gets(uint8_t *s, size_t len)
{
  size_t i;
	
  for (i = 0; i < len; i++) {
    while ((uart->lsr & (1 << 0)) == 0)
      ;
		
    s[i] = (uint8_t) (uart->hr & 0xff);
  }
	
  return i;
}

static size_t
puts(uint8_t *s, size_t len)
{
  size_t i;
	
  for (i = 0; i < len; i++) {
    if (s[i] == '\n') {
      while ((uart->lsr & (1 << 5)) == 0)
	;
      uart->hr = '\r';
    }

    while ((uart->lsr & (1 << 5)) == 0)
      ;
    uart->hr = s[i];
  }
	
  return i;
}

static void
comopen(struct request *req, struct response *resp)
{
  resp->ret = OK;
}

static void
comclose(struct request *req, struct response *resp)
{
  resp->ret = OK;
}

static void
comwalk(struct request *req, struct response *resp)
{
  resp->ret = ENOIMPL;
}

static void
comread(struct request *req, struct response *resp)
{
  uint32_t offset, len;
  uint8_t *buf;

  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&len, buf, sizeof(uint32_t));

  resp->ret = OK;
  resp->buf = malloc(sizeof(uint8_t) * len);
  resp->lbuf = len;

  gets(resp->buf, len);
}

static void
comwrite(struct request *req, struct response *resp)
{
  uint32_t offset, len, n;
  uint8_t *buf;

  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&len, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);

  n = puts(buf, len);

  resp->lbuf = sizeof(uint32_t);
  resp->buf = malloc(sizeof(uint32_t));
  memmove(resp->buf, &n, sizeof(uint32_t));
  resp->ret = OK;
}

static void
comremove(struct request *req, struct response *resp)
{
  resp->ret = ENOIMPL;
}

static void
comcreate(struct request *req, struct response *resp)
{
  resp->ret = ENOIMPL;
}

static struct fsmount mount = {
  &comopen,
  &comclose,
  &comwalk,
  &comread,
  &comwrite,
  &comremove,
  &comcreate,
};

int
commount(char *path)
{
  int f, p1[2], p2[2];

  if (pipe(p1) == ERR) {
    return -2;
  } else if (pipe(p2) == ERR) {
    return -3;
  }

  f = open(path, O_WRONLY|O_CREATE, ATTR_wr|ATTR_rd);
  if (f < 0) {
    return -4;
  }
  
  if (bind(p1[1], p2[0], path) == ERR) {
    return -5;
  }

  close(p1[1]);
  close(p2[0]);

  f = fork(FORK_sngroup);
  if (!f) {
    if (!uartinit()) {
      return -1;
    }

    sleep(10); /* Give time for parent to close pipes */
    return fsmountloop(p1[0], p2[1], &mount);
  }

  close(p1[0]);
  close(p2[1]);

  return f;
}

void
printf(const char *fmt, ...)
{
  char str[128];
  size_t i;
  va_list ap;
	
  va_start(ap, fmt);
  i = vsnprintf(str, 128, fmt, ap);
  va_end(ap);

  if (i > 0) {
    write(stdout, str, i);
  }
}

