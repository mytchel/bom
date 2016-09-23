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

static volatile struct uart_struct *uart = nil;

bool
uartinit(void)
{
  size_t size = UART0_LEN;
	
  uart = (struct uart_struct *)
    getmem(MEM_io, (void *) UART0, &size);

  return !(uart == nil);
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
	
  for (i = 0; i < len && s[i]; i++) {
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

void
dprintf(const char *fmt, ...)
{
  char str[128];
  size_t i;
  va_list ap;

  va_start(ap, fmt);
  i = vsnprintf(str, 128, fmt, ap);
  va_end(ap);

  if (i > 0) {
    puts((uint8_t *) str, i);
  }
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
comread(struct request *req, struct response *resp)
{
  uint32_t offset, len;
  uint8_t *buf;

  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&len, buf, sizeof(uint32_t));

  resp->buf = malloc(sizeof(uint8_t) * len);
  if (resp->buf == nil) {
    resp->lbuf = 0;
    resp->ret = ENOMEM;
  } else {
    resp->lbuf = gets(resp->buf, len);
    resp->ret = OK;
  }
}

static void
comwrite(struct request *req, struct response *resp)
{
  uint32_t offset, len;
  uint8_t *buf;

  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);
  memmove(&len, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);

  resp->buf = malloc(sizeof(uint32_t));
  if (resp->buf == nil) {
    resp->lbuf = 0;
    resp->ret = ENOMEM;
  } else {
    resp->lbuf = sizeof(uint32_t);
    resp->ret = OK;
    *(resp->buf) = puts(buf, len);
  }
}

static struct fsmount mount = {
  &comopen,
  &comclose,
  nil,
  &comread,
  &comwrite,
  nil,
  nil,
};

int
commount(char *path)
{
  int f, fd, p1[2], p2[2];
 
  if (pipe(p1) == ERR) {
    return -1;
  } else if (pipe(p2) == ERR) {
    return -1;
  }

  fd = open(path, O_WRONLY|O_CREATE, ATTR_wr|ATTR_rd);
  if (fd < 0) {
    return -2;
  }
  
  if (bind(p1[1], p2[0], path) == ERR) {
    return -3;
  }

  close(p1[1]);
  close(p2[0]);

  f = fork(FORK_sngroup);
  if (f < 0) {
    return -4;
  } else if (!f) {
    if (!uartinit()) {
      return -1;
    }

    f = fsmountloop(p1[0], p2[1], &mount);
    dprintf("com mount at %s exiting!\n", path);
    exit(f);
  }

  close(p1[0]);
  close(p2[1]);
  close(fd);

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
