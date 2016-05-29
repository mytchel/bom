OUTPUT_ARCH(arm)
ENTRY(_start)
SECTIONS
{
	_ram_start = 0x80000000;
	_ram_end = 0xbfffffff;
	
	.text 0x82000000 :
	{
		_kernel_bin_start = .;
		*(.text)
	}
	
	.data : ALIGN(4096)
	{
		*(.data)
	}
	
	.rodata : ALIGN(4096)
	{
		*(.rodata)
	}
	
	.bss : ALIGN(4096)
	{
		_kernel_bss_start = .;
		*(.bss)
	}
	
	. = ALIGN(4096);
	_kernel_bss_end = .;
	
	. = . + 0x2000;
	_svc_stack_top = .;
	. = . + 0x2000;
	_fiq_stack_top = .;
	. = . + 0x2000;
	_irq_stack_top = .;
	. = . + 0x2000;
	_abort_stack_top = .;
	
	_kernel_bin_end = .;
}
