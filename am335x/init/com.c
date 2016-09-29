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

  resp->buf = malloc(len);
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
  uint32_t offset;
  uint8_t *buf;

  buf = req->buf;
  memmove(&offset, buf, sizeof(uint32_t));
  buf += sizeof(uint32_t);

  resp->buf = malloc(sizeof(uint32_t));
  if (resp->buf == nil) {
    resp->lbuf = 0;
    resp->ret = ENOMEM;
  } else {
    resp->lbuf = sizeof(uint32_t);
    resp->ret = OK;
    *(resp->buf) = puts(buf, req->lbuf - sizeof(uint32_t));
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
