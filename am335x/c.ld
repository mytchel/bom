ENTRY(_start)

SECTIONS
{
	.text :
	{
		libc.a(.text)
		*(.text)
		*(.text.*)
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

	.bss :
	{
		*(.bss)
		*(.bss.*)
	}

	. = ALIGN(4096);
}
