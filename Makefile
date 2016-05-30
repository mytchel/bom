TARGET?=am335x

-include $(TARGET)/Makefile

CFLAGS += -Wall -Werror -Iinclude

LDFLAGS += 

SRC= $(TARGET)/*.s $(TARGET)/*.c kern/*.c

out/$(TARGET).elf: ${SRC} include/*.h
	$(CC) ${CFLAGS} -o out/$(TARGET).elf ${SRC} $(LDFLAGS) 
	
out/$(TARGET).bin: out/$(TARGET).elf
	$(OBJCOPY) -Obinary out/$(TARGET).elf out/$(TARGET).bin

out/$(TARGET).list: out/$(TARGET).elf
	$(OBJDUMP) -SD out/$(TARGET).elf > out/$(TARGET).list

clean:
	rm -f out/*
