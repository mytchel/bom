bintoarray_src = bintoarray/bintoarray.c

bintoarray/bintoarray: $(bintoarray_src)
	@echo HOSTCC $@
	@$(HOSTCC) $(HOST_CFLAGS) -o $@ $(bintoarray_src) $(HOST_LDFLAGS)

CLEAN += bintoarray/bintoarray
