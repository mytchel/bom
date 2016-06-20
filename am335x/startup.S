#include "p_modes.h"

.section .text
.global _start
_start:
	cpsid if @ disable interupts
	ldr sp, =_svc_stack_top
	
	@ Print boot message
	ldr r0, =boot_msg
	bl puts

	@ Clear kernel bss segment	
	ldr r0, =_kernel_bss_start
	ldr r1, =_kernel_bss_end
	mov r2, #0
	mov r3, #0
1:
	stmia r0!, {r2, r3}
	add r0, r0, #4
	cmp r0, r1
	blo 1b

	bl vector_table_init
	
	@ Disable cache and mmu

	mrc p15, 0, r0, c1, c0, 0
	bic r0, r0, #(1 << 12) @ disable instruction cache
	bic r0, r0, #(1 << 2) @ disable data cache
	bic r0, r0, #(1 << 0) @ disable mmu
	mcr p15, 0, r0, c1, c0, 0

	@ Set up stacks
		
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
	
	@ never returns
	bl scheduler_init
	
.section .rodata
boot_msg:
	.asciz "\nBooting...\n\n"
	