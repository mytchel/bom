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

#ifndef _AM335X_GPIO_H_
#define _AM335X_GPIO_H_

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

#endif
