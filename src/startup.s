.section .text

.arm
.global _start
_start:
	@ Print boot message
	ldr r0, =boot_msg
	bl uart_puts

	bl vector_table_init
	
	bl main
	
.section .rodata
boot_msg:
	.asciz "\nBooting...\n\n"
	