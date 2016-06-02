
TARGET ?= am335x

$(TARGET):
	@$(MAKE) -C $(TARGET) all

clean:
	@$(MAKE) -C $(TARGET) clean

.PHONY: $(TARGET) clean
