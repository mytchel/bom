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
	
	. = ALIGN(4096);

	.rodata : 
	{
		*(.rodata)
		*(.rodata.*)
	}

	. = ALIGN(4096);

	.data :
	{
		*(.data)
		*(.data.*)
	}
	
	. = ALIGN(4096);

	_kernel_bss_start = .;
	.bss : 
	{
		*(.bss)
		*(.bss.*)
	}
	_kernel_bss_end = .;

	. = ALIGN(4096);

	. = . + 0x100;
	_ex_stack_top = .;
	
	_heap_start = .;
	. = . + 0x10000;
	. = ALIGN(4096);
	_heap_end = .;

	. = ALIGN(4096);
	_kernel_end = .;
	
	. = 0xa0000000;
	_ram_end = .;
}
