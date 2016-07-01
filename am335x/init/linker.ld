ENTRY(_start)

SECTIONS
{
	. = 0;

	.text :
	{
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
	}
	
	. = ALIGN(4096);
	
	.data :
	{
		*(.data)
		*(.data.*)
		*(.bss)
		*(.bss.*)
	}
}
