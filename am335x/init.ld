ENTRY(main)

SECTIONS
{
	. = 0;

	.text :
	{
		*(.text.startup)
		*(.text)
	}

	.data : 
	{
		*(.data.*)
		*(.data)
	}
	
	.rodata :
	{
		*(.rodata.*)
		*(.rodata)
	}
	
	.bss :
	{
		*(.bss.*)
		*(.bss)
	}
}
