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
		*(.data)
	}
	
	.rodata :
	{
		*(.rodata)
	}
	
	. = ALIGN(4096);
		
	_kernel_bss_start = .;
	.bss : 
	{
		*(.bss)
	}
	_kernel_bss_end = .;

	_heap_start = .;
	. = . + 0x01000000;
	. = ALIGN(4096);
	_heap_end = .;
	
	. = . + 0x2000;
	_svc_stack_top = .;
	. = . + 0x2000;
	_irq_stack_top = .;
	. = . + 0x2000;
	_abort_stack_top = .;
	. = . + 0x2000;
	_undef_stack_top = .;
	
	. = ALIGN(4096);
	_kernel_end = .;
	
	. = 0xa0000000;
	_ram_end = .;
}
