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

#define BUF_SIZE        512

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
static int readreqlock, fsoutlock;
static int fsin, fsout;

static struct stat comstatstruct = {
  ATTR_wr|ATTR_rd,
  0
};

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

static int
readloop(void)
{
  struct response resp;
  struct readreq *req;
  size_t respsize;
  uint32_t done;
  uint8_t buf[FSBUFMAX];

  respsize = sizeof(resp.rid)
    + sizeof(resp.ret)
    + sizeof(resp.lbuf);

  uart->ier = 1;

  while (true) {
    while ((req = readrequests) == nil)
      sleep(20);

    readrequests = req->next;

    done = 0;
    while (done < req->len) {
      waitintr(UART0_INTR);
      buf[done++] = uart->hr;
    }

    resp.rid = req->rid;
    resp.lbuf = done;
    resp.ret = OK;

    buf[done] = 0;
    while (!testandset(&fsoutlock))
      sleep(0);

    if (write(fsout, &resp, respsize) != respsize) {
      exit(ELINK);
    }

    if (write(fsout, buf, done) != done) {
      exit(ELINK);
    }

    fsoutlock = 0;
    
    free(req);
  }

  return 0;
}

static void
addreadreq(struct request *req)
{
  struct request_read *rr;
  struct readreq *read, *r;

  rr = (struct request_read *) req->buf;

  read = malloc(sizeof(struct readreq));
  if (read == nil) {
    dprintf("com: Error mallocing read request!\n");
    return;
  }

  read->rid = req->rid;
  read->len = rr->len;
  read->next = nil;

  while (!testandset(&readreqlock))
    sleep(0);
  
  for (r = readrequests; r != nil && r->next != nil; r = r->next)
    ;

  if (r == nil) {
    readrequests = read;
  } else {
    r->next = read;
  }

  readreqlock = 0;
}

static void
comstat(struct request *req, struct response *resp)
{
  resp->buf = (uint8_t *) &comstatstruct;
  resp->lbuf = sizeof(struct stat);
  resp->ret = OK;
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
comwrite(struct request *req, struct response *resp)
{
  struct request_write *rw;
  
  rw = (struct request_write *) req->buf;

  resp->ret = puts(rw->buf, rw->len);
}

int
comfsmountloop(void)
{
  uint8_t buf[FSBUFMAX];
  size_t reqsize, respsize;
  struct response resp;
  struct request req;
  int r;

  reqsize = sizeof(req.rid)
    + sizeof(req.type)
    + sizeof(req.fid)
    + sizeof(req.lbuf);

  respsize = sizeof(resp.rid)
    + sizeof(resp.ret)
    + sizeof(resp.lbuf);
  
  req.buf = buf;

  while (true) {
    if ((r = read(fsin, &req, reqsize)) != reqsize) {
      goto err;
    }

    resp.rid = req.rid;
    resp.lbuf = 0;
    resp.ret = ENOIMPL;

    if (req.lbuf > 0 && read(fsin, req.buf, req.lbuf) != req.lbuf) {
      dprintf("read buf failed!\n");
      goto err;
    }

    switch (req.type) {
    case REQ_stat:
      comstat(&req, &resp);
      break;
    case REQ_open:
      comopen(&req, &resp);
      break;
    case REQ_close:
      comclose(&req, &resp);
      break;
    case REQ_write:
      comwrite(&req, &resp);
      break;
    case REQ_read:
      addreadreq(&req);
      /* Dont write a response */
      continue;
    }

    while (!testandset(&fsoutlock))
      sleep(0);
    
    if (write(fsout, &resp, respsize) != respsize) {
      goto err;
    }
		
    if (resp.lbuf > 0) {
      if (write(fsout, resp.buf, resp.lbuf) != resp.lbuf) {
	goto err;
      }
    }

    fsoutlock = 0;
  }

  return 0;

 err:

  if (resp.lbuf > 0)
    free(resp.buf);

  return -1;
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
