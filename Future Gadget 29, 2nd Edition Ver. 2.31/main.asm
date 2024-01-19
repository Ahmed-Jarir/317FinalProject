;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Future Gadget 29, 2nd Edition Ver. 2.31
;;
;; COMP317 Final Project
;;
;; Authors:
;; - Ameer Taweel (0077340)
;; - Ahmed Jareer (0074982)
;;
;; Resources Usage:
;; - Program Size: 4238 Bytes
;; - Data    Size: 174  Bytes
;;
;; Summary:
;;
;; Multiple times during this course, we faced a situation where the code works
;; in the debugger/simulator but not on the physical microcontroller. Debugging
;; and fixing such issues was hard because we couldn't inspect the system's
;; internal state.
;;
;; Therefore, built a system that is easy to inspect. It exposes a shell using
;; the ANSI standard over USART. The system has commands to check the system's
;; state, like memory regions and I/O pin values. Moreover, the system supports
;; periodic tasks, like logging a memory region every two minutes.
;;
;; It also has commands for modifying the system's state, like setting a memory
;; region or setting the mode of an I/O pin.
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
; System Clock Frequency
.equ F_CLK     = 4 * 1024 * 1024
; System Oscillator Clock Frequency
.equ F_OSC     = 4 * 1000 * 1000
; Contents of the UBRRH and UBRRL Registers (0 - 4095)
; Asynchronous Normal Mode (U2X = 0)
.equ BAUD_UBRR = (F_OSC / (16 * BAUD_RATE)) - 1

.equ T1_PRESCALE = 1024
.equ T1_MAX_VAL  = (F_CLK / T1_PRESCALE) - 1

; Maximum Command Length
.equ CMD_MAX_LEN     = 32
.equ MAX_REPEAT_CMDS = 4
; 1           Byte  -> Counter
; 1           Byte  -> Interval
; 1           Byte  -> Command Length
; CMD_MAX_LEN Bytes -> Command
.equ REPEAT_CMD_LEN  = CMD_MAX_LEN + 3


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; # Data Segment
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


; Data Segment Start
.dseg

CMD    : .byte CMD_MAX_LEN
CMD_IDX: .byte 1

REPEAT_CMDS: .byte REPEAT_CMD_LEN * MAX_REPEAT_CMDS
REPEAT_IDX : .byte 1


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; # Interrupt Vector Table
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


; Code Segment Start
.cseg

.org 0x0000
	jmp RESET
.org 0x000E
	jmp T1_COMPA
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


.equ PROMPT_LEN = 6

PROMPT:
	.db "FG>>> ", 0, 0
REPEATED_COMMAND_NOTIFICATION:
	.db "PERIODIC CMD: ", 0, 0
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

	; Set Frame Format:
	; - 8 Data Bits
	; - 2 Stop Bits
	ldi TEMP, (1 << URSEL) | (1 << USBS) | (1 << UCSZ0) | (1 << UCSZ1)
	out UCSRC, TEMP

	; Initialize USART END

	; Initialize command index to 0
	ldi ZH, HIGH(CMD_IDX)
	ldi ZL, LOW (CMD_IDX)
	clr TEMP
	st Z, TEMP

	; Initialize repeat index to 0
	ldi ZH, HIGH(REPEAT_IDX)
	ldi ZL, LOW (REPEAT_IDX)
	clr TEMP
	st Z, TEMP

	; Set Timer 1 Max Value
	ldi TEMP, HIGH(T1_MAX_VAL)
	out OCR1AH, TEMP
	ldi TEMP, LOW(T1_MAX_VAL)
	out OCR1AL, TEMP

	; Enable Timer 1 Output Compare Interrupt
	ldi TEMP, 1 << OCIE1A
	out TIMSK, TEMP
	ldi TEMP, 0
	out TCCR1A, TEMP

	; Set Timer 1 Prescale
	ldi TEMP, (1 << WGM12) | (1 << CS12) | (1 << CS10)
	out TCCR1B, TEMP

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


T1_COMPA:
	; Set Register Aliases
	.def TEMP = R16
	.def CNTR = R17
	.def IDX  = R18
	.def FLAG = R19

	; Backup Registers
	push TEMP
	in TEMP, SREG
	push TEMP
	push CNTR
	push IDX
	push FLAG
	push YH
	push YL

	; Get Commands Count
	ldi YH, HIGH(REPEAT_IDX)
	ldi YL, LOW (REPEAT_IDX)
	ld IDX, Y

	cpi IDX, 0
	breq T1_COMPA_RET

	ldi YH, HIGH(REPEAT_CMDS)
	ldi YL, LOW (REPEAT_CMDS)

	; No Command Executed Yet
	ldi FLAG, 0

	ldi CNTR, 0

	T1_COMPA_EXEC:
		; Get Count
		ld TEMP, Y
		dec TEMP
		cpi TEMP, 0
		brne T1_COMPA_NO_EXEC

		; Execute Command

		cpi FLAG, 0
		brne NO_CLEAR

		rcall CLEAR_LINE
		ldi FLAG, 1

		NO_CLEAR:

		; Print Notification
		ldi TEMP, HIGH(REPEATED_COMMAND_NOTIFICATION << 1)
		push TEMP
		ldi TEMP, LOW (REPEATED_COMMAND_NOTIFICATION << 1)
		push TEMP
		rcall PUTSTR
		pop TEMP
		pop TEMP

		; Get Interval
		adiw YH:YL, 1
		ld TEMP, Y
		; Set Count To Interval
		st -Y, TEMP

		; Get Command Length
		adiw YH:YL, 2
		ld TEMP, Y+

		; Execute Command
		push TEMP
		push YH
		push YL
		rcall EXECUTE
		pop TEMP
		pop TEMP
		pop TEMP

		sbiw YH:YL, 3

		rjmp T1_COMPA_COMMON

		T1_COMPA_NO_EXEC:
			; Set Decreased Count
			st Y, TEMP

		T1_COMPA_COMMON:
			subi YL, -REPEAT_CMD_LEN
			sbci YH, 0
			inc CNTR
			cp CNTR, IDX
			brne T1_COMPA_EXEC

	cpi FLAG, 0
	breq T1_COMPA_RET

	rcall DIRTY_LINE

	T1_COMPA_RET:
		; Restore Registers
		pop YL
		pop YH
		pop FLAG
		pop IDX
		pop CNTR
		pop TEMP
		out SREG, TEMP
		pop TEMP

		reti

	; Clear Register Aliases
	.undef TEMP
	.undef CNTR
	.undef IDX
	.undef FLAG


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
;; - List Repeated Commands:
;;   l
;; - Deleted Repeated Command:
;;   d $IDX
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
	cpi TEMP, 0
	breq EXECUTE_INVALID

	; Load Command Address
	ldd ZH, Y+(STACK_OFFSET + 4)
	ldd ZL, Y+(STACK_OFFSET + 3)

	cpi TEMP, 1
	breq SKIP_SPACE_CHECK
	; Get second character
	push ZH
	push ZL
	adiw ZH:ZL, 1
	ld TEMP, Z
	pop ZL
	pop ZH
	
	cpi TEMP, ASCII_SPACE
	brne EXECUTE_INVALID

	SKIP_SPACE_CHECK:

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
		brne EXECUTE_L
		rcall REPEAT_CMD
		rjmp  EXECUTE_RET

	EXECUTE_L:
		cpi TEMP, ASCII_LOWER_L
		brne EXECUTE_D
		rcall LIST_REPEAT_CMD
		rjmp  EXECUTE_RET

	EXECUTE_D:
		cpi TEMP, ASCII_LOWER_D
		brne EXECUTE_INVALID
		rcall DEL_REPEAT_CMD
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


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Repeat Command (Add)
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
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


REPEAT_CMD_ERR_MSG: .db "ERROR: Invalid Input For `RepeatCommand`", ASCII_NEW_LINE, 0
REPEAT_CMD_SUC_MSG: .db "Periodic Command Added", ASCII_NEW_LINE, 0


REPEAT_CMD:
	.def IDX  = R16
	.def LEN  = R17
	.def ITRV = R18
	.def TEMP = R19

	; Number of pushes
	.set STACK_OFFSET = 10
	; Backup Registers
	push IDX
	push LEN
	push ITRV
	push TEMP
	push XH
	push XL
	push YH
	push YL
	push ZH
	push ZL

	; Load Stack Pointer
	; Y <- SP
	in YH, SPH
	in YL, SPL

	; Load Command Length
	ldd LEN, Y+(STACK_OFFSET + 5)

	; p TT r X+
	cpi LEN, 8
	brlo REPEAT_CMD_ERR

	ldi ZH, HIGH(REPEAT_IDX)
	ldi ZL, LOW (REPEAT_IDX)

	; Make Sure We Have Enough Space
	ld IDX, Z
	cpi IDX, MAX_REPEAT_CMDS
	brsh REPEAT_CMD_ERR

	; Get Interval
	ldd XH, Y+(STACK_OFFSET + 4)
	ldd XL, Y+(STACK_OFFSET + 3)
	adiw XH:XL, 2

	ldi TEMP, 0
	push TEMP
	push XH
	push XL
	rcall ASCII_WORD_TO_HEX
	pop ITRV
	pop TEMP
	pop TEMP
	; IF (SUBROUTINE ERR != 0) THEN ERR
	cpi TEMP, 0
	brne REPEAT_CMD_ERR

	cpi ITRV, 0
	breq REPEAT_CMD_ERR

	; Increment Index
	inc IDX
	st Z, IDX

	; Get Command To Repeat
	adiw XH:XL, 3
	subi LEN, 5

	; Get SRAM location to store
	ldi ZH, HIGH(REPEAT_CMDS)
	ldi ZL, LOW (REPEAT_CMDS)
	dec IDX

	REPEAT_CMD_PTR_LOOP:
		cpi IDX, 0
		breq REPEAT_CMD_PTR_LOOP_EXIT
		subi ZL, -REPEAT_CMD_LEN
		sbci ZH, 0
		dec IDX
		rjmp REPEAT_CMD_PTR_LOOP

	REPEAT_CMD_PTR_LOOP_EXIT:
	
	st Z+, ITRV
	st Z+, ITRV
	st Z+, LEN

	; Use ITRV As Loop Counter
	ldi ITRV, 0

	REPEAT_CMD_COPY:
		ld TEMP, X+
		st Z+, TEMP
		inc ITRV
		cp ITRV, LEN
		brlo REPEAT_CMD_COPY

	ldi TEMP, HIGH(REPEAT_CMD_SUC_MSG << 1)
	push TEMP
	ldi TEMP, LOW (REPEAT_CMD_SUC_MSG << 1)
	push TEMP
	rcall PUTSTR
	pop TEMP
	pop TEMP

	rjmp REPEAT_CMD_RET

	REPEAT_CMD_ERR:
		ldi TEMP, HIGH(REPEAT_CMD_ERR_MSG << 1)
		push TEMP
		ldi TEMP, LOW (REPEAT_CMD_ERR_MSG << 1)
		push TEMP
		rcall PUTSTR
		pop TEMP
		pop TEMP

	REPEAT_CMD_RET:
		; Restore Registers
		pop ZL
		pop ZH
		pop YL
		pop YH
		pop XL
		pop XH
		pop TEMP
		pop ITRV
		pop LEN
		pop IDX

		ret

	.undef IDX
	.undef LEN
	.undef ITRV
	.undef TEMP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Repeat Command (List)
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
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


LIST_REPEAT_CMD_INTERVAL: .db   "SEC -> ", 0
LIST_REPEAT_CMD_CMD     : .db ", CMD -> ", 0
LIST_REPEAT_CMD_ERR_MSG : .db "ERROR: `ListRepeatedCommands` Does Not Take Input", ASCII_NEW_LINE, 0, 0


LIST_REPEAT_CMD:
	.def IDX  = R16
	.def LEN  = R17
	.def CNTR = R18
	.def TEMP = R19

	; Number of pushes
	.set STACK_OFFSET = 6
	; Backup Registers
	push IDX
	push LEN
	push CNTR
	push TEMP
	push YH
	push YL

	; Load Stack Pointer
	; Y <- SP
	in YH, SPH
	in YL, SPL

	; Load Command Length
	ldd LEN, Y+(STACK_OFFSET + 5)

	; l
	cpi LEN, 1
	brne LIST_REPEAT_CMD_ERR

	ldi YH, HIGH(REPEAT_IDX)
	ldi YL, LOW (REPEAT_IDX)

	; Get Commands Count
	ld IDX, Y

	cpi IDX, 0
	breq LIST_REPEAT_CMD_RET

	; Get Data Location
	ldi YH, HIGH(REPEAT_CMDS)
	ldi YL, LOW (REPEAT_CMDS)

	ldi CNTR, 0

	LIST_REPEAT_CMD_PRINT_LOOP:
		ldi TEMP, HIGH(LIST_REPEAT_CMD_INTERVAL << 1)
		push TEMP
		ldi TEMP, LOW (LIST_REPEAT_CMD_INTERVAL << 1)
		push TEMP
		rcall PUTSTR
		pop TEMP
		pop TEMP

		; Get Interval
		adiw YH:YL, 1
		ld TEMP, Y+

		; Print Interval
		push TEMP
		push TEMP
		rcall HEX_BYTE_TO_ASCII
		; Get Least Significant Char
		pop TEMP
		rcall PUTCHAR
		push TEMP
		rcall PUTCHAR
		pop TEMP
		pop TEMP

		ldi TEMP, HIGH(LIST_REPEAT_CMD_CMD << 1)
		push TEMP
		ldi TEMP, LOW (LIST_REPEAT_CMD_CMD << 1)
		push TEMP
		rcall PUTSTR
		pop TEMP
		pop TEMP

		; Get Command Length
		ld LEN, Y+

		push CNTR
		ldi CNTR, 0

		LIST_REPEAT_CMD_PRINT_LOOP_INNER:
			ld TEMP, Y+
			push TEMP
			rcall PUTCHAR
			pop TEMP
			inc CNTR
			cp CNTR, LEN
			brlo LIST_REPEAT_CMD_PRINT_LOOP_INNER

		ldi TEMP, ASCII_NEW_LINE
		push TEMP
		rcall PUTCHAR
		pop TEMP

		ldi TEMP, (-1 * CMD_MAX_LEN)
		add TEMP, LEN
		sub YL, TEMP
		sbci YH, 0

		pop CNTR
		inc CNTR
		cp CNTR, IDX
		brlo LIST_REPEAT_CMD_PRINT_LOOP

	rjmp LIST_REPEAT_CMD_RET

	LIST_REPEAT_CMD_ERR:
		ldi TEMP, HIGH(LIST_REPEAT_CMD_ERR_MSG << 1)
		push TEMP
		ldi TEMP, LOW (LIST_REPEAT_CMD_ERR_MSG << 1)
		push TEMP
		rcall PUTSTR
		pop TEMP
		pop TEMP

	LIST_REPEAT_CMD_RET:
		; Restore Registers
		pop YL
		pop YH
		pop TEMP
		pop CNTR
		pop LEN
		pop IDX

		ret

	.undef IDX
	.undef LEN
	.undef CNTR
	.undef TEMP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Repeat Command (Delete)
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
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


DEL_REPEAT_CMD_ERR_MSG: .db "ERROR: Invalid Input For `DeleteRepeatedCommands`", ASCII_NEW_LINE, 0, 0
DEL_REPEAT_CMD_SUC_MSG: .db "Periodic Command Deleted", ASCII_NEW_LINE, 0


DEL_REPEAT_CMD:
	.def IDX  = R16
	.def LEN  = R17
	.def TEMP = R18

	; Number of pushes
	.set STACK_OFFSET = 7
	; Backup Registers
	push IDX
	push LEN
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
	ldd LEN, Y+(STACK_OFFSET + 5)

	; d II
	cpi LEN, 4
	brne DEL_REPEAT_CMD_ERR

	; Load Command Pointer
	ldd ZH, Y+(STACK_OFFSET + 4)
	ldd ZL, Y+(STACK_OFFSET + 3)

	adiw ZH:ZL, 2

	ldi TEMP, 0
	push TEMP
	push ZH
	push ZL
	rcall ASCII_WORD_TO_HEX
	pop IDX
	pop TEMP
	pop TEMP
	; IF (SUBROUTINE ERR != 0) THEN ERR
	cpi TEMP, 0
	brne DEL_REPEAT_CMD_ERR

	ldi YH, HIGH(REPEAT_IDX)
	ldi YL, LOW (REPEAT_IDX)

	; Get Commands Count
	ld TEMP, Y

	; Can't Delete Command That Does Not Exist
	cp IDX, TEMP
	brlo DEL_REPEAT_CMD_NO_ERR

	DEL_REPEAT_CMD_ERR:
		ldi TEMP, HIGH(DEL_REPEAT_CMD_ERR_MSG << 1)
		push TEMP
		ldi TEMP, LOW (DEL_REPEAT_CMD_ERR_MSG << 1)
		push TEMP
		rcall PUTSTR
		pop TEMP
		pop TEMP
		rjmp DEL_REPEAT_CMD_RET

	DEL_REPEAT_CMD_NO_ERR:

	dec TEMP
	st Y, TEMP

	; Done If Deleting Last Command
	cp IDX, TEMP
	breq DEL_REPEAT_CMD_RET_PRINT

	; Get Location Of Deleted Element
	ldi YH, HIGH(REPEAT_CMDS)
	ldi YL, LOW (REPEAT_CMDS)

	push TEMP
	push IDX

	DEL_REPEAT_CMD_PTR_LOOP:
		cpi IDX, 0
		breq DEL_REPEAT_CMD_PTR_LOOP_EXIT
		subi YL, -REPEAT_CMD_LEN
		sbci YH, 0
		dec IDX
		rjmp DEL_REPEAT_CMD_PTR_LOOP

	DEL_REPEAT_CMD_PTR_LOOP_EXIT:

	; How Many Times To Copy
	pop IDX
	pop TEMP
	sub TEMP, IDX
	mov IDX, TEMP

	; Get Location Of Next Element
	mov ZH, YH
	mov ZL, YL
	subi ZL, -REPEAT_CMD_LEN
	sbci ZH, 0

	DEL_REPEAT_CMD_COPY_LOOP:
		cpi IDX, 0
		breq DEL_REPEAT_CMD_RET_PRINT

		ld TEMP, Z+
		st Y+, TEMP
		ld TEMP, Z+
		st Y+, TEMP
		ld LEN, Z+
		st Y+, LEN

		push LEN

		DEL_REPEAT_CMD_COPY_LOOP_INNER:
			cpi LEN, 0
			breq DEL_REPEAT_CMD_COPY_LOOP_INNER_EXIT
			ld TEMP, Z+
			st Y+, TEMP
			dec LEN
			rjmp DEL_REPEAT_CMD_COPY_LOOP_INNER

		DEL_REPEAT_CMD_COPY_LOOP_INNER_EXIT:

		pop LEN

		dec IDX
		ldi TEMP, (-1 * CMD_MAX_LEN)
		add TEMP, LEN
		sub YL, TEMP
		sbci YH, 0
		sub ZL, TEMP
		sbci ZH, 0
		rjmp DEL_REPEAT_CMD_COPY_LOOP

	DEL_REPEAT_CMD_RET_PRINT:
		ldi TEMP, HIGH(DEL_REPEAT_CMD_SUC_MSG << 1)
		push TEMP
		ldi TEMP, LOW (DEL_REPEAT_CMD_SUC_MSG << 1)
		push TEMP
		rcall PUTSTR
		pop TEMP
		pop TEMP

	DEL_REPEAT_CMD_RET:
		; Restore Registers
		pop ZL
		pop ZH
		pop YL
		pop YH
		pop TEMP
		pop LEN
		pop IDX

		ret

	.undef IDX
	.undef LEN
	.undef TEMP

CLEAR_LINE:
	.def TEMP = R16
	.def LEN  = R17

	; Backup Registers
	push TEMP
	push LEN
	push YH
	push YL

	ldi YH, HIGH(CMD_IDX)
	ldi YL, LOW (CMD_IDX)
	ld LEN, Y
	subi LEN, -PROMPT_LEN

	ldi TEMP, ASCII_BACKSPACE
	push TEMP

	mov TEMP, LEN

	CLEAR_LINE_BACKSPACE_LOOP_1:
		rcall PUTCHAR
		dec TEMP
		cpi TEMP, 0
		brne CLEAR_LINE_BACKSPACE_LOOP_1

	pop TEMP

	ldi TEMP, ASCII_SPACE
	push TEMP

	mov TEMP, LEN

	CLEAR_LINE_SPACE_LOOP:
		rcall PUTCHAR
		dec TEMP
		cpi TEMP, 0
		brne CLEAR_LINE_SPACE_LOOP

	pop TEMP

	ldi TEMP, ASCII_BACKSPACE
	push TEMP

	mov TEMP, LEN

	CLEAR_LINE_BACKSPACE_LOOP_2:
		rcall PUTCHAR
		dec TEMP
		cpi TEMP, 0
		brne CLEAR_LINE_BACKSPACE_LOOP_2

	pop TEMP
	
	; Restore Registers
	pop YL
	pop YH
	pop LEN
	pop TEMP
	
	ret

	.undef TEMP
	.undef LEN

DIRTY_LINE:
	.def TEMP = R16
	.def LEN  = R17

	; Backup Registers
	push TEMP
	push LEN
	push YH
	push YL

	; Print Prompt
	ldi TEMP, HIGH(PROMPT << 1)
	push TEMP
	ldi TEMP, LOW (PROMPT << 1)
	push TEMP
	rcall PUTSTR
	pop TEMP
	pop TEMP

	ldi YH, HIGH(CMD_IDX)
	ldi YL, LOW (CMD_IDX)
	ld LEN, Y

	cpi LEN, 0
	breq DIRTY_LINE_RET

	ldi YH, HIGH(CMD)
	ldi YL, LOW (CMD)

	DIRTY_LINE_LOOP:
		ld TEMP, Y+
		push TEMP
		rcall PUTCHAR
		pop TEMP
		dec LEN
		cpi LEN, 0
		brne DIRTY_LINE_LOOP

	DIRTY_LINE_RET:
		; Restore Registers
		pop YL
		pop YH
		pop LEN
		pop TEMP
	
		ret

	.undef TEMP
	.undef LEN
