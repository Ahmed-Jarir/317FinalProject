;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Include Guard
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


.ifdef INCLUDE_READ_IO
	.exit
.endif

.equ INCLUDE_READ_IO = 1


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Imports
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


.INCLUDE "ascii.inc"
.INCLUDE "io.inc"
.INCLUDE "io-name-to-addr.inc"


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Read IO Register
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


READ_IO_ERR_MSG: .db "ERROR: Invalid Input For `ReadIO`", ASCII_NEW_LINE, 0, 0


READ_IO:
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

	; o (X)+
	cpi TEMP, 3
	brlo READ_IO_ERR

	; Load Command Address
	ldd ZH, Y+(STACK_OFFSET + 4)
	ldd ZL, Y+(STACK_OFFSET + 3)

	; Get string address of IO register name
	adiw ZH:ZL, 2
	; Decrease length by 2
	subi TEMP, 2

	push TEMP
	push ZH
	push ZL
	rcall ASCII_IO_REG_TO_MEM_ADDR
	pop ZL
	pop ZH
	pop TEMP
	; IF (SUBROUTINE ERR != 0) THEN ERR
	cpi TEMP, 0
	brne READ_IO_ERR

	; Read IO Register Value
	ld TEMP, Z
	; Print as ASCII
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

	; Print New Line
	ldi TEMP, ASCII_NEW_LINE
	push TEMP
	rcall PUTCHAR
	pop TEMP

	rjmp READ_IO_RET

	READ_IO_ERR:
		ldi TEMP, HIGH(READ_IO_ERR_MSG << 1)
		push TEMP
		ldi TEMP, LOW (READ_IO_ERR_MSG << 1)
		push TEMP
		rcall PUTSTR
		pop TEMP
		pop TEMP

	READ_IO_RET:
		; Restore Registers
		pop ZL
		pop ZH
		pop YL
		pop YH
		pop TEMP

		ret
	
	.undef TEMP