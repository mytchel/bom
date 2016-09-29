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
	
	_kernel_bss_start = .;
	.bss : 
	{
		*(.bss)
		*(.bss.*)
	}
	_kernel_bss_end = .;

	. = ALIGN(4096);

	. = . + 0x4000;
	_ex_stack_top = .;
	
	. = ALIGN(4096);
	_heap_start = .;
	. = . + 0x80000;
	. = ALIGN(4096);
	_heap_end = .;

	. = ALIGN(4096);
	_kernel_end = .;
	
	. = 0xa0000000;
	_ram_end = .;
}
