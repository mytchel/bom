/*
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

#ifndef _MMC_H_
#define _MMC_H_

typedef enum { MMC_EMMC, MMC_SD } mmc_t;

extern volatile struct mmchs_regs *regs;
extern char *name;
extern int intr;
extern uint32_t csd[4];
extern uint32_t rca;
extern uint32_t nblk;

bool
mmchssendcmd(uint32_t cmd, uint32_t arg);

bool
mmchssendappcmd(uint32_t cmd, uint32_t arg);

bool
mmchssoftreset(void);

void
mmcreset(uint32_t line);

bool
cardgotoidle(void);

bool
cardidentification(void);

bool
cardqueryvolttype(void);

bool
cardidentify(void);

bool
cardselect(void);

bool
cardcsd(void);

bool
mmchsreaddata(uint32_t *buf, size_t l);

bool
mmchswritedata(uint32_t *buf, size_t l);

bool
mmcinit(void);

bool
sdinit(void);

bool
readblock(uint32_t blk, uint8_t *buf);

bool
writeblock(uint32_t blk, uint8_t *buf);

int
__bitfield(uint32_t *src, int start, int len);

#endif
