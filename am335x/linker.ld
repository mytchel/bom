ENTRY(_start)

SECTIONS
{
	. = 0x80000000;
	_ram_start = .;
	. = 0x82000000;
	_kernel_bin_start = .;
	
	. = ALIGN(4096);

	.text :
	{
		*(.text)
	}
	
	. = ALIGN(4096);
	
	.data : 
	{
		*(.data)
	}
		
	. = ALIGN(4096);
	
	.rodata :
	{
		*(.rodata)
	}
	
	. = ALIGN(4096);
	
	.bss : 
	{
		_kernel_bss_start = .;
		*(.bss)
	}

	. = ALIGN(4096);
	
	_kernel_bss_end = .;

	_kernel_heap_start = .;
	. = . + 0x01000000;
	_kernel_heap_end = .;

	. = ALIGN(4096);
	
	. = . + 0x2000;
	_svc_stack_top = .;
	. = . + 0x2000;
	_fiq_stack_top = .;
	. = . + 0x2000;
	_irq_stack_top = .;
	. = . + 0x2000;
	_abort_stack_top = .;
	
	_kernel_bin_end = .;
	. = 0xa000000;
	_ram_end = .;
}
