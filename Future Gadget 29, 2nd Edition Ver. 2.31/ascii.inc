;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Include Guard
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


.ifdef INCLUDE_ASCII
	.exit
.endif

.equ INCLUDE_ASCII = 1


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; ASCII Codes
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


.equ ASCII_BACKSPACE = 0x08
.equ ASCII_NEW_LINE  = 0x0D
.equ ASCII_SPACE     = 0x20
.equ ASCII_ZERO      = 0x30
.equ ASCII_NINE      = 0x39
.equ ASCII_ARROW_R   = 0x3E
.equ ASCII_UPPER_A   = 0x41
.equ ASCII_UPPER_E   = 0x45
.equ ASCII_UPPER_F   = 0x46
.equ ASCII_UPPER_G   = 0x47
.equ ASCII_LOWER_A   = 0x61
.equ ASCII_LOWER_F   = 0x66
.equ ASCII_LOWER_G   = 0x67
.equ ASCII_LOWER_I   = 0x69
.equ ASCII_LOWER_O   = 0x6F
.equ ASCII_LOWER_P   = 0x70
.equ ASCII_LOWER_R   = 0x72
.equ ASCII_LOWER_S   = 0x73
.equ ASCII_LOWER_W   = 0x77


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Parse ASCII Character As Hex
;;
;; Inputs:
;; - Constant Zero <- SP + 4
;; - ASCII    Char <- SP + 3 (First Byte Before Return Address)
;;
;; Outputs:
;; - Error Status  -> SP + 4
;; - Hex   Digit   -> SP + 3 (First Byte Before Return Address)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


ASCII_BYTE_TO_HEX:
	.def CHAR = R16

	; Number of pushes
	.set STACK_OFFSET = 3
	; Backup Registers
	push CHAR
	push YH
	push YL

	; Load Stack Pointer
	; Y <- SP
	in YH, SPH
	in YL, SPL

	; Load ASCII Character
	ldd CHAR, Y+(STACK_OFFSET + 3)

	; IF (CHAR < ASCII_ZERO) THEN ERR
	cpi CHAR, ASCII_ZERO
	brlo ASCII_BYTE_TO_HEX_ERR

	; IF (CHAR <= ASCII_NINE) THEN DIGIT
	cpi CHAR, ASCII_NINE + 1
	brlo ASCII_BYTE_TO_HEX_DIGIT

	; IF (CHAR < ASCII_UPPER_A) THEN ERR
	cpi CHAR, ASCII_UPPER_A
	brlo ASCII_BYTE_TO_HEX_ERR

	; IF (CHAR <= ASCII_UPPER_F) THEN UPPER CHAR
	cpi CHAR, ASCII_UPPER_F + 1
	brlo ASCII_BYTE_TO_HEX_UPPER_CHAR

	; IF (CHAR < ASCII_LOWER_A) THEN ERR
	cpi CHAR, ASCII_LOWER_A
	brlo ASCII_BYTE_TO_HEX_ERR

	; IF (CHAR <= ASCII_LOWER_F) THEN LOWER CHAR
	cpi CHAR, ASCII_LOWER_F + 1
	brlo ASCII_BYTE_TO_HEX_LOWER_CHAR

	ASCII_BYTE_TO_HEX_DIGIT:
		; CHAR <- CHAR - ASCII_ZERO
		subi CHAR, ASCII_ZERO
		; Return Hex Digit
		std Y+(STACK_OFFSET + 3), CHAR
		rjmp ASCII_BYTE_TO_HEX_RET

	ASCII_BYTE_TO_HEX_UPPER_CHAR:
		; CHAR <- CHAR - ASCII_UPPER_A + 10
		subi CHAR, (ASCII_UPPER_A - 10)
		; Return Hex Digit
		std Y+(STACK_OFFSET + 3), CHAR
		rjmp ASCII_BYTE_TO_HEX_RET

	ASCII_BYTE_TO_HEX_LOWER_CHAR:
		; CHAR <- CHAR - ASCII_LOWER_A + 10
		subi CHAR, (ASCII_LOWER_A - 10)
		; Return Hex Digit
		std Y+(STACK_OFFSET + 3), CHAR
		rjmp ASCII_BYTE_TO_HEX_RET

	ASCII_BYTE_TO_HEX_ERR:
		; Error Status <- True
		ldi CHAR, 1
		std Y+(STACK_OFFSET + 4), CHAR

	ASCII_BYTE_TO_HEX_RET:
		; Restore Registers
		pop YL
		pop YH
		pop CHAR

		ret

	.undef CHAR


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Parse Two ASCII Digits As Hex
;;
;; Inputs:
;; - Constant Zero          <- SP + 5
;; - ASCII String Pointer H <- SP + 4
;; - ASCII String Pointer L <- SP + 3 (First Byte Before Return Address)
;;
;; Outputs:
;; - Error Status           -> SP + 5
;; - No    Output           -> SP + 4
;; - Return Value           -> SP + 3 (First Byte Before Return Address)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


ASCII_WORD_TO_HEX:
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

	; Load String Address
	ldd ZH, Y+(STACK_OFFSET + 4)
	ldd ZL, Y+(STACK_OFFSET + 3)

	; Push Zero To Stack
	ldi TEMP, 0
	push TEMP
	; Load Most Significant ASCII Character
	ld TEMP, Z+
	; Push ASCII Character To Stack
	push TEMP
	rcall ASCII_BYTE_TO_HEX
	; Get Return Value
	pop BYTE
	; Get Error Status
	pop TEMP
	; IF (SUBROUTINE ERR != 0) THEN ERR
	cpi TEMP, 0
	brne ASCII_WORD_TO_HEX_ERR

	; Byte <- Byte << 4
	lsl BYTE
	lsl BYTE
	lsl BYTE
	lsl BYTE

	; Push Zero To Stack
	ldi TEMP, 0
	push TEMP
	; Load Most Significant ASCII Character
	ld TEMP, Z
	; Push ASCII Character To Stack
	push TEMP
	rcall ASCII_BYTE_TO_HEX
	; Get Return Value
	pop TEMP
	; Append Return Value To Byte
	or BYTE, TEMP
	; Get Error Status
	pop TEMP
	; IF (SUBROUTINE ERR != 0) THEN ERR
	cpi TEMP, 0
	brne ASCII_WORD_TO_HEX_ERR

	; Return Valid Value
	std Y+(STACK_OFFSET + 3), BYTE
	rjmp ASCII_WORD_TO_HEX_RET

	ASCII_WORD_TO_HEX_ERR:
		ldi TEMP, 1
		std Y+(STACK_OFFSET + 5), TEMP

	ASCII_WORD_TO_HEX_RET:
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

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Parse Four ASCII Digits As Hex
;;
;; Inputs:
;; - Constant Zero          <- SP + 5
;; - ASCII String Pointer H <- SP + 4
;; - ASCII String Pointer L <- SP + 3 (First Byte Before Return Address)
;;
;; Outputs:
;; - Error Status           -> SP + 5
;; - Return Value H         -> SP + 4
;; - Return Value L         -> SP + 3 (First Byte Before Return Address)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


ASCII_QUAD_TO_HEX:
	.def TEMP  = R16
	.def BYTEH = R17
	.def BYTEL = R18

	; Number of pushes
	.set STACK_OFFSET = 7
	; Backup Registers
	push TEMP
	push BYTEH
	push BYTEL
	push YH
	push YL
	push ZH
	push ZL

	; Load Stack Pointer
	; Y <- SP
	in YH, SPH
	in YL, SPL

	; Load String Address
	ldd ZH, Y+(STACK_OFFSET + 4)
	ldd ZL, Y+(STACK_OFFSET + 3)

	; Push Arguments To Stack
	ldi TEMP, 0
	push TEMP
	push ZH
	push ZL
	; Parse High Bytes
	rcall ASCII_WORD_TO_HEX
	; Get Return Value
	pop BYTEH
	; Discard Dummy Output
	pop TEMP
	; Get Error Status
	pop TEMP
	; IF (SUBROUTINE ERR != 0) THEN ERR
	cpi TEMP, 0
	brne ASCII_QUAD_TO_HEX_ERR

	; Advance To Low Bytes
	adiw ZH:ZL, 2

	; Push Arguments To Stack
	ldi TEMP, 0
	push TEMP
	push ZH
	push ZL
	; Parse Low Bytes
	rcall ASCII_WORD_TO_HEX
	; Get Return Value
	pop BYTEL
	; Discard Dummy Output
	pop TEMP
	; Get Error Status
	pop TEMP
	; IF (SUBROUTINE ERR != 0) THEN ERR
	cpi TEMP, 0
	brne ASCII_QUAD_TO_HEX_ERR

	; Return Valid Value
	std Y+(STACK_OFFSET + 4), BYTEH
	std Y+(STACK_OFFSET + 3), BYTEL
	rjmp ASCII_QUAD_TO_HEX_RET

	ASCII_QUAD_TO_HEX_ERR:
		ldi TEMP, 1
		std Y+(STACK_OFFSET + 5), TEMP

	ASCII_QUAD_TO_HEX_RET:
		; Restore Registers
		pop ZL
		pop ZH
		pop YL
		pop YH
		pop BYTEL
		pop BYTEH
		pop TEMP

		ret

	.undef TEMP
	.undef BYTEH
	.undef BYTEL


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Convert Hex Nibble To ASCII Character
;;
;; Inputs:
;; - Nibble     <- SP + 3 (First Byte Before Return Address)
;;
;; Outputs:
;; - ASCII Char -> SP + 3 (First Byte Before Return Address)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


HEX_NIBBLE_TO_ASCII:
	.def NIBBLE = R16

	; Number of pushes
	.set STACK_OFFSET = 3
	; Backup Registers
	push NIBBLE
	push YH
	push YL

	; Load Stack Pointer
	; Y <- SP
	in YH, SPH
	in YL, SPL

	; Load Hex Nibble
	ldd NIBBLE, Y+(STACK_OFFSET + 3)

	cpi NIBBLE, 10
	brlo HEX_NIBBLE_TO_ASCII_DIGIT

	HEX_NIBBLE_TO_ASCII_CHAR:
		; NIBBLE <- NIBBLE - 10 + ASCII_UPPER_A
		subi NIBBLE, (10 - ASCII_UPPER_A)
		rjmp HEX_NIBBLE_TO_ASCII_RET

	HEX_NIBBLE_TO_ASCII_DIGIT:
		; NIBBLE <- NIBBLE + ASCII_ZERO
		subi NIBBLE, (-ASCII_ZERO)

	HEX_NIBBLE_TO_ASCII_RET:
		std Y+(STACK_OFFSET + 3), NIBBLE

		; Restore Registers
		pop YL
		pop YH
		pop NIBBLE

		ret

	.undef NIBBLE

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Convert Hex Byte To Two ASCII Characters
;;
;; Inputs:
;; - Dummy Input                    <- SP + 4
;; - Byte                           <- SP + 3 (First Byte Before Return Address)
;;
;; Outputs:
;; - ASCII Char (Most  Significant) -> SP + 4
;; - ASCII Char (Least Significant) -> SP + 3 (First Byte Before Return Address)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


HEX_BYTE_TO_ASCII:
	.def BYTE   = R16
	.def NIBBLE = R17

	; Number of pushes
	.set STACK_OFFSET = 4
	; Backup Registers
	push BYTE
	push NIBBLE
	push YH
	push YL

	; Load Stack Pointer
	; Y <- SP
	in YH, SPH
	in YL, SPL

	; Load Hex Byte
	ldd BYTE, Y+(STACK_OFFSET + 3)

	; Convert Least Significant Nibble
	mov NIBBLE, BYTE
	andi NIBBLE, 0x0F
	push NIBBLE
	rcall HEX_NIBBLE_TO_ASCII
	pop NIBBLE

	; Output Least Significant Nibble
	std Y+(STACK_OFFSET + 3), NIBBLE

	; Convert Most Significant Nibble
	mov NIBBLE, BYTE
	andi NIBBLE, 0xF0
	swap NIBBLE
	push NIBBLE
	rcall HEX_NIBBLE_TO_ASCII
	pop NIBBLE

	; Output Most Significant Nibble
	std Y+(STACK_OFFSET + 4), NIBBLE

	; Restore Registers
	pop YL
	pop YH
	pop NIBBLE
	pop BYTE

	ret

	.undef BYTE
	.undef NIBBLE