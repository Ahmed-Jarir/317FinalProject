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
	brlo REPEAT_CMD_ERR

	; Length should be odd
	sbrs LEN, 0
	rjmp REPEAT_CMD_ERR

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
	brne REPEAT_CMD_ERR

	; There should be a space between parameters
	adiw ZH:ZL, 4
	ld TEMP, Z
	cpi TEMP, ASCII_SPACE
	brne REPEAT_CMD_ERR

	; Get string address of bytes to write
	adiw ZH:ZL, 1

	push ZH
	push ZL

	; Get max iterations
	subi LEN, 7
	lsr LEN
	ldi COUNT, 0

	REPEAT_CMD_VALIDATE_LOOP:
		ldi TEMP, 0
		push TEMP
		ld TEMP, Z
		push TEMP
		rcall ASCII_BYTE_TO_HEX
		pop TEMP
		pop TEMP
		; IF (SUBROUTINE ERR != 0) THEN ERR
		cpi TEMP, 0
		brne REPEAT_CMD_ERR
		inc COUNT
		; Get Next Byte
		adiw ZH:ZL, 1
		cp COUNT, LEN
		brne REPEAT_CMD_VALIDATE_LOOP

	pop ZL
	pop ZH
	ldi COUNT, 0
	REPEAT_CMD_WRITE:
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
		brne REPEAT_CMD_WRITE

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
		pop LEN
		pop COUNT
		pop TEMP

		ret
	
	.undef TEMP
	.undef COUNT
	.undef LEN