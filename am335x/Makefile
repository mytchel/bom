dir=am335x

CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld
OBJCOPY := $(CROSS_COMPILE)objcopy
OBJDUMP := $(CROSS_COMPILE)objdump

CFLAGS = -mcpu=cortex-a8 \
	-nostdlib -nostdinc -nodefaultlibs

LDFLAGS = -T $(dir)/linker.ld \
	-nostdlib -nostdinc -nodefaultlibs -ffreestanding

all: out/am335x.umg out/am335x.list out/uenv.txt

-include $(dir)/mkuboot/Makefile

loadaddr=0x82000000

out/am335x.umg: out/am335x.bin $(dir)/mkuboot/mkuboot
	./out/mkuboot -a arm -e ${loadaddr} -l ${loadaddr} -o linux out/am335x.bin out/am335x.umg

out/uenv.txt: $(dir)/uenv.txt
	cp $(dir)/uenv.txt out/uenv.txt
	