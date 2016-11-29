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

#include <libc.h>
#include <fs.h>
#include <string.h>
#include <mem.h>
#include <block.h>

#include "sdmmcreg.h"
#include "sdhcreg.h"
#include "omap_mmc.h"
#include "mmchs.h"

static uint8_t ext_csd[512];

static bool
cardextcsd(void)
{
  uint32_t cmd;

  cmd = MMCHS_SD_CMD_INDEX_CMD(MMC_SEND_EXT_CSD)
    | MMCHS_SD_CMD_RSP_TYPE_48B
    | MMCHS_SD_CMD_DP_DATA
    | MMCHS_SD_CMD_DDIR_READ;

  if (!mmchssendcmd(cmd, 0)) {
    return false;
  }

  if (!mmchsreaddata((uint32_t *) ext_csd, 512)) {
    return false;
  }

  return true;
}

bool
mmcinit(void)
{
  uint32_t ecsdcount;
  
  if (!cardextcsd()) {
    printf("%s failed to read extended csd\n", name);
    return false;
  }

  ecsdcount = intcopylittle32(&ext_csd[212]);
  if (ecsdcount > 0) {
    nblk = ecsdcount;
  } else {
    nblk = MMC_CSD_CAPACITY(csd);
  }

  return true;
}
