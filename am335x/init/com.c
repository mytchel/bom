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
#include <mem.h>
#include <stdarg.h>
#include <string.h>
#include <fs.h>
#include <fssrv.h>

#define UART0   	0x44E09000
#define UART0_LEN   	0x1000
#define UART0_INTR      72

#define BUFMAX          512

struct uart_struct {
  uint32_t hr;
  uint32_t ier;
  uint32_t iir;
  uint32_t lcr;
  uint32_t mcr;
  uint32_t lsr;
  uint32_t tcr;
  uint32_t spr;
  uint32_t mdr1;
  uint32_t mdr2;
  uint32_t txfll;
  uint32_t txflh;
  uint32_t rxfll;
  uint32_t rxflh;
  uint32_t blr;
  uint32_t uasr;
  uint32_t scr;
  uint32_t ssr;
  uint32_t eblr;
  uint32_t mvr;
  uint32_t sysc;
  uint32_t syss;
  uint32_t wer;
  uint32_t cfps;
  uint32_t rxfifo_lvl;
  uint32_t txfifo_lvl;
  uint32_t ier2;
  uint32_t isr2;
  uint32_t freq_sel;
  uint32_t mdr3;
  uint32_t tx_dma_threshold;
};

struct readreq {
  uint32_t rid;
  uint32_t len;
  uint8_t *buf;
  struct readreq *next;
};

static volatile struct uart_struct *uart = nil;

static struct readreq *readrequests;
static int lock;
static int fsin, fsout;

static struct stat comstatstruct = {
  ATTR_wr|ATTR_rd,
  0
};

static void
getlock(void)
{
  int s;

  s = 1;
  while (!testandset(&lock)) {
    sleep(s);
    s *= 2;
  }
}

static void
freelock(void)
{
  lock = 0;
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

void
dprintf(const char *fmt, ...)
{
  char str[32];
  size_t i;
  va_list ap;

  va_start(ap, fmt);
  i = vsnprintf(str, sizeof(str), fmt, ap);
  va_end(ap);

  if (i > 0) {
    puts((uint8_t *) str, i);
  }
}

static int
readloop(void)
{
  struct response_head resp;
  struct readreq *req;
  uint8_t data[BUFMAX];
  uint32_t done;
  uint8_t i;
  
  uart->ier = 1;

  while (true) {
    while ((req = readrequests) == nil)
      sleep(0);

    getlock();

    readrequests = req->next;

    freelock();

    done = 0;
    while (done < req->len) {
      i = 0xff;
      while (i-- > 0) {
	if (uart->lsr & 1) {
	  goto copy;
	}
      }

      waitintr(UART0_INTR);

    copy:
      data[done++] = uart->hr & 0xff;
    }

    resp.rid = req->rid;
    resp.ret = OK;

    free(req);

    getlock();
    
    if (write(fsout, &resp, sizeof(resp)) < 0) {
      return ELINK;
    }
    
    if (write(fsout, data, done) < 0) {
      return ELINK;
    }

    freelock();
  }

  return OK;
}

static void
addreadreq(struct request_read *req)
{
  struct readreq *read, *r;

  read = malloc(sizeof(struct readreq));
  if (read == nil) {
    dprintf("com: Error mallocing read request!\n");
    return;
  }

  read->rid = req->head.rid;
  read->len = req->body.len;
  read->next = nil;

  getlock();
  
  for (r = readrequests; r != nil && r->next != nil; r = r->next)
    ;

  if (r == nil) {
    readrequests = read;
  } else {
    r->next = read;
  }

  freelock();
}

static void
comstat(struct request_stat *req, struct response_stat *resp)
{
  memmove(&resp->body.stat, &comstatstruct, sizeof(struct stat));
  resp->head.ret = OK;
}

static void
comopen(struct request_open *req, struct response_open *resp)
{
  resp->head.ret = OK;
  resp->body.minchunk = 1;
  resp->body.maxchunk = BUFMAX;
}

static void
comclose(struct request_close *req, struct response_close *resp)
{
  resp->head.ret = OK;
}

static void
comwrite(struct request_write *req, struct response_write *resp)
{
  resp->body.len = puts(req->body.data, req->body.len);
  resp->head.ret = OK;
}

int
comfsmountloop(void)
{
  struct response *resp;
  struct request *req;
  uint8_t wdata[BUFMAX];
  size_t len;

  req = malloc(sizeof(struct request));
  if (req == nil) {
    return ENOMEM;
  }
  
  resp = malloc(sizeof(struct response));
  if (resp == nil) {
    free(req);
    return ENOMEM;
  }

  while (true) {
    if ((len = read(fsin, req, sizeof(struct request))) < 0) {
      return ELINK;
    }

    resp->head.rid = req->head.rid;

    switch (req->head.type) {
    case REQ_stat:
      len = sizeof(struct response_stat);
      comstat((struct request_stat *) req,
	      (struct response_stat *) resp);
      break;

    case REQ_open:
      len = sizeof(struct response_open);
      comopen((struct request_open *) req,
	      (struct response_open *) resp);
      break;

    case REQ_close:
      len = sizeof(struct response_close);
      comclose((struct request_close *) req,
	      (struct response_close *) resp);
      break;

    case REQ_write:
      req->write.data = wdata;
      if (read(fsin, wdata, req->write.len) < 0) {
	return ELINK;
      }

      len = sizeof(struct response_write);
      comwrite((struct request_write *) req,
	      (struct response_write *) resp);
      break;

    case REQ_read:
      addreadreq((struct request_read *) req);
      /* Dont write a response */
      continue;
      break;

    default:
      resp->head.ret = ENOIMPL;
      len = sizeof(struct response_head);
    }

    getlock();

    if (write(fsout, resp, len) < 0) {
      dprintf("com failed to write response %i\n", resp->head.rid);
      return ELINK;
    }
    
    freelock();
  }

  return 0;
}
 
int
commount(char *path)
{
  int f, fd, p1[2], p2[2];
  size_t size = UART0_LEN;

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
  if (f > 0) {
    close(p1[0]);
    close(p2[1]);
    close(fd);
    return f;
  }

  fsout = p2[1];
  fsin = p1[0];
 
  uart = (struct uart_struct *)
    getmem(MEM_io, (void *) UART0, &size);

  if (uart == nil) {
    exit(-4);
  }

  f = fork(FORK_smem|FORK_sngroup|FORK_sfgroup);
  if (f == 0) {
    f = readloop();
  } else {
    f = comfsmountloop();
  }

  exit(f);
}
