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
		*(.data)
		*(.rodata)
	}
}
