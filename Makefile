#
#  Copyright (c) 2016 Mytchel Hammond <mytchel@openmailbox.org>
#  
#  Permission is hereby granted, free of charge, to any person
#  obtaining a copy of this software and associated documentation
#  files (the "Software"), to deal in the Software without
#  restriction, including without limitation the rights to use,
#  copy, modify, merge, publish, distribute, sublicense, and/or
#  sell copies of the Software, and to permit persons to whom the
#  Software is furnished to do so, subject to the following
#  conditions:
#  
#  The above copyright notice and this permission notice shall be
#  included in all copies or substantial kernions of the Software.
#  
#  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
#  OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
#  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
#  OTHER DEALINGS IN THE SOFTWARE
#

TARGETS = am335x

BASE = $(.CURDIR)

-include config.mk

SUBDIR = 
SUBDIR += lib
SUBDIR += bin

TARGET ?= none

.PHONY: all
all: $(TARGET) $(SUBDIR)

.PHONY: none
none:
	@echo Select a target first with
	@echo $(MAKE) config_[target]
	@echo where [target] is one of: $(TARGETS)


.for t in $(TARGETS)
.PHONY: config_$(t)
config_$(t):
	cp $(t)/config.mk config.mk
.endfor

.PHONY: $(TARGET)
$(TARGET):
	$(MAKE) -C $(TARGET) BASE="$(BASE)"


.for s in $(SUBDIR)
.PHONY: $(s)
$(s):
	$(MAKE) -C $(s) BASE="$(BASE)"

.PHONY: $(s)/clean
$(s)/clean:
	$(MAKE) -C $(s) clean BASE="$(BASE)"

.endfor

.PHONY: clean
clean: $(SUBDIR:%=%/clean)
	$(MAKE) -C $(TARGET) clean BASE="$(BASE)"
