CROSS_COMPILE ?= arm-none-eabi-
CC := $(CROSS_COMPILE)gcc
LD := $(CROSS_COMPILE)ld

CFLAGS =-Wall -Werror -O2 \
	-nostdlib -nodefaultlibs -ffreestanding \
	-Iinclude

LDFLAGS = -T linker.ld 

all: out.umg out.list

SRC=src/startup.s src/*.c

out.elf: ${SRC}
	$(CC) ${CFLAGS} -o out.elf ${SRC} $(LDFLAGS) 
	
out.bin: out.elf
	$(CROSS_COMPILE)objcopy -Obinary out.elf out.bin

out.list: out.elf
	$(CROSS_COMPILE)objdump -S out.elf > out.list


loadaddr=0x82000000

out.umg: out.bin mkuboot/mkuboot
	./mkuboot/mkuboot -a arm -e ${loadaddr} -l ${loadaddr} -o linux out.bin out.umg

clean:
	rm -f *.o *.bin *.elf *.list *.umg
