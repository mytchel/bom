ENTRY(_start)

SECTIONS
{
	. = 0;

	.text :
	{
		*(.text)
		*(.text.*)
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
}
