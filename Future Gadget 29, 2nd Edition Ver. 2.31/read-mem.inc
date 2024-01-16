;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Include Guard
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


.ifdef INCLUDE_READ_MEM
	.exit
.endif

.equ INCLUDE_READ_MEM = 1


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Imports
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


.INCLUDE "ascii.inc"
.INCLUDE "io.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Read Memory Region
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


READ_SRAM_ERR_MSG: .db "ERROR: Invalid Input For `ReadMem`", ASCII_NEW_LINE, 0


READ_MEM:
	.def TEMP  = R16

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

	; r XXXX XXXX
	cpi TEMP, 11
	brne READ_MEM_ERR

	; Load Command Address
	ldd ZH, Y+(STACK_OFFSET + 4)
	ldd ZL, Y+(STACK_OFFSET + 3)

	; Get string address of first memory address
	adiw ZH:ZL, 2

	; Parse first memory address
	ldi TEMP, 0
	push TEMP
	push ZH
	push ZL
	rcall ASCII_QUAD_TO_HEX
	pop YL
	pop YH
	pop TEMP
	; IF (SUBROUTINE ERR != 0) THEN ERR
	cpi TEMP, 0
	brne READ_MEM_ERR

	; There should be a space between parameters
	adiw ZH:ZL, 4
	ld TEMP, Z
	cpi TEMP, ASCII_SPACE
	brne READ_MEM_ERR

	; Get string address of second memory address
	adiw ZH:ZL, 1

	; Parse second memory address
	ldi TEMP, 0
	push TEMP
	push ZH
	push ZL
	rcall ASCII_QUAD_TO_HEX
	pop ZL
	pop ZH
	pop TEMP
	; IF (SUBROUTINE ERR != 0) THEN ERR
	cpi TEMP, 0
	brne READ_MEM_ERR

	; Ensure First Address <= Second Address
	cp YH, ZH
	brlo READ_MEM_PARAMS_LOOP
	brne READ_MEM_ERR

	; High byte equal
	; Check low byte
	cp YL, ZL
	brlo READ_MEM_PARAMS_LOOP
	brne READ_MEM_ERR

	; Params Valid
	READ_MEM_PARAMS_LOOP:
		; Read Memory
		ld TEMP, Y+

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
		ldi TEMP, ASCII_SPACE
		push TEMP
		rcall PUTCHAR
		pop TEMP

		cp YH, ZH
		brlo READ_MEM_PARAMS_LOOP
		brne READ_MEM_PARAMS_LOOP_EXIT

		cp YL, ZL
		brlo READ_MEM_PARAMS_LOOP
		brne READ_MEM_PARAMS_LOOP_EXIT

	READ_MEM_PARAMS_LOOP_EXIT:
		ldi TEMP, ASCII_BACKSPACE
		push TEMP
		rcall PUTCHAR
		pop TEMP
		ldi TEMP, ASCII_NEW_LINE
		push TEMP
		rcall PUTCHAR
		pop TEMP
		rjmp READ_MEM_RET

	READ_MEM_ERR:
		ldi TEMP, HIGH(READ_SRAM_ERR_MSG << 1)
		push TEMP
		ldi TEMP, LOW (READ_SRAM_ERR_MSG << 1)
		push TEMP
		rcall PUTSTR
		pop TEMP
		pop TEMP

	READ_MEM_RET:
		; Restore Registers
		pop ZL
		pop ZH
		pop YL
		pop YH
		pop TEMP

		ret
	
	.undef TEMP