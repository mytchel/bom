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
}
