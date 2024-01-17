;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Include Guard
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


.ifdef INCLUDE_WRITE_IO
	.exit
.endif

.equ INCLUDE_WRITE_IO = 1


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Imports
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


.INCLUDE "ascii.inc"
.INCLUDE "io.inc"
.INCLUDE "io-name-to-addr.inc"


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Write IO Register
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


WRITE_IO_ERR_MSG: .db "ERROR: Invalid Input For `WriteIO`", ASCII_NEW_LINE, 0
WRITE_IO_SUC_MSG: .db "IO Register Write Complete", ASCII_NEW_LINE, 0


WRITE_IO:
	.def TEMP = R16
	.def BYTE = R17

	; Number of pushes
	.set STACK_OFFSET = 6
	; Backup Registers
	push TEMP
	push BYTE
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

	; i YY (X)+
	cpi TEMP, 6
	brlo WRITE_IO_ERR

	; Load Command Address
	ldd ZH, Y+(STACK_OFFSET + 4)
	ldd ZL, Y+(STACK_OFFSET + 3)

	; Get string address of byte to store
	adiw ZH:ZL, 2

	ldi TEMP, 0
	push TEMP
	push ZH
	push ZL
	rcall ASCII_WORD_TO_HEX
	pop BYTE
	pop TEMP
	pop TEMP
	; IF (SUBROUTINE ERR != 0) THEN ERR
	cpi TEMP, 0
	brne WRITE_IO_ERR

	; Get string address of IO register name
	adiw ZH:ZL, 3
	; Load Command Length
	ldd TEMP, Y+(STACK_OFFSET + 5)
	; Decrease length by 5
	subi TEMP, 5

	push TEMP
	push ZH
	push ZL
	rcall ASCII_IO_REG_TO_MEM_ADDR
	pop ZL
	pop ZH
	pop TEMP
	; IF (SUBROUTINE ERR != 0) THEN ERR
	cpi TEMP, 0
	brne WRITE_IO_ERR

	st Z, BYTE

	ldi TEMP, HIGH(WRITE_IO_SUC_MSG << 1)
	push TEMP
	ldi TEMP, LOW (WRITE_IO_SUC_MSG << 1)
	push TEMP
	rcall PUTSTR
	pop TEMP
	pop TEMP

	rjmp WRITE_IO_RET

	WRITE_IO_ERR:
		ldi TEMP, HIGH(WRITE_IO_ERR_MSG << 1)
		push TEMP
		ldi TEMP, LOW (WRITE_IO_ERR_MSG << 1)
		push TEMP
		rcall PUTSTR
		pop TEMP
		pop TEMP

	WRITE_IO_RET:
		; Restore Registers
		pop ZL
		pop ZH
		pop YL
		pop YH
		pop BYTE
		pop TEMP

		ret
	
	.undef TEMP
	.undef BYTE