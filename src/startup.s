.section .text
.global _start
_start:
	cpsid if @ disable interupts
	ldr sp, =_svc_stack_top
	
	@ Print boot message
	ldr r0, =boot_msg
	bl uart_puts
	
	bl vector_table_init
	
	@ disable cache 
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #(1 << 12) 		@ set I flag to 0
	bic r0, r0, #(1 << 2)		@ set C flag to 0
	mcr p15, 0, r0, c1, c0, 0

@	cps #17
@	ldr sp, =_fiq_stack_top
	
@	cps #18
@	ldr sp, =_irq_stack_top

@	cps #23
@	ldr sp, =_abort_stack_top
	
	cps #19
	cpsie i
		
	bl main
	
.section .rodata
boot_msg:
	.asciz "\nBooting...\n\n"
	