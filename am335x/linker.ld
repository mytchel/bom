ENTRY(_start)

SECTIONS
{
	. = 0x80000000;
	_ram_start = .;
	. = 0x82000000;

	_kernel_start = .;
	.text :
	{
		*(.text)
	}
	
	.data : 
	{
		*(.rodata)
		*(.data)
	}
	
	_kernel_bss_start = .;
	.bss : 
	{
		*(.bss)
	}
	_kernel_bss_end = .;

	. = . + 0x1000;
	_svc_stack_top = .;
	. = . + 0x1000;
	_irq_stack_top = .;
	. = . + 0x1000;
	_abort_stack_top = .;
	. = . + 0x1000;
	_undef_stack_top = .;
	
	_heap_start = .;
	. = . + 0x40000;
	. = ALIGN(4096);
	_heap_end = .;

	_kernel_end = .;
	
	. = 0xa0000000;
	_ram_end = .;
}
