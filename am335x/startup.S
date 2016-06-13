#include "p_modes.h"

.section .text
.global _start
_start:
	cpsid if @ disable interupts
	ldr sp, =_svc_stack_top
	
	@ Print boot message
	ldr r0, =boot_msg
	bl puts
	
	bl vector_table_init
	
	@ disable cache 
	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #(1 << 12) 		@ set I flag to 0
	bic r0, r0, #(1 << 2)		@ set C flag to 0
	mcr p15, 0, r0, c1, c0, 0
	
	cps #MODE_FIQ
	ldr sp, =_fiq_stack_top
	cps #MODE_IRQ
	ldr sp, =_irq_stack_top
	cps #MODE_ABORT
	ldr sp, =_abort_stack_top
	cps #MODE_SVC

	bl memory_init
	bl intc_init
	bl systick_init
	
	bl scheduler_init
	bl schedule
	cpsie i @ enable interupts
	
	@ starts the first process that was returned by schedule
	b start_proc
	 
.section .rodata
boot_msg:
	.asciz "\nBooting...\n\n"
	