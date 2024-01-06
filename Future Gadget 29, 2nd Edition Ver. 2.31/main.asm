
;*****************************************************************
;
; File Name		: ‘Lab3-uart.asm'
; Title			: Interrupt-Driven UART sample code
; Date			: 11/20/2013
; Version		: 1.1
; Target MCU	        : ATMega32
;
;*****************************************************************

; define constants
.equ	ZeroASCII = 48
.equ	AASCII = 65
.equ	BAUD_RATE	= 9600
.equ	CPU_CLOCK	= 4000000
.equ	BAUD_UBRR	= (CPU_CLOCK/(16*BAUD_RATE))-1

.equ	MELOSIZE	= 200

.equ	Fck = CPU_CLOCK/64
; define register aliases
.def	SENDCHAR	= R1
.def	TEMP		= r16	; temporary register
.def	TEMP1		= r17	; another temporary register

; data segment
        .dseg
melody: .byte	MELOSIZE

; code segment
.cseg
.org    $000
	jmp	reset		; $000 HW reset or watchdog
.org $012
    jmp		tim1_ovf                 ; Timer1 overflow Handler
/*.org $01A 
	jmp USART_Receive
*/; begin main code
reset:
	ldi 	TEMP, low(RAMEND)		; initialize stack pointer
	out		SPL, TEMP
	ldi 	TEMP, high(RAMEND)
	out 	SPH, TEMP

	; set baud rate
	ldi	TEMP, low(BAUD_UBRR)
	out	UBRRL, TEMP
	ldi	TEMP, high(BAUD_UBRR)
	out	UBRRH, TEMP

	;set timer1 PWM mode
	ldi   TEMP,(1<<WGM10)+(1<<COM1B1)
	out   TCCR1A,TEMP								; 8 bit Fast PWM non-inverting (Fck/256)
	ldi   TEMP,(1<<WGM12)+(1<<CS10)+(1<<CS11)		; 8 bit Fast PWM non-inverting (Fck/256)
	out   TCCR1B,TEMP								; prescaler = 64
	
	;enable timer1 / timer0 interrupt
	ldi   TEMP,(1<<TOIE1)
	out   TIMSK,TEMP

	sbi UCSRB, TXEN			; enable transmitter
	sbi UCSRB, RXEN			; enable receiver
    
	; set frame format: 8 data, 2 stop bits	ldi TEMP, (1 << URSEL)|(1 << USBS)| (3 << UCSZ0)	mov UCSRC, TEMP
    sei					; enable interrupts

mloop:
	rcall USART_Receive
	rjmp mloop

USART_Receive:
	; Wait for data to be received
	sbis UCSRA, RXC
	rjmp USART_Receive
	; Get and return received data from buffer
	in TEMP, UDR
	mov SENDCHAR, TEMP
	rcall putchar
	ret

; subroutines
putchar:
        sbis    UCSRA, UDRE		; loop until USR:UDRE is 1
        rjmp    putchar
        out     UDR, SENDCHAR		; write SENTCHAR to transmitter buffer
        ret

; Interrupt Handler

tim1_ovf:
	push   TEMP    				; Store temporary register
	in     TEMP,SREG
	push   TEMP					; Store status register

	pop     TEMP
	out     SREG, TEMP             ; Restore SREG
	pop     TEMP                  ; Restore temporary register;
	reti
/*USART_Receive:
	; Wait for data to be received
	sbis UCSRA, RXC
	rjmp USART_Receive
	; Get and return received data from buffer
	in TEMP, UDR
	mov SENDCHAR, TEMP
	rcall putchar
	reti
*/