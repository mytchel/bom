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
#include <string.h>
#include <fs.h>
#include <fssrv.h>

#define GPIO_LEN   	0x1000

#define GPIO0           0x44e07000
#define GPIO1           0x4804c000
#define GPIO2           0x481ac000
#define GPIO3           0x481ae000

#define GPIO0_intra     96
#define GPIO0_intrb     97
#define GPIO1_intra     98
#define GPIO1_intrb     99
#define GPIO2_intra     32
#define GPIO2_intrb     33
#define GPIO3_intra     62
#define GPIO3_intrb     63

struct gpio_regs {
  uint32_t rev;
  uint32_t pad1[3];
  uint32_t sysconfig;
  uint32_t pad2[3];
  uint32_t eoi;
  uint32_t irqstatus_raw_0;
  uint32_t irqstatus_raw_1;
  uint32_t irqstatus_0;
  uint32_t irqstatus_1;
  uint32_t irqstatus_set_0;
  uint32_t irqstatus_set_1;
  uint32_t irqstatus_clr_0;
  uint32_t irqstatus_clr_1;
  uint32_t irqwaken_0;
  uint32_t irqwaken_1;
  uint32_t pad3[50];
  uint32_t sysstatus;
  uint32_t pad4[6];
  uint32_t ctrl;
  uint32_t oe;
  uint32_t datain;
  uint32_t dataout;
  uint32_t leveldetect0;
  uint32_t leveldetect1;
  uint32_t risingdetect;
  uint32_t fallingdetect;
  uint32_t debounceenable;
  uint32_t debouncingtime;
  uint32_t pad5[14];
  uint32_t cleardataout;
  uint32_t setdataout;
};

static volatile struct gpio_regs *regs = nil;

static struct stat rootstat = {
  ATTR_wr|ATTR_rd,
  0,
  GPIO_LEN,
};

static void
bstat(struct request_stat *req, struct response_stat *resp)
{
  memmove(&resp->body.stat, &rootstat, sizeof(struct stat));
  resp->head.ret = OK;
}

static void
bopen(struct request_open *req, struct response_open *resp)
{
  resp->head.ret = OK;
}

static void
bclose(struct request_close *req, struct response_close *resp)
{
  resp->head.ret = OK;
}

/* Read format is 
   0 [in|out] [low|high]
   1 [in|out] [low|high]
   ...
 */

static void
bread(struct request_read *req, struct response_read *resp)
{
  uint32_t oe, di;
  char line[32];
  size_t len, l;
  int p;

  p = req->body.offset / 6;

  oe = regs->oe;
  di = regs->datain;

  len = 0;
  while (len < req->body.len && p < 32) {
    l = snprintf(line, sizeof(line),
		 "%i %s %s\n",
		 p,
		 ((oe >> p) & 1) ? "in" : "out",
		 ((di >> p) & 1) ? "high" : "low");

    if (len + l > req->body.len) {
      memmove(resp->body.data + len, line, req->body.len - len);
      len = req->body.len;
    } else {
      memmove(resp->body.data + len, line, l);
      len += l;
    }

    p++;
  }
  
  resp->body.len = len;
  resp->head.ret = OK;
}

/* Write format is 
       n [in|out]
       n [low|high]
       ...
   Where n is a pin number between 0 and 31 inclusive.
 */

static void
bwrite(struct request_write *req, struct response_write *resp)
{
  char s[32], change[32];
  uint32_t dc, ds, oe;
  size_t len, l;
  int p;

  dc = 0;
  ds = 0;
  oe = regs->oe;
  
  len = 0;
  while (len < req->body.len) {
    for (l = 0; len + l < req->body.len && l < sizeof(s); l++) {

      s[l] = req->body.data[len + l];
      if (s[l] == '\n') {
	len++;
	break;
      } else if (s[l] == 0) {
	break;
      }
    }

    if (l == 0) {
      break;
    }
    
    s[l] = 0;
    if (sscanf(s, "%i %s", &p, change) != 2) {
      break;
    }

    if (strncmp(change, "in", 3)) {
      oe |= (1 << p);
    } else if (strncmp(change, "out", 4)) {
      oe &= ~(1 << p);
    } else if (strncmp(change, "low", 4)) {
      dc |= (1 << p);
    } else if (strncmp(change, "high", 5)) {
      ds |= (1 << p);
    } else {
      break;
    }
    
    len += l;
  }

  regs->oe = oe;
  regs->setdataout = ds;
  regs->cleardataout = dc;

  resp->body.len = len;
  resp->head.ret = OK;
}
 
static struct fsmount fsmount = {
  .stat = &bstat,
  .open = &bopen,
  .close = &bclose,
  .read = &bread,
  .write = &bwrite,
};

int
gpiomount(char *path, uint32_t addr)
{
  int f, fd, p1[2], p2[2];
  size_t size = GPIO_LEN;

  if (pipe(p1) == ERR) {
    return -1;
  } else if (pipe(p2) == ERR) {
    return -1;
  }

  fd = open(path, O_WRONLY|O_CREATE, rootstat.attr);
  if (fd < 0) {
    return -2;
  }

  if (mount(p1[1], p2[0], path, rootstat.attr) == ERR) {
    return -3;
  }

  close(fd);
  close(p1[1]);
  close(p2[0]);

  f = fork(FORK_sngroup);
  if (f > 0) {
    close(p1[0]);
    close(p2[1]);
    close(fd);
    return f;
  }

  regs = (struct gpio_regs *)
    mmap(MEM_io, (void *) addr, &size);

  if (regs == nil) {
    exit(-4);
  }

  fsmount.databuf = malloc(512);
  fsmount.buflen = 512;
  
  f = fsmountloop(p1[0], p2[1], &fsmount);

  printf("gpio mount on %s exiting with %i\n", path, f);

  free(fsmount.databuf);
  
  exit(f);
}

int
initgpios(void)
{
  gpiomount("/dev/gpio0", GPIO0);
  gpiomount("/dev/gpio1", GPIO1);
  gpiomount("/dev/gpio2", GPIO2);
  gpiomount("/dev/gpio3", GPIO3);

  return OK;
}
