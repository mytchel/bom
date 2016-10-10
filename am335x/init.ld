ENTRY(_start)

SECTIONS
{
	. = 0;

	.text :
	{
		*(.text)
		*(.text.*)
	}
	
	. = ALIGN(4096);
	
	.data :
	{
		*(.rodata)
		*(.rodata.*)
		*(.data)
		*(.data.*)
		*(.bss)
		*(.bss.*)
	}
}
