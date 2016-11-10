SECTIONS
{
	. = 0x80000000;
	_ram_start = .;

	.text ALIGN(4096) :
	{
		_kernel_start = .;
		*(.text)
	}
	
	.rodata ALIGN(4096) : 
	{
		*(.rodata)
		*(.rodata.*)
	}

	.data ALIGN(4096) : 
	{
		*(.data)
		*(.data.*)
	}
	
	.bss ALIGN(4096) : 
	{
		_kernel_bss_start = .;
		*(.bss)
		*(.bss.*)
		_kernel_bss_end = .;
	}

	. = ALIGN(4096);

	. = . + 0x100;
	_ex_stack_top = .;


	.heap ALIGN(4096) :
	{
	      _heap_start = .;
	      . = . + 0x10000;
	      _heap_end = .;
	 }	

	_kernel_end = .;

	. = 0xa0000000;
	_ram_end = .;
}
