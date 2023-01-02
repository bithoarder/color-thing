DDRB	= 0x17
PORTB	= 0x18
UARTBIT	= 3

	.text

.macro	BANG0	bit, long
	lsr	r24		; [1]
	brcs	L_\bit\()_1	; [2]
	nop			; [3]
L_\bit\()_0:
	cbi	PORTB, UARTBIT	; [5]
.if	\long != 0
	nop
.endif
.endm

.macro	BANG1	bit, long
	lsr	r24		; [1]
	brcc	L_\bit\()_0	; [2]
	nop			; [3]
L_\bit\()_1:
	sbi	PORTB, UARTBIT	; [5]
.if	\long
	nop
.endif
.endm

;------------------------------------------------------------------------------

.global	writeSerialByte
writeSerialByte:
	sbi	PORTB, UARTBIT
	sbi	DDRB, UARTBIT
	nop
	nop
	nop
	nop
	nop
	nop


	;; send start bit
	cbi	PORTB, UARTBIT

	BANG0	0, 1
	BANG0	1, 0
	BANG0	2, 1
	BANG0	3, 0
	BANG0	4, 1
	BANG0	5, 0
	BANG0	6, 1
	BANG0	7, 0
	rjmp	L_stop

	BANG1	0, 1
	BANG1	1, 0
	BANG1	2, 1
	BANG1	3, 0
	BANG1	4, 1
	BANG1	5, 0
	BANG1	6, 1
	BANG1	7, 0
	nop
	nop

L_stop:	nop			; [3]
	sbi	PORTB, UARTBIT	; [5]
	ret
