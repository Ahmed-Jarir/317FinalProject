;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Title			: Interrupt-Driven USART sample code
;; Date			: 11/20/2013
;; Version		: 1.1
;; Target MCU	  : ATMega32
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; # Code Style
;;
;;   - No global register aliases.
;;   - Capitalize all user-defined names (constants, register aliases, labels).
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; # Constants
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Baud Rate in Bits Per Second (bps)
.equ BAUD_RATE = 9600
; System Oscillator Clock Frequency
.equ F_OSC     = 4 * 1000 * 1000
; Contents of the UBRRH and UBRRL Registers (0 - 4095)
; Asynchronous Normal Mode (U2X = 0)
.equ BAUD_UBRR = (F_OSC / (16 * BAUD_RATE)) - 1

; Maximum Command Length
.equ CMD_MAX_LEN = 100

; ASCII Codes
.equ ASCII_BACKSPACE = 0x08
.equ ASCII_NEW_LINE  = 0x0A
.equ ASCII_SPACE     = 0x20
.equ ASCII_ZERO      = 48
.equ ASCII_A         = 65

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; # Data Segment
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Data Segment Start
.dseg

CMD    : .byte CMD_MAX_LEN
CMD_IDX: .byte 1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; # Interrupt Vector Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Code Segment Start
.cseg

.org 0x0000
	jmp RESET
.org 0x001A 
	jmp USART_RX_COMPLETE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; # Main Routine
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

RESET:
	; Set Register Aliases
	.def TEMP = R16

	; Initialize Stack Pointer
	ldi TEMP, HIGH(RAMEND)
	out SPH, TEMP
	ldi TEMP, LOW(RAMEND)
	out SPL, TEMP

	; Initialize USART BEGIN

	; Set Baud Rate
	ldi TEMP, HIGH(BAUD_UBRR)
	out UBRRH, TEMP
	ldi TEMP, LOW(BAUD_UBRR)
	out UBRRL, TEMP

	; TXEN  -> Enable Transmitter
	; RXEN  -> Enable Receiver
	; RXCIE -> Enable Rx Complete Interrupt
	ldi TEMP, (1 << RXCIE) | (1 << RXEN) | (1 << TXEN)
	out UCSRB, TEMP

	; Set Frame Format:	; - 8 Data Bits	; - 2 Stop Bits	ldi TEMP, (1 << URSEL) | (1 << USBS) | (1 << UCSZ0) | (1 << UCSZ1)	out UCSRC, TEMP

	; Initialize USART END

	; Initialie command index to 0
	ldi ZH, HIGH(CMD_IDX)
	ldi ZL, LOW (CMD_IDX)
	clr TEMP
	st Z, TEMP

	; Enable Global Interrupts
    sei

	RESET_LOOP:
		rjmp RESET_LOOP

	; Clear Register Aliases
	.undef TEMP

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; # Interrupt Handlers
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

USART_RX_COMPLETE:
	; Set Register Aliases
	.def TEMP = R16
	.def CHAR = R17
	.def IDX  = R18

	; Backup Registers
	push TEMP
	in TEMP, SREG
	push TEMP
	push ZH
	push ZL
	push YH
	push YL
	push CHAR
	push IDX

	; Read ASCII Character
	in CHAR, UDR

	cpi CHAR, ASCII_NEW_LINE

	brne USART_RX_COMPLETE_NOT_DONE
	
	; TODO: Call parser and executor
	nop
	rjmp USART_RX_COMPLETE_RET

	USART_RX_COMPLETE_NOT_DONE:
		ldi ZH, HIGH(CMD_IDX)
		ldi ZL, LOW (CMD_IDX)
		ld IDX, Z

		cpi CHAR, ASCII_BACKSPACE

		brne USART_RX_COMPLETE_NOT_BACKSPACE

		cpi IDX, 0
		breq USART_RX_COMPLETE_RET

		dec IDX
		st Z, IDX
		; Echo backspace back
		ldi R16, ASCII_BACKSPACE
		rcall PUTCHAR
		ldi R16, ASCII_SPACE
		rcall PUTCHAR
		ldi R16, ASCII_BACKSPACE
		rcall PUTCHAR

		rjmp USART_RX_COMPLETE_RET

	USART_RX_COMPLETE_NOT_BACKSPACE:
		; Handle full command buffer
		cpi IDX, CMD_MAX_LEN

		brne USART_RX_COMPLETE_NOT_FULL

		rjmp USART_RX_COMPLETE_RET

	USART_RX_COMPLETE_NOT_FULL:
		; Get next-character index
		ldi YH, HIGH(CMD)
		ldi YL, LOW(CMD)
		add YL, IDX
		clr TEMP
		adc YH, TEMP

		; Store character
		st Y, CHAR
	
		; Increment index
		inc IDX
		st Z, IDX

		; Echo character back
		mov R16, CHAR
		rcall PUTCHAR

	USART_RX_COMPLETE_RET:
		; Restore Registers
		pop IDX
		pop CHAR
		pop YL
		pop YH
		pop ZL
		pop ZH
		pop TEMP
		out SREG, TEMP
		pop TEMP

		reti

	; Clear Register Aliases
	.undef TEMP
	.undef CHAR
	.undef IDX

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; # Subroutines
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

; Inputs:
; - CHAR: R16
PUTCHAR:
	; Set Register Aliases
	.def CHAR = R16

	; Wait until sending is safe
	sbis UCSRA, UDRE
	rjmp PUTCHAR

	out UDR, CHAR
	
	ret

	.undef CHAR