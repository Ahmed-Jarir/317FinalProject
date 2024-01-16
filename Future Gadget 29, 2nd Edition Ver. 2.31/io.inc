;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Include Guard
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


.ifdef INCLUDE_IO
	.exit
.endif

.equ INCLUDE_IO = 1


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Transmit Character Over USART
;;
;; Inputs:
;; - Char      <- SP + 3 (First Byte Before Return Address)
;;
;; Outputs:
;; - No Output -> SP + 3 (First Byte Before Return Address)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


PUTCHAR:
	; Set Register Aliases
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

	; Load Character
	ldd CHAR, Y+(STACK_OFFSET + 3)

	; Wait until sending is safe
	PUTCHAR_WAIT_SAFE_SEND:	
		sbis UCSRA, UDRE
		rjmp PUTCHAR_WAIT_SAFE_SEND

	out UDR, CHAR
	
	; Restore Registers
	pop YL
	pop YH
	pop CHAR

	ret

	.undef CHAR


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Print a Null-Terminated Program Memory String Over USART
;;
;; Inputs:
;; - String Address H <- SP + 4
;; - String Address L <- SP + 3 (First Byte Before Return Address)
;;
;; Outputs:
;; - No Output        -> SP + 4
;; - No Output        -> SP + 3 (First Byte Before Return Address)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


PUTSTR:
	.def CHAR = R16

	; Number of pushes
	.set STACK_OFFSET = 5
	; Backup Registers
	push CHAR
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

	PUTSTR_LOOP:
		lpm CHAR, Z
		cpi CHAR, 0
		breq PUTSTR_RET
		push CHAR
		rcall PUTCHAR
		pop CHAR
		adiw ZH:ZL, 1
		rjmp PUTSTR_LOOP

	PUTSTR_RET:
		; Restore Registers
		pop ZL
		pop ZH
		pop YL
		pop YH
		pop CHAR

		ret

	.undef CHAR