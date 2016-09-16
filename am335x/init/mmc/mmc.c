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
#include <fs.h>
#include <stdarg.h>
#include <string.h>

#include "sdmmcreg.h"
#include "sdhcreg.h"
#include "omap_mmc.h"

#define MMCHS0	0x48060000 /* sd */
#define MMC1	0x481D8000 /* mmc */
#define MMCHS2	0x47810000 /* not used */

#define MMC0_intr 64
#define MMC1_intr 28
#define MMC2_intr 29

enum { RESP_NO, RESP_LEN_48, RESP_LEN_136 };

struct mmc_command {
  uint32_t cmd;
  uint32_t arg;
  int resp_type;
  uint32_t resp[4];
  bool read, write;
  uint8_t *data;
  uint32_t data_len;
};

static struct mmchs *mmchs;

static void
handle_brr(void);

static void
handle_bwr(void);

int
__bitfield(uint32_t *src, int start, int len);

static bool
mmchsintrwait(uint32_t mask)
{
  uint32_t v;

  while (waitintr(mmchs->intr) != ERR) {
    printf("%s process intr\n", mmchs->name);
    while ((v = mmchs->regs->stat) != 0) {

      if (v & MMCHS_SD_STAT_BRR) {
	handle_brr();
	continue;
      } else if (v & MMCHS_SD_STAT_BWR) {
	handle_bwr();
	continue;
      }

      if (v & mask) {
	return true;
      } else if (v & MMCHS_SD_STAT_ERROR_MASK) {
	return false;
      }

      printf("%s unexpected interrupt 0b%b\n", mmchs->name, v);
    }
  }

  return false;
}

void
handle_bwr(void)
{
  size_t i;
  uint32_t v;

  if (mmchs->data == nil) {
    printf("%s->data == nil!\n", mmchs->name);
    return;
  }

  for (i = 0; i < mmchs->data_len; i += 4) {
    while (!(mmchs->regs->pstate & MMCHS_SD_PSTATE_BWE)) {
      printf("%s pstate bad!\n", mmchs->name);
    }

    *((uint8_t *) &v) = mmchs->data[i];
    *((uint8_t *) &v + 1) = mmchs->data[i+1];
    *((uint8_t *) &v + 2) = mmchs->data[i+2];
    *((uint8_t *) &v + 3) = mmchs->data[i+3];

    mmchs->regs->data = v;
  }

  mmchs->regs->stat = MMCHS_SD_STAT_BWR;
  mmchs->data = nil;
}

void
handle_brr(void)
{
  size_t i;
  uint32_t v;
  
  if (mmchs->data == nil) {
    printf("%s->data == nil!\n", mmchs->name);
    return;
  }

  for (i = 0; i < mmchs->data_len; i += 4) {
    while (!(mmchs->regs->pstate & MMCHS_SD_PSTATE_BRE)) {
      printf("%s pstate bad!\n", mmchs->name);
    }

    v = mmchs->regs->data;

    mmchs->data[i] = *((uint8_t *) &v);
    mmchs->data[i+1] = *((uint8_t *) &v + 1);
    mmchs->data[i+2] = *((uint8_t *) &v + 2);
    mmchs->data[i+3] = *((uint8_t *) &v + 3);
  }

  mmchs->regs->stat = MMCHS_SD_STAT_BRR;
  mmchs->data = nil;
}

static bool
mmchssendrawcmd(uint32_t cmd, uint32_t arg)
{
  if (mmchs->regs->stat != 0) {
    printf("%s stat in bad shape 0b%b\n",
	   mmchs->name, mmchs->regs->stat);
    return false;
  }

  mmchs->regs->arg = arg;
  mmchs->regs->cmd = cmd;

  if (!mmchsintrwait(MMCHS_SD_STAT_CC)) {
    mmchs->regs->stat = MMCHS_SD_STAT_CC;
    return false;
  }

  mmchs->regs->stat = MMCHS_SD_STAT_CC;

  return true;
}

static bool
mmchssendcmd(struct mmc_command *c)
{
  uint32_t cmd;
  bool r;

  cmd = MMCHS_SD_CMD_INDX_CMD(c->cmd);

  switch (c->resp_type) {
  case RESP_LEN_48:
    cmd |= MMCHS_SD_CMD_RSP_TYPE_48B;
    break;
  case RESP_LEN_136:
    cmd |= MMCHS_SD_CMD_RSP_TYPE_136B;
    break;
  case RESP_NO:
    cmd |= MMCHS_SD_CMD_RSP_TYPE_NO_RESP;
    break;
  default:
    return false;
  }

  if (c->read || c->write) {
    if (c->data == nil || c->data_len > 0xfff)
      return false;
    
    mmchs->data = c->data;
    mmchs->data_len = c->data_len;

    mmchs->regs->blk &= ~MMCHS_SD_BLK_BLEN;
    mmchs->regs->blk |= c->data_len;

    cmd |= MMCHS_SD_CMD_DP_DATA;
    cmd |= MMCHS_SD_CMD_MSBS_SINGLE;
  }
  
  if (c->read) {
    cmd |= MMCHS_SD_CMD_DDIR_READ;
    mmchs->regs->ie |= MMCHS_SD_IE_BRR_ENABLE;
  } else if (c->write) {
    cmd |= MMCHS_SD_CMD_DDIR_WRITE;
    mmchs->regs->ie |= MMCHS_SD_IE_BWR_ENABLE;
  }

  r = mmchssendrawcmd(cmd, c->arg);

  if (r && (c->read || c->write)) {
    if (!mmchsintrwait(MMCHS_SD_IE_TC_ENABLE)) {
      printf("%s wait for transfer complete failed!\n", mmchs->name);
      return false;
    }

    mmchs->regs->stat = MMCHS_SD_IE_TC_ENABLE;
  }
  
  if (c->read) {
    mmchs->regs->ie |= MMCHS_SD_IE_BRR_ENABLE;
  } else if (c->write) {
    mmchs->regs->ie |= MMCHS_SD_IE_BWR_ENABLE;
  }


  switch (c->resp_type) {
  case RESP_LEN_48:
    c->resp[0] = mmchs->regs->rsp10;
  case RESP_LEN_136:
    c->resp[1] = mmchs->regs->rsp32;
    c->resp[2] = mmchs->regs->rsp54;
    c->resp[3] = mmchs->regs->rsp76;
    break;
  case RESP_NO:
    break;
  }

  return r;
}

static bool
mmchssendappcmd(struct mmc_command *c)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_APP_CMD;
  cmd.arg = mmchs->rca;
  cmd.resp_type = RESP_LEN_48;
  cmd.read = false; cmd.write = false;
  cmd.data = nil;
  cmd.data_len = 0;

  if (!mmchssendcmd(&cmd)) {
    return false;
  }

  return mmchssendcmd(c);
}

static bool
readblock(uint32_t blk, uint8_t *buf)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_READ_BLOCK_SINGLE;
  cmd.arg = blk;
  cmd.resp_type = RESP_LEN_48;
  cmd.read = true;
  cmd.write = false;
  cmd.data = buf;
  cmd.data_len = 512;

  return mmchssendcmd(&cmd);
}

/*
  static bool
  writeblock(struct mmchs *mmchs, uint32_t blk, uint32_t *buf)
  {
  struct mmc_command cmd;

  cmd.cmd = MMC_WRITE_BLOCK_SINGLE;
  cmd.resp_type = RESP_LEN_48;
  cmd.read = false;
  cmd.write = true;
  cmd.arg = blk;
  cmd.data = buf;
  cmd.data_len = 512;

  return mmchssendcmd(&cmd);
  }
*/

static bool
mmchsinit(void)
{
  int i;

  mmchs->regs->sysconfig |= MMCHS_SD_SYSCONFIG_SOFTRESET;

  i = 100;
  do {
    sleep(1);
    i--;
  } while (i > 0 &&
	   !(mmchs->regs->sysstatus & MMCHS_SD_SYSSTATUS_RESETDONE));

  if (!i) {
    printf("%s: Failed to reset controller!\n", mmchs->name);
    return false;
  }

  /* Enable 1.8, 3.0 3.3V */
  mmchs->regs->capa |= MMCHS_SD_CAPA_VS18 |
    MMCHS_SD_CAPA_VS30 |
    MMCHS_SD_CAPA_VS33;

  mmchs->regs->sysconfig |= MMCHS_SD_SYSCONFIG_AUTOIDLE;
  mmchs->regs->sysconfig |= MMCHS_SD_SYSCONFIG_ENAWAKEUP;
  mmchs->regs->sysconfig &= ~MMCHS_SD_SYSCONFIG_SIDLEMODE;
  mmchs->regs->sysconfig |= MMCHS_SD_SYSCONFIG_SIDLEMODE_IDLE;
  mmchs->regs->sysconfig &= ~MMCHS_SD_SYSCONFIG_CLOCKACTIVITY;
  mmchs->regs->sysconfig |= MMCHS_SD_SYSCONFIG_CLOCKACTIVITY_OFF;
  mmchs->regs->sysconfig &= ~MMCHS_SD_SYSCONFIG_STANDBYMODE;
  mmchs->regs->sysconfig |= MMCHS_SD_SYSCONFIG_STANDBYMODE_WAKEUP_INTERNAL;

  /* wakup on sd interrupt for sdio */
  mmchs->regs->hctl |= MMCHS_SD_HCTL_IWE;

  /* change to 1 bit mode */ 
  mmchs->regs->con &= ~MMCHS_SD_CON_DW8;
  mmchs->regs->hctl &= ~MMCHS_SD_HCTL_DTW;

  /* set to 3.0V */
  mmchs->regs->hctl &= ~MMCHS_SD_HCTL_SDVS;
  mmchs->regs->hctl |= MMCHS_SD_HCTL_SDVS_VS30;

  /* Power on card */
  mmchs->regs->hctl |= MMCHS_SD_HCTL_SDBP;

  i = 100;
  do {
    sleep(1);
    i--;
  } while (i > 0 && !(mmchs->regs->hctl & MMCHS_SD_HCTL_SDBP));

  if (!i) {
    printf("%s: failed to power on card!\n", mmchs->name);
    return false;
  }

  /* enable internal clock and clock to card */
  mmchs->regs->sysctl |= MMCHS_SD_SYSCTL_ICE;

  /* set clock frequence to 400kHz */
  mmchs->regs->sysctl &= ~MMCHS_SD_SYSCTL_CLKD;
  mmchs->regs->sysctl |= 0xf0 << 6;

  /* enable clock */
  mmchs->regs->sysctl |= MMCHS_SD_SYSCTL_CEN;

  i = 100;
  do {
    sleep(1);
    i--;
  } while (i > 0 && !(mmchs->regs->sysctl & MMCHS_SD_SYSCTL_ICS));

  if (!i) {
    printf("%s: mmchs clock not stable\n", mmchs->name);
    return false;
  }

  /* Enable interrupts */
  mmchs->regs->ie |= MMCHS_SD_IE_CC_ENABLE;
  mmchs->regs->ie |= MMCHS_SD_IE_TC_ENABLE;
  mmchs->regs->ie |= MMCHS_SD_IE_ERROR_MASK;

  /* clear stat register */
  mmchs->regs->stat = 0xffffffff;

  /* send init signal */
  mmchs->regs->con |= MMCHS_SD_CON_INIT;
  mmchs->regs->cmd = 0;

  i = 25;
  do {
    sleep(1);
    i--;
  } while (i > 0 && !(mmchs->regs->stat & MMCHS_SD_STAT_CC));

  if (!i) {
    printf("%s: mmchs command 0 failed!\n", mmchs->name);
    return false;
  }

  /* clean stat */
  mmchs->regs->stat = MMCHS_SD_IE_CC_ENABLE;

  /* remove con init */
  mmchs->regs->con &= ~MMCHS_SD_CON_INIT;
 
  /* enable send interrupts to intc */
  mmchs->regs->ise = 0xffffffff;
  
  return true;
}

static bool
cardgotoidle(void)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_GO_IDLE_STATE;
  cmd.resp_type = RESP_NO;
  cmd.read = cmd.write = false;
  cmd.arg = 0;
  cmd.data = nil;

  return mmchssendcmd(&cmd);
}

static bool
cardidentification(void)
{
  struct mmc_command cmd;

  cmd.cmd = SD_SEND_IF_COND;
  cmd.resp_type = RESP_LEN_48;
  cmd.read = cmd.write = false;
  cmd.data = nil;
  cmd.arg = MMCHS_SD_ARG_CMD8_VHS | MMCHS_SD_ARG_CMD8_CHECK_PATTERN;

  if (!mmchssendcmd(&cmd)) {
    return false;
  }

  if (cmd.resp[0] !=
      (MMCHS_SD_ARG_CMD8_VHS | MMCHS_SD_ARG_CMD8_CHECK_PATTERN)) {
    printf("%s check pattern check failed 0x%h\n",
	   mmchs->name, cmd.resp);
    return false;
  }

  return true;
}

static bool
cardqueryvolttype(void)
{
  struct mmc_command cmd;

  cmd.cmd = SD_APP_OP_COND;
  cmd.resp_type = RESP_LEN_48;
  cmd.read = cmd.write = false;
  cmd.data = nil;
  cmd.arg =
    MMC_OCR_3_3V_3_4V | MMC_OCR_3_2V_3_3V | MMC_OCR_3_1V_3_2V |
    MMC_OCR_3_0V_3_1V | MMC_OCR_2_9V_3_0V | MMC_OCR_2_8V_2_9V |
    MMC_OCR_2_7V_2_8V | MMC_OCR_HCS;

  if (!mmchssendappcmd(&cmd)) {
    /* mmc cards should fall into here. */
    printf("%s is a mmc card\n", mmchs->name);
    mmchs->regs->stat = 0xffffffff; /* clear stat */
    mmchs->regs->sysctl |= MMCHS_SD_SYSCTL_SRC;
    while (mmchs->regs->sysctl & MMCHS_SD_SYSCTL_SRC)
      sleep(0);

    cmd.cmd = MMC_SEND_OP_COND;
    while (!(cmd.resp[0] & MMC_OCR_MEM_READY)) {
      if (!mmchssendcmd(&cmd)) {
	return false;
      }
    }
   
  } else {
    /* SD cards here. */
    printf("%s is a sd card\n", mmchs->name);
    while (!(cmd.resp[0] & MMC_OCR_MEM_READY)) {
      if (!mmchssendappcmd(&cmd)) {
	return false;
      }
    }

  }

  return true;
}

static bool
cardidentify(void)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_ALL_SEND_CID;
  cmd.resp_type = RESP_LEN_136;
  cmd.read = cmd.write = false;
  cmd.data = nil;
  cmd.arg = 0;

  if (!mmchssendcmd(&cmd)) {
    return false;
  }

  cmd.cmd = MMC_SET_RELATIVE_ADDR;
  cmd.resp_type = RESP_LEN_48;

  if (!mmchssendcmd(&cmd)) {
    return false;
  }

  mmchs->rca = SD_R6_RCA(cmd.resp);

  return true;
}

static bool
cardselect(void)
{
  struct mmc_command cmd;
  
  cmd.cmd = MMC_SELECT_CARD;
  cmd.resp_type = RESP_LEN_48;
  cmd.read = cmd.write = false;
  cmd.data = nil;
  cmd.arg = MMC_ARG_RCA(mmchs->rca);

  return mmchssendcmd(&cmd);
}

static bool
cardcsd(void)
{
  struct mmc_command cmd;

  cmd.cmd = MMC_SEND_CSD;
  cmd.resp_type = RESP_LEN_136;
  cmd.read = cmd.write = false;
  cmd.data = nil;
  cmd.arg = MMC_ARG_RCA(mmchs->rca);

  if (!mmchssendcmd(&cmd)) {
    return false;
  }

  mmchs->csd[0] = cmd.resp[0];
  mmchs->csd[1] = cmd.resp[1];
  mmchs->csd[2] = cmd.resp[2];
  mmchs->csd[3] = cmd.resp[3];

  return true;
}

static bool
cardinit(void)
{
  if (!cardgotoidle()) {
    printf("%s failed to goto idle state!\n", mmchs->name);
    return false;
  }

  if (cardidentification()) {
    printf("%s is sd 2.0\n", mmchs->name);
  } else {
    printf("%s possibly mmc card!\n", mmchs->name);
    mmchs->regs->stat = 0xffffffff; /* clear stat */
  }

  mmchs->regs->sysctl |= MMCHS_SD_SYSCTL_SRC;
  printf("%s wait for reset to start\n", mmchs->name);
  while (!(mmchs->regs->sysctl & MMCHS_SD_SYSCTL_SRC));

  printf("%s wait for reset to end\n", mmchs->name);
  while (mmchs->regs->sysctl & MMCHS_SD_SYSCTL_SRC)
    sleep(1);
  printf("%s ready to move on\n", mmchs->name);

  if (!cardqueryvolttype()) {
    printf("%s failed to query voltage and type!\n", mmchs->name);
    return false;
  }

  if (!cardidentify()) {
    printf("%s identify card failed!\n", mmchs->name);
    return false;
  }

  if (!cardcsd()) {
    printf("%s get card csd failed!\n", mmchs->name);
    return false;
  }

  if (!cardselect()) {
    printf("%s select card failed!\n", mmchs->name);
    return false;
  }

  if (SD_CSD_READ_BL_LEN(mmchs->csd) != 0x09) {
    printf("%s block length is not 512!\n", mmchs->name);
    return false;
  }

  return true;
}

static void
bopen(struct request *req, struct response *resp)
{
  printf("%s opened\n");
  resp->ret = ENOIMPL;
}

static void
bclose(struct request *req, struct response *resp)
{
  printf("%s close\n");
  resp->ret = ENOIMPL;
}

static void
bread(struct request *req, struct response *resp)
{
  printf("%s read\n");
  resp->ret = ENOIMPL;

  readblock(0, nil);
}

static void
bwrite(struct request *req, struct response *resp)
{
  printf("%s write\n");
  resp->ret = ENOIMPL;
}

static void
handlereq(struct request *req, struct response *resp)
{
  switch (req->type) {
  case REQ_open:
    bopen(req, resp);
    break;
  case REQ_close:
    bclose(req, resp);
    break;
  case REQ_read:
    bread(req, resp);
    break;
  case REQ_write:
    bwrite(req, resp);
    break;
  default:
    resp->ret = ENOIMPL;
    break;
  }
}

static int
mmchsmountloop(int in, int out)
{
  uint8_t buf[1024];
  size_t reqsize, respsize;
  struct request req;
  struct response resp;

  reqsize = sizeof(req.rid)
    + sizeof(req.type)
    + sizeof(req.fid)
    + sizeof(req.lbuf);

  respsize = sizeof(resp.rid)
    + sizeof(resp.ret)
    + sizeof(resp.lbuf);
  
  req.buf = buf;
	
  while (true) {
    if (read(in, &req, reqsize) != reqsize)
      break;

    resp.rid = req.rid;
    resp.lbuf = 0;
    resp.ret = OK;
		
    if (req.lbuf > 1024) {
      resp.ret = ENOMEM;
    } else if (req.lbuf > 0) {
      if (read(in, req.buf, req.lbuf) != req.lbuf) {
	break;
      }
    }
		
    if (resp.ret == OK) {
      handlereq(&req, &resp);
    }
		
    if (write(out, &resp, respsize) != respsize)
      break;
		
    if (resp.lbuf > 0) {
      if (write(out, resp.buf, resp.lbuf) != resp.lbuf) {
	break;
      }

      free(resp.buf);
    }
  }

  return 0; 
}

static int
mmchsproc(char *name, void *addr, int intr)
{
  size_t size = 4 * 1024;
  char filename[256];
  int fd, p1[2], p2[2];

  printf("%s running in proc %i\n", name, getpid());
  
  mmchs = malloc(sizeof(struct mmchs));
  if (!mmchs) {
    printf("%s malloc failed!\n", name);
    return -1;
  }

  mmchs->intr = intr;
  mmchs->name = name;
  mmchs->regs = (struct mmchs_regs *) getmem(MEM_io, addr, &size);
  if (!mmchs->regs) {
    printf("%s failed to map register space!\n", mmchs->name);
    return -2;
  }

  if (!mmchsinit()) {
    printf("%s failed to init host!\n", mmchs->name);
    return -2;
  }

  if (!cardinit()) {
    printf("%s failed to init card!\n", mmchs->name);
    return -3;
  }

  switch (SD_CSD_CSDVER(mmchs->csd)) {
  case 0:
    mmchs->size = SD_CSD_CAPACITY(mmchs->csd) * 512;
    break;
  case 2:
    mmchs->size = SD_CSD_V2_CAPACITY(mmchs->csd) * 512;
    break;
  case 3:
    mmchs->size = MMC_CSD_CAPACITY(mmchs->csd) * 512;
    break;
  default:
    printf("%s has unknown csd version %i\n", mmchs->name,
	   SD_CSD_CSDVER(mmchs->csd));
  }

  printf("%s is of size %i Mb\n", mmchs->name,
	 mmchs->size / 1024 / 1024);
  
  if (pipe(p1) == ERR) {
    return -4;
  } else if (pipe(p2) == ERR) {
    return -5;
  }

  snprintf(filename, sizeof(filename), "/dev/%s", mmchs->name);

  printf("%s binding to %s\n", mmchs->name, filename);
  
  fd = open(filename, O_WRONLY|O_CREATE, ATTR_wr|ATTR_rd);
  if (fd < 0) {
    printf("%s failed to create %s.\n", mmchs->name, filename);
    return -6;
  }
  close(fd);
  
  if (bind(p1[1], p2[0], filename) == ERR) {
    return -7;
  }

  close(p1[1]);
  close(p2[0]);

  return mmchsmountloop(p1[0], p2[1]);
}

int
mmc(void)
{
  int p;
	
  p = fork(FORK_sngroup);
  if (p == 0) {
    return mmchsproc("mmc0", (void *) MMCHS0, MMC0_intr);
  } else if (p < 0) {
    printf("mmchs failed to fork for mmc0\n");
  }

  p = fork(FORK_sngroup);
  if (p == 0) {
    return mmchsproc("mmc1", (void *) MMC1, MMC1_intr);
  } else if (p < 0) {
    printf("mmchs failed to fork for mmc1\n");
  }

  return 0;
}

