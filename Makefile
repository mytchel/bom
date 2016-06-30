TARGET ?= am335x

$(TARGET):
	@$(MAKE) -C $(TARGET)

clean:
	@$(MAKE) -C $(TARGET) clean

.PHONY: $(TARGET) clean
