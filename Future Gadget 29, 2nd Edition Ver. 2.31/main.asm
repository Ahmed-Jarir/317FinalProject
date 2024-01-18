;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;
; Future Gadget 29, 2nd Edition Ver. 2.31
;
; COMP317 Final Project
;
; Authors:
; - Ameer Taweel (0077340)
; - Ahmed Jareer (0074982)
;
; Resources Usage:
; - Program Size: TODO
; - Data    Size: TODO
; - Registers   : TODO
;
; Summary:
;
; Multiple times during this course, we faced a situation where the code works
; in the debugger/simulator but not on the physical microcontroller. Debugging
; and fixing such issues was hard because we couldn't inspect the system's
; internal state.
;
; Therefore, built a system that is easy to inspect. It exposes a shell using
; the ANSI standard over USART. The system has commands to check the system's
; state, like memory regions and I/O pin values. Moreover, the system supports
; periodic tasks, like logging a memory region every two minutes.
;
; It also has commands for modifying the system's state, like setting a memory
; region or setting the mode of an I/O pin.
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; # Code Style
;;
;;   - No global register aliases.
;;   - Capitalize all user-defined names (constants, register aliases, labels).
;;   - Pass subroutine parameters using the stack.
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
;; # Imports
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


.INCLUDE "ascii.inc"
.INCLUDE "io.inc"
.INCLUDE "read-mem.inc"
.INCLUDE "write-mem.inc"
.INCLUDE "read-io.inc"
.INCLUDE "write-io.inc"


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; # Program Memory Constants
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


PROMPT:
	.db "FG>>> ", 0, 0
EXEC_INVALID_CMD:
	.db "ERROR: Invalid Command", ASCII_NEW_LINE, 0


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

	; Read command index
	ldi ZH, HIGH(CMD_IDX)
	ldi ZL, LOW (CMD_IDX)
	ld IDX, Z

	cpi CHAR, ASCII_NEW_LINE

	brne USART_RX_COMPLETE_NOT_DONE

	ldi R16, ASCII_NEW_LINE
	push R16
	rcall PUTCHAR
	pop R16
	
	push IDX
	ldi TEMP, HIGH(CMD)
	push TEMP
	ldi TEMP, LOW (CMD)
	push TEMP
	rcall EXECUTE
	pop TEMP
	pop TEMP
	pop TEMP

	; Print Prompt
	ldi TEMP, HIGH(PROMPT << 1)
	push TEMP
	ldi TEMP, LOW (PROMPT << 1)
	push TEMP
	rcall PUTSTR
	pop TEMP
	pop TEMP

	; Reset index
	clr IDX
	st Z, IDX

	rjmp USART_RX_COMPLETE_RET

	USART_RX_COMPLETE_NOT_DONE:
		cpi CHAR, ASCII_BACKSPACE

		brne USART_RX_COMPLETE_NOT_BACKSPACE

		cpi IDX, 0
		breq USART_RX_COMPLETE_RET

		dec IDX
		st Z, IDX
		; Echo backspace back
		; Backspace -> Space -> Backspace
		ldi R16, ASCII_BACKSPACE
		push R16
		rcall PUTCHAR
		ldi R16, ASCII_SPACE
		push R16
		rcall PUTCHAR
		pop R16
		rcall PUTCHAR
		pop R16

		rjmp USART_RX_COMPLETE_RET

	USART_RX_COMPLETE_NOT_BACKSPACE:
		; Handle full command buffer
		cpi IDX, CMD_MAX_LEN

		breq USART_RX_COMPLETE_RET

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
		push R16
		rcall PUTCHAR
		pop R16

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


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Execute Command
;;
;; Inputs:
;; - Command Length           <- SP + 5
;; - Command String Pointer H <- SP + 4
;; - Command String Pointer L <- SP + 3 (First Byte Before Return Address)
;;
;; Outputs:
;; - No Output                -> SP + 5
;; - No Output                -> SP + 4
;; - No Output                -> SP + 3 (First Byte Before Return Address)
;;
;; Supported Commands:
;; - Read Memory:
;;   r $BEG_ADDR $END_ADDR
;; - Write Memory:
;;   w $BEG_ADDR $BYTES_TO_WRITE
;; - Read IO Register:
;;   o $IO_REG_NAME
;; - Write IO Register:
;;   i $IO_REG_NAME $BYTE
;; - Repeat Command:
;;   p $SECONDS $COMMAND
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


EXECUTE:
	.def TEMP = R16
	
	; Number of pushes
	.set STACK_OFFSET = 5
	; Backup Registers
	push TEMP
	push YH
	push YL
	push ZH
	push ZL

	; Load Stack Pointer
	; Y <- SP
	in YH, SPH
	in YL, SPL

	; Load Command Length
	ldd TEMP, Y+(STACK_OFFSET + 5)
	; Push Command Length (Prepare Subroutine Call)
	push TEMP

	; Two Dummy Pushes In Case Error
	push TEMP
	push TEMP

	; Empty command
	cpi TEMP, 2
	brlo EXECUTE_INVALID

	; Load Command Address
	ldd ZH, Y+(STACK_OFFSET + 4)
	ldd ZL, Y+(STACK_OFFSET + 3)

	; Get second character
	push ZH
	push ZL
	adiw ZH:ZL, 1
	ld TEMP, Z
	pop ZL
	pop ZH
	
	cpi TEMP, ASCII_SPACE
	brne EXECUTE_INVALID

	; Remove Dummy Pushes
	pop TEMP
	pop TEMP

	; Push Command Length (Prepare Subroutine Call)
	push ZH
	push ZL
	
	ld TEMP, Z

	cpi TEMP, ASCII_LOWER_R
	brne EXECUTE_W
	rcall READ_MEM
	rjmp  EXECUTE_RET

	EXECUTE_W:
		cpi TEMP, ASCII_LOWER_W
		brne EXECUTE_O
		rcall WRITE_MEM
		rjmp  EXECUTE_RET

	EXECUTE_O:
		cpi TEMP, ASCII_LOWER_O
		brne EXECUTE_I
		rcall READ_IO
		rjmp  EXECUTE_RET

	EXECUTE_I:
		cpi TEMP, ASCII_LOWER_I
		brne EXECUTE_P
		rcall WRITE_IO
		rjmp  EXECUTE_RET

	EXECUTE_P:
		cpi TEMP, ASCII_LOWER_P
		brne EXECUTE_INVALID
		rcall REPEAT_CMD
		rjmp  EXECUTE_RET
	
	EXECUTE_INVALID:
		; Print Error
		ldi TEMP, HIGH(EXEC_INVALID_CMD << 1)
		push TEMP
		ldi TEMP, LOW (EXEC_INVALID_CMD << 1)
		push TEMP
		rcall PUTSTR
		pop TEMP
		pop TEMP

	EXECUTE_RET:
		pop TEMP
		pop TEMP
		pop TEMP

		; Restore Registers
		pop ZL
		pop ZH
		pop YL
		pop YH
		pop TEMP
	
		ret

	.undef TEMP

REPEAT_CMD:
ret