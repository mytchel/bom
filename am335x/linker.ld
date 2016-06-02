
MEMORY
{
	ram (rwx) : ORIGIN = 0x82000000, LENGTH = 0x10000
}

SECTIONS
{
	_ram_start = 0x80000000;
	_ram_end = 0xbfffffff;
	
	.text 0x82000000 :
	{
		*(.text)
		*(.text.*)
		*(.rodata)
		*(.rodata.*)
	} > ram
	
	.data :
	{
		*(.data)
		*(.data.*)
	} > ram
	
	.bss :
	{
		_kernel_bss_start = .;
		*(.bss)
		*(.bss.*)
	} > ram
	
	. = ALIGN(4096);
	_kernel_bss_end = .;

	. = ORIGIN(ram) + LENGTH(ram);
	. = . + 0x2000;
	_svc_stack_top = .;
	. = . + 0x2000;
	_fiq_stack_top = .;
	. = . + 0x2000;
	_irq_stack_top = .;
	. = . + 0x2000;
	_abort_stack_top = .;
}
