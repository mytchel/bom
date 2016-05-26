CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

CFLAGS =-Wall -Werror -O2 \
	-nostdlib -nostartfiles -ffreestanding

LDFLAGS = -T linker.ld 

all: out.umg out.list

startup.o: startup.s
	$(CC) $(CFLAGS) -o startup.o -c startup.s 

out.elf: startup.o
	$(LD) $(LDFLAGS) -o out.elf startup.o
	
out.bin: out.elf
	$(CROSS_COMPILE)objcopy -Obinary out.elf out.bin

out.list: out.elf
	$(CROSS_COMPILE)objdump -S out.elf > out.list


loadaddr=0x82000000

out.umg: out.bin mkuboot/mkuboot
	./mkuboot/mkuboot -a arm -e ${loadaddr} -l ${loadaddr} -o linux out.bin out.umg

clean:
	rm -f *.o *.bin *.elf *.list *.umg
