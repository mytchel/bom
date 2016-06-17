
SECTIONS
{
	.text 0x82000000 :
	{
		_kernel_bin_start = .;
		startup.o(.text)
		*(.text)
	}
	
	.data : ALIGN(4096)
	{
		*(.data)
	}
	
	.rodata : ALIGN(4096)
	{
		*(.rodata)
	}
	
	.bss : ALIGN(4096)
	{
		_kernel_bss_start = .;
		*(.bss)
	}

	. = ALIGN(4096);
	_kernel_bss_end = .;

	_kernel_heap_start = .;
	. = . + 0x10000000;
	. = ALIGN(4096);
	_kernel_heap_end = .;
		
	. = . + 0x2000;
	_svc_stack_top = .;
	. = . + 0x2000;
	_fiq_stack_top = .;
	. = . + 0x2000;
	_irq_stack_top = .;
	. = . + 0x2000;
	_abort_stack_top = .;
	
	_kernel_bin_end = .;
}
