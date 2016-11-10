ENTRY(_start)

SECTIONS
{
	. = 0;

	.text ALIGN(4096) :
	{
		libc.a(.text)
		*(.text)
		*(.text.*)
		. = ALIGN(4096);
		*(.rodata)
		*(.rodata.*)
	}
	
	.data ALIGN(4096) :
	{
		*(.data)
		*(.data.*)
		. = ALIGN(4096);
		*(.bss)
		*(.bss.*)
	}
}
