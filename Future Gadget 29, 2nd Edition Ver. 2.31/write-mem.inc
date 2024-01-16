;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Include Guard
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


.ifdef INCLUDE_WRITE_MEM
	.exit
.endif

.equ INCLUDE_WRITE_MEM = 1


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Imports
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


.INCLUDE "ascii.inc"
.INCLUDE "io.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Write Memory Address
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


WRITE_MEM_ERR_MSG: .db "ERROR: Invalid Input For `WriteMem`", ASCII_NEW_LINE, 0, 0
WRITE_MEM_SUC_MSG: .db "Memory Write Complete", ASCII_NEW_LINE, 0, 0


WRITE_MEM:
	.def TEMP  = R16
	.def COUNT = R17
	.def LEN   = R18

	; Number of pushes
	.set STACK_OFFSET = 7
	; Backup Registers
	push TEMP
	push COUNT
	push LEN
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

	; w XXXX (YY)+
	cpi LEN, 9
	brlo WRITE_MEM_ERR

	; Length should be odd
	sbrs LEN, 0
	rjmp WRITE_MEM_ERR

	; Load Command Address
	ldd ZH, Y+(STACK_OFFSET + 4)
	ldd ZL, Y+(STACK_OFFSET + 3)

	; Get string address of memory address
	adiw ZH:ZL, 2

	; Parse memory address
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
	brne WRITE_MEM_ERR

	; There should be a space between parameters
	adiw ZH:ZL, 4
	ld TEMP, Z
	cpi TEMP, ASCII_SPACE
	brne WRITE_MEM_ERR

	; Get string address of bytes to write
	adiw ZH:ZL, 1

	push ZH
	push ZL

	; Get max iterations
	subi LEN, 7
	lsr LEN
	ldi COUNT, 0

	WRITE_MEM_VALIDATE_LOOP:
		ldi TEMP, 0
		push TEMP
		ld TEMP, Z
		push TEMP
		rcall ASCII_BYTE_TO_HEX
		pop TEMP
		pop TEMP
		; IF (SUBROUTINE ERR != 0) THEN ERR
		cpi TEMP, 0
		brne WRITE_MEM_ERR
		inc COUNT
		; Get Next Byte
		adiw ZH:ZL, 1
		cp COUNT, LEN
		brne WRITE_MEM_VALIDATE_LOOP

	pop ZL
	pop ZH
	ldi COUNT, 0
	WRITE_MEM_WRITE:
		ldi TEMP, 0
		push TEMP
		push ZH
		push ZL
		rcall ASCII_WORD_TO_HEX
		pop TEMP
		st Y+, TEMP
		pop TEMP
		pop TEMP
		adiw ZH:ZL, 2
		inc COUNT
		cp COUNT, LEN
		brne WRITE_MEM_WRITE

	ldi TEMP, HIGH(WRITE_MEM_SUC_MSG << 1)
	push TEMP
	ldi TEMP, LOW (WRITE_MEM_SUC_MSG << 1)
	push TEMP
	rcall PUTSTR
	pop TEMP
	pop TEMP

	rjmp WRITE_MEM_RET

	WRITE_MEM_ERR:
		ldi TEMP, HIGH(WRITE_MEM_ERR_MSG << 1)
		push TEMP
		ldi TEMP, LOW (WRITE_MEM_ERR_MSG << 1)
		push TEMP
		rcall PUTSTR
		pop TEMP
		pop TEMP

	WRITE_MEM_RET:
		; Restore Registers
		pop ZL
		pop ZH
		pop YL
		pop YH
		pop LEN
		pop COUNT
		pop TEMP

		ret
	
	.undef TEMP
	.undef COUNT
	.undef LEN