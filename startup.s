.equ UART0.BASE, 0x44E09000
.equ UART1.BASE, 0x48022000
.equ UART2.BASE, 0x48024000
.equ UART3.BASE, 0x481A6000
.equ UART4.BASE, 0x481A8000
.equ UART5.BASE, 0x481AA000

.arm
_start:
	ldr r0,=UART0.BASE
	mov r1,#'a'
	strb r1,[r0]
_hang: b _hang
