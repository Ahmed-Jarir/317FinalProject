;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Include Guard
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


.ifdef INCLUDE_IO_NAME_TO_ADDR
	.exit
.endif

.equ INCLUDE_IO_NAME_TO_ADDR = 1


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Parse String As IO Register And Return Memory Address
;;
;; Inputs:
;; - String Length          <- SP + 5
;; - ASCII String Pointer H <- SP + 4
;; - ASCII String Pointer L <- SP + 3 (First Byte Before Return Address)
;;
;; Outputs:
;; - Error Status           -> SP + 5
;; - Return Value H         -> SP + 4
;; - Return Value L         -> SP + 3 (First Byte Before Return Address)
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


ASCII_IO_REG_TO_MEM_ADDR:
	.def LEN  = R16
	.def CHAR = R17
	.def CNTR = R18
	.def TEMP = R19

	; Number of pushes
	.set STACK_OFFSET = 8
	; Backup Registers
	push LEN
	push CHAR
	push CNTR
	push TEMP
	push YH
	push YL
	push ZH
	push ZL

	; Load Stack Pointer
	; Z <- SP
	in ZH, SPH
	in ZL, SPL

	; Load String Length
	ldd LEN, Z+(STACK_OFFSET + 5)

	; Load String Pointer
	ldd YH, Z+(STACK_OFFSET + 4)
	ldd YL, Z+(STACK_OFFSET + 3)

	; Load Tree Pointer (Program Memory)
	ldi ZH, HIGH(IO_LOOKUP_TREE << 1)
	ldi ZL, LOW (IO_LOOKUP_TREE << 1)

	ldi CNTR, 0

	ASCII_IO_REG_TO_MEM_ADDR_LOOP:
		ld CHAR, Y
		; Skip Done Leaf
		adiw ZH:ZL, 2
		ASCII_IO_REG_TO_MEM_ADDR_INNER_LOOP:
			; Get Character In Tree
			lpm TEMP, Z
			cpi TEMP, 0
			; Invlaid String
			breq ASCII_IO_REG_TO_MEM_ADDR_ERR
			cp TEMP, CHAR
			breq ASCII_IO_REG_TO_MEM_ADDR_INNER_LOOP_MATCH
			; No Match
			adiw ZH:ZL, 4
			rjmp ASCII_IO_REG_TO_MEM_ADDR_INNER_LOOP
			ASCII_IO_REG_TO_MEM_ADDR_INNER_LOOP_MATCH:
				adiw ZH:ZL, 2
				; High Byte
				lpm TEMP, Z
				adiw ZH:ZL, 1
				; Low Byte
				lpm CHAR, Z
				mov ZH, TEMP
				mov ZL, CHAR
				adiw YH:YL, 1
				inc CNTR
				cp CNTR, LEN
				brlo ASCII_IO_REG_TO_MEM_ADDR_LOOP

	; Load Stack Pointer
	; Y <- SP
	in YH, SPH
	in YL, SPL
			
	; We have a valid string
	; Get memory address
	; High Byte
	lpm TEMP, Z
	std Y+(STACK_OFFSET + 4), TEMP
	adiw ZH:ZL, 1
	; Low Byte
	lpm TEMP, Z
	std Y+(STACK_OFFSET + 3), TEMP
	; No Error
	ldi TEMP, 0
	std Y+(STACK_OFFSET + 5), TEMP
	
	rjmp ASCII_IO_REG_TO_MEM_ADDR_RET

	ASCII_IO_REG_TO_MEM_ADDR_ERR:
		; Error Status <- True
		ldi TEMP, 1
		std Y+(STACK_OFFSET + 5), TEMP

	ASCII_IO_REG_TO_MEM_ADDR_RET:
		; Restore Registers
		pop ZL
		pop ZH
		pop YL
		pop YH
		pop TEMP
		pop CNTR
		pop CHAR
		pop LEN

		ret

	.undef LEN
	.undef CHAR
	.undef CNTR
	.undef TEMP


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Lookup Tree To Convert ASCII To IO Register Memory Address
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


IO_LOOKUP_TREE:
        .db HIGH(0x00), LOW(0x00)
        .db "A", 0, HIGH(IO_LOOKUP_TREE_A << 1), LOW(IO_LOOKUP_TREE_A << 1)
        .db "D", 0, HIGH(IO_LOOKUP_TREE_D << 1), LOW(IO_LOOKUP_TREE_D << 1)
        .db "E", 0, HIGH(IO_LOOKUP_TREE_E << 1), LOW(IO_LOOKUP_TREE_E << 1)
        .db "G", 0, HIGH(IO_LOOKUP_TREE_G << 1), LOW(IO_LOOKUP_TREE_G << 1)
        .db "I", 0, HIGH(IO_LOOKUP_TREE_I << 1), LOW(IO_LOOKUP_TREE_I << 1)
        .db "M", 0, HIGH(IO_LOOKUP_TREE_M << 1), LOW(IO_LOOKUP_TREE_M << 1)
        .db "O", 0, HIGH(IO_LOOKUP_TREE_O << 1), LOW(IO_LOOKUP_TREE_O << 1)
        .db "P", 0, HIGH(IO_LOOKUP_TREE_P << 1), LOW(IO_LOOKUP_TREE_P << 1)
        .db "S", 0, HIGH(IO_LOOKUP_TREE_S << 1), LOW(IO_LOOKUP_TREE_S << 1)
        .db "T", 0, HIGH(IO_LOOKUP_TREE_T << 1), LOW(IO_LOOKUP_TREE_T << 1)
        .db "U", 0, HIGH(IO_LOOKUP_TREE_U << 1), LOW(IO_LOOKUP_TREE_U << 1)
        .db "W", 0, HIGH(IO_LOOKUP_TREE_W << 1), LOW(IO_LOOKUP_TREE_W << 1)
        .db 0, 0
IO_LOOKUP_TREE_A:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_AC << 1), LOW(IO_LOOKUP_TREE_AC << 1)
        .db "D", 0, HIGH(IO_LOOKUP_TREE_AD << 1), LOW(IO_LOOKUP_TREE_AD << 1)
        .db "S", 0, HIGH(IO_LOOKUP_TREE_AS << 1), LOW(IO_LOOKUP_TREE_AS << 1)
        .db 0, 0
IO_LOOKUP_TREE_AC:
        .db HIGH(0x00), LOW(0x00)
        .db "S", 0, HIGH(IO_LOOKUP_TREE_ACS << 1), LOW(IO_LOOKUP_TREE_ACS << 1)
        .db 0, 0
IO_LOOKUP_TREE_ACS:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_ACSR << 1), LOW(IO_LOOKUP_TREE_ACSR << 1)
        .db 0, 0
IO_LOOKUP_TREE_ACSR:
        .db HIGH(0x28), LOW(0x28)
        .db 0, 0
IO_LOOKUP_TREE_AD:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_ADC << 1), LOW(IO_LOOKUP_TREE_ADC << 1)
        .db "M", 0, HIGH(IO_LOOKUP_TREE_ADM << 1), LOW(IO_LOOKUP_TREE_ADM << 1)
        .db 0, 0
IO_LOOKUP_TREE_ADC:
        .db HIGH(0x00), LOW(0x00)
        .db "H", 0, HIGH(IO_LOOKUP_TREE_ADCH << 1), LOW(IO_LOOKUP_TREE_ADCH << 1)
        .db "L", 0, HIGH(IO_LOOKUP_TREE_ADCL << 1), LOW(IO_LOOKUP_TREE_ADCL << 1)
        .db "S", 0, HIGH(IO_LOOKUP_TREE_ADCS << 1), LOW(IO_LOOKUP_TREE_ADCS << 1)
        .db 0, 0
IO_LOOKUP_TREE_ADCH:
        .db HIGH(0x25), LOW(0x25)
        .db 0, 0
IO_LOOKUP_TREE_ADCL:
        .db HIGH(0x24), LOW(0x24)
        .db 0, 0
IO_LOOKUP_TREE_ADCS:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_ADCSR << 1), LOW(IO_LOOKUP_TREE_ADCSR << 1)
        .db 0, 0
IO_LOOKUP_TREE_ADCSR:
        .db HIGH(0x00), LOW(0x00)
        .db "A", 0, HIGH(IO_LOOKUP_TREE_ADCSRA << 1), LOW(IO_LOOKUP_TREE_ADCSRA << 1)
        .db 0, 0
IO_LOOKUP_TREE_ADCSRA:
        .db HIGH(0x26), LOW(0x26)
        .db 0, 0
IO_LOOKUP_TREE_ADM:
        .db HIGH(0x00), LOW(0x00)
        .db "U", 0, HIGH(IO_LOOKUP_TREE_ADMU << 1), LOW(IO_LOOKUP_TREE_ADMU << 1)
        .db 0, 0
IO_LOOKUP_TREE_ADMU:
        .db HIGH(0x00), LOW(0x00)
        .db "X", 0, HIGH(IO_LOOKUP_TREE_ADMUX << 1), LOW(IO_LOOKUP_TREE_ADMUX << 1)
        .db 0, 0
IO_LOOKUP_TREE_ADMUX:
        .db HIGH(0x27), LOW(0x27)
        .db 0, 0
IO_LOOKUP_TREE_AS:
        .db HIGH(0x00), LOW(0x00)
        .db "S", 0, HIGH(IO_LOOKUP_TREE_ASS << 1), LOW(IO_LOOKUP_TREE_ASS << 1)
        .db 0, 0
IO_LOOKUP_TREE_ASS:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_ASSR << 1), LOW(IO_LOOKUP_TREE_ASSR << 1)
        .db 0, 0
IO_LOOKUP_TREE_ASSR:
        .db HIGH(0x42), LOW(0x42)
        .db 0, 0
IO_LOOKUP_TREE_D:
        .db HIGH(0x00), LOW(0x00)
        .db "D", 0, HIGH(IO_LOOKUP_TREE_DD << 1), LOW(IO_LOOKUP_TREE_DD << 1)
        .db 0, 0
IO_LOOKUP_TREE_DD:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_DDR << 1), LOW(IO_LOOKUP_TREE_DDR << 1)
        .db 0, 0
IO_LOOKUP_TREE_DDR:
        .db HIGH(0x00), LOW(0x00)
        .db "A", 0, HIGH(IO_LOOKUP_TREE_DDRA << 1), LOW(IO_LOOKUP_TREE_DDRA << 1)
        .db "B", 0, HIGH(IO_LOOKUP_TREE_DDRB << 1), LOW(IO_LOOKUP_TREE_DDRB << 1)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_DDRC << 1), LOW(IO_LOOKUP_TREE_DDRC << 1)
        .db "D", 0, HIGH(IO_LOOKUP_TREE_DDRD << 1), LOW(IO_LOOKUP_TREE_DDRD << 1)
        .db 0, 0
IO_LOOKUP_TREE_DDRA:
        .db HIGH(0x3A), LOW(0x3A)
        .db 0, 0
IO_LOOKUP_TREE_DDRB:
        .db HIGH(0x37), LOW(0x37)
        .db 0, 0
IO_LOOKUP_TREE_DDRC:
        .db HIGH(0x34), LOW(0x34)
        .db 0, 0
IO_LOOKUP_TREE_DDRD:
        .db HIGH(0x31), LOW(0x31)
        .db 0, 0
IO_LOOKUP_TREE_E:
        .db HIGH(0x00), LOW(0x00)
        .db "E", 0, HIGH(IO_LOOKUP_TREE_EE << 1), LOW(IO_LOOKUP_TREE_EE << 1)
        .db 0, 0
IO_LOOKUP_TREE_EE:
        .db HIGH(0x00), LOW(0x00)
        .db "A", 0, HIGH(IO_LOOKUP_TREE_EEA << 1), LOW(IO_LOOKUP_TREE_EEA << 1)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_EEC << 1), LOW(IO_LOOKUP_TREE_EEC << 1)
        .db "D", 0, HIGH(IO_LOOKUP_TREE_EED << 1), LOW(IO_LOOKUP_TREE_EED << 1)
        .db 0, 0
IO_LOOKUP_TREE_EEA:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_EEAR << 1), LOW(IO_LOOKUP_TREE_EEAR << 1)
        .db 0, 0
IO_LOOKUP_TREE_EEAR:
        .db HIGH(0x00), LOW(0x00)
        .db "H", 0, HIGH(IO_LOOKUP_TREE_EEARH << 1), LOW(IO_LOOKUP_TREE_EEARH << 1)
        .db "L", 0, HIGH(IO_LOOKUP_TREE_EEARL << 1), LOW(IO_LOOKUP_TREE_EEARL << 1)
        .db 0, 0
IO_LOOKUP_TREE_EEARH:
        .db HIGH(0x3F), LOW(0x3F)
        .db 0, 0
IO_LOOKUP_TREE_EEARL:
        .db HIGH(0x3E), LOW(0x3E)
        .db 0, 0
IO_LOOKUP_TREE_EEC:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_EECR << 1), LOW(IO_LOOKUP_TREE_EECR << 1)
        .db 0, 0
IO_LOOKUP_TREE_EECR:
        .db HIGH(0x3C), LOW(0x3C)
        .db 0, 0
IO_LOOKUP_TREE_EED:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_EEDR << 1), LOW(IO_LOOKUP_TREE_EEDR << 1)
        .db 0, 0
IO_LOOKUP_TREE_EEDR:
        .db HIGH(0x3D), LOW(0x3D)
        .db 0, 0
IO_LOOKUP_TREE_G:
        .db HIGH(0x00), LOW(0x00)
        .db "I", 0, HIGH(IO_LOOKUP_TREE_GI << 1), LOW(IO_LOOKUP_TREE_GI << 1)
        .db 0, 0
IO_LOOKUP_TREE_GI:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_GIC << 1), LOW(IO_LOOKUP_TREE_GIC << 1)
        .db "F", 0, HIGH(IO_LOOKUP_TREE_GIF << 1), LOW(IO_LOOKUP_TREE_GIF << 1)
        .db 0, 0
IO_LOOKUP_TREE_GIC:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_GICR << 1), LOW(IO_LOOKUP_TREE_GICR << 1)
        .db 0, 0
IO_LOOKUP_TREE_GICR:
        .db HIGH(0x5B), LOW(0x5B)
        .db 0, 0
IO_LOOKUP_TREE_GIF:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_GIFR << 1), LOW(IO_LOOKUP_TREE_GIFR << 1)
        .db 0, 0
IO_LOOKUP_TREE_GIFR:
        .db HIGH(0x5A), LOW(0x5A)
        .db 0, 0
IO_LOOKUP_TREE_I:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_IC << 1), LOW(IO_LOOKUP_TREE_IC << 1)
        .db 0, 0
IO_LOOKUP_TREE_IC:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_ICR << 1), LOW(IO_LOOKUP_TREE_ICR << 1)
        .db 0, 0
IO_LOOKUP_TREE_ICR:
        .db HIGH(0x00), LOW(0x00)
        .db "1", 0, HIGH(IO_LOOKUP_TREE_ICR1 << 1), LOW(IO_LOOKUP_TREE_ICR1 << 1)
        .db 0, 0
IO_LOOKUP_TREE_ICR1:
        .db HIGH(0x00), LOW(0x00)
        .db "H", 0, HIGH(IO_LOOKUP_TREE_ICR1H << 1), LOW(IO_LOOKUP_TREE_ICR1H << 1)
        .db "L", 0, HIGH(IO_LOOKUP_TREE_ICR1L << 1), LOW(IO_LOOKUP_TREE_ICR1L << 1)
        .db 0, 0
IO_LOOKUP_TREE_ICR1H:
        .db HIGH(0x47), LOW(0x47)
        .db 0, 0
IO_LOOKUP_TREE_ICR1L:
        .db HIGH(0x46), LOW(0x46)
        .db 0, 0
IO_LOOKUP_TREE_M:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_MC << 1), LOW(IO_LOOKUP_TREE_MC << 1)
        .db 0, 0
IO_LOOKUP_TREE_MC:
        .db HIGH(0x00), LOW(0x00)
        .db "U", 0, HIGH(IO_LOOKUP_TREE_MCU << 1), LOW(IO_LOOKUP_TREE_MCU << 1)
        .db 0, 0
IO_LOOKUP_TREE_MCU:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_MCUC << 1), LOW(IO_LOOKUP_TREE_MCUC << 1)
        .db 0, 0
IO_LOOKUP_TREE_MCUC:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_MCUCR << 1), LOW(IO_LOOKUP_TREE_MCUCR << 1)
        .db "S", 0, HIGH(IO_LOOKUP_TREE_MCUCS << 1), LOW(IO_LOOKUP_TREE_MCUCS << 1)
        .db 0, 0
IO_LOOKUP_TREE_MCUCR:
        .db HIGH(0x55), LOW(0x55)
        .db 0, 0
IO_LOOKUP_TREE_MCUCS:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_MCUCSR << 1), LOW(IO_LOOKUP_TREE_MCUCSR << 1)
        .db 0, 0
IO_LOOKUP_TREE_MCUCSR:
        .db HIGH(0x54), LOW(0x54)
        .db 0, 0
IO_LOOKUP_TREE_O:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_OC << 1), LOW(IO_LOOKUP_TREE_OC << 1)
        .db "S", 0, HIGH(IO_LOOKUP_TREE_OS << 1), LOW(IO_LOOKUP_TREE_OS << 1)
        .db 0, 0
IO_LOOKUP_TREE_OC:
        .db HIGH(0x00), LOW(0x00)
        .db "D", 0, HIGH(IO_LOOKUP_TREE_OCD << 1), LOW(IO_LOOKUP_TREE_OCD << 1)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_OCR << 1), LOW(IO_LOOKUP_TREE_OCR << 1)
        .db 0, 0
IO_LOOKUP_TREE_OCD:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_OCDR << 1), LOW(IO_LOOKUP_TREE_OCDR << 1)
        .db 0, 0
IO_LOOKUP_TREE_OCDR:
        .db HIGH(0x51), LOW(0x51)
        .db 0, 0
IO_LOOKUP_TREE_OCR:
        .db HIGH(0x00), LOW(0x00)
        .db "0", 0, HIGH(IO_LOOKUP_TREE_OCR0 << 1), LOW(IO_LOOKUP_TREE_OCR0 << 1)
        .db "1", 0, HIGH(IO_LOOKUP_TREE_OCR1 << 1), LOW(IO_LOOKUP_TREE_OCR1 << 1)
        .db "2", 0, HIGH(IO_LOOKUP_TREE_OCR2 << 1), LOW(IO_LOOKUP_TREE_OCR2 << 1)
        .db 0, 0
IO_LOOKUP_TREE_OCR0:
        .db HIGH(0x5C), LOW(0x5C)
        .db 0, 0
IO_LOOKUP_TREE_OCR1:
        .db HIGH(0x00), LOW(0x00)
        .db "A", 0, HIGH(IO_LOOKUP_TREE_OCR1A << 1), LOW(IO_LOOKUP_TREE_OCR1A << 1)
        .db "B", 0, HIGH(IO_LOOKUP_TREE_OCR1B << 1), LOW(IO_LOOKUP_TREE_OCR1B << 1)
        .db 0, 0
IO_LOOKUP_TREE_OCR1A:
        .db HIGH(0x00), LOW(0x00)
        .db "H", 0, HIGH(IO_LOOKUP_TREE_OCR1AH << 1), LOW(IO_LOOKUP_TREE_OCR1AH << 1)
        .db "L", 0, HIGH(IO_LOOKUP_TREE_OCR1AL << 1), LOW(IO_LOOKUP_TREE_OCR1AL << 1)
        .db 0, 0
IO_LOOKUP_TREE_OCR1AH:
        .db HIGH(0x4B), LOW(0x4B)
        .db 0, 0
IO_LOOKUP_TREE_OCR1AL:
        .db HIGH(0x4A), LOW(0x4A)
        .db 0, 0
IO_LOOKUP_TREE_OCR1B:
        .db HIGH(0x00), LOW(0x00)
        .db "H", 0, HIGH(IO_LOOKUP_TREE_OCR1BH << 1), LOW(IO_LOOKUP_TREE_OCR1BH << 1)
        .db "L", 0, HIGH(IO_LOOKUP_TREE_OCR1BL << 1), LOW(IO_LOOKUP_TREE_OCR1BL << 1)
        .db 0, 0
IO_LOOKUP_TREE_OCR1BH:
        .db HIGH(0x49), LOW(0x49)
        .db 0, 0
IO_LOOKUP_TREE_OCR1BL:
        .db HIGH(0x48), LOW(0x48)
        .db 0, 0
IO_LOOKUP_TREE_OCR2:
        .db HIGH(0x43), LOW(0x43)
        .db 0, 0
IO_LOOKUP_TREE_OS:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_OSC << 1), LOW(IO_LOOKUP_TREE_OSC << 1)
        .db 0, 0
IO_LOOKUP_TREE_OSC:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_OSCC << 1), LOW(IO_LOOKUP_TREE_OSCC << 1)
        .db 0, 0
IO_LOOKUP_TREE_OSCC:
        .db HIGH(0x00), LOW(0x00)
        .db "A", 0, HIGH(IO_LOOKUP_TREE_OSCCA << 1), LOW(IO_LOOKUP_TREE_OSCCA << 1)
        .db 0, 0
IO_LOOKUP_TREE_OSCCA:
        .db HIGH(0x00), LOW(0x00)
        .db "L", 0, HIGH(IO_LOOKUP_TREE_OSCCAL << 1), LOW(IO_LOOKUP_TREE_OSCCAL << 1)
        .db 0, 0
IO_LOOKUP_TREE_OSCCAL:
        .db HIGH(0x51), LOW(0x51)
        .db 0, 0
IO_LOOKUP_TREE_P:
        .db HIGH(0x00), LOW(0x00)
        .db "I", 0, HIGH(IO_LOOKUP_TREE_PI << 1), LOW(IO_LOOKUP_TREE_PI << 1)
        .db "O", 0, HIGH(IO_LOOKUP_TREE_PO << 1), LOW(IO_LOOKUP_TREE_PO << 1)
        .db 0, 0
IO_LOOKUP_TREE_PI:
        .db HIGH(0x00), LOW(0x00)
        .db "N", 0, HIGH(IO_LOOKUP_TREE_PIN << 1), LOW(IO_LOOKUP_TREE_PIN << 1)
        .db 0, 0
IO_LOOKUP_TREE_PIN:
        .db HIGH(0x00), LOW(0x00)
        .db "A", 0, HIGH(IO_LOOKUP_TREE_PINA << 1), LOW(IO_LOOKUP_TREE_PINA << 1)
        .db "B", 0, HIGH(IO_LOOKUP_TREE_PINB << 1), LOW(IO_LOOKUP_TREE_PINB << 1)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_PINC << 1), LOW(IO_LOOKUP_TREE_PINC << 1)
        .db "D", 0, HIGH(IO_LOOKUP_TREE_PIND << 1), LOW(IO_LOOKUP_TREE_PIND << 1)
        .db 0, 0
IO_LOOKUP_TREE_PINA:
        .db HIGH(0x39), LOW(0x39)
        .db 0, 0
IO_LOOKUP_TREE_PINB:
        .db HIGH(0x36), LOW(0x36)
        .db 0, 0
IO_LOOKUP_TREE_PINC:
        .db HIGH(0x33), LOW(0x33)
        .db 0, 0
IO_LOOKUP_TREE_PIND:
        .db HIGH(0x30), LOW(0x30)
        .db 0, 0
IO_LOOKUP_TREE_PO:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_POR << 1), LOW(IO_LOOKUP_TREE_POR << 1)
        .db 0, 0
IO_LOOKUP_TREE_POR:
        .db HIGH(0x00), LOW(0x00)
        .db "T", 0, HIGH(IO_LOOKUP_TREE_PORT << 1), LOW(IO_LOOKUP_TREE_PORT << 1)
        .db 0, 0
IO_LOOKUP_TREE_PORT:
        .db HIGH(0x00), LOW(0x00)
        .db "A", 0, HIGH(IO_LOOKUP_TREE_PORTA << 1), LOW(IO_LOOKUP_TREE_PORTA << 1)
        .db "B", 0, HIGH(IO_LOOKUP_TREE_PORTB << 1), LOW(IO_LOOKUP_TREE_PORTB << 1)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_PORTC << 1), LOW(IO_LOOKUP_TREE_PORTC << 1)
        .db "D", 0, HIGH(IO_LOOKUP_TREE_PORTD << 1), LOW(IO_LOOKUP_TREE_PORTD << 1)
        .db 0, 0
IO_LOOKUP_TREE_PORTA:
        .db HIGH(0x3B), LOW(0x3B)
        .db 0, 0
IO_LOOKUP_TREE_PORTB:
        .db HIGH(0x38), LOW(0x38)
        .db 0, 0
IO_LOOKUP_TREE_PORTC:
        .db HIGH(0x35), LOW(0x35)
        .db 0, 0
IO_LOOKUP_TREE_PORTD:
        .db HIGH(0x32), LOW(0x32)
        .db 0, 0
IO_LOOKUP_TREE_S:
        .db HIGH(0x00), LOW(0x00)
        .db "F", 0, HIGH(IO_LOOKUP_TREE_SF << 1), LOW(IO_LOOKUP_TREE_SF << 1)
        .db "P", 0, HIGH(IO_LOOKUP_TREE_SP << 1), LOW(IO_LOOKUP_TREE_SP << 1)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_SR << 1), LOW(IO_LOOKUP_TREE_SR << 1)
        .db 0, 0
IO_LOOKUP_TREE_SF:
        .db HIGH(0x00), LOW(0x00)
        .db "I", 0, HIGH(IO_LOOKUP_TREE_SFI << 1), LOW(IO_LOOKUP_TREE_SFI << 1)
        .db 0, 0
IO_LOOKUP_TREE_SFI:
        .db HIGH(0x00), LOW(0x00)
        .db "O", 0, HIGH(IO_LOOKUP_TREE_SFIO << 1), LOW(IO_LOOKUP_TREE_SFIO << 1)
        .db 0, 0
IO_LOOKUP_TREE_SFIO:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_SFIOR << 1), LOW(IO_LOOKUP_TREE_SFIOR << 1)
        .db 0, 0
IO_LOOKUP_TREE_SFIOR:
        .db HIGH(0x50), LOW(0x50)
        .db 0, 0
IO_LOOKUP_TREE_SP:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_SPC << 1), LOW(IO_LOOKUP_TREE_SPC << 1)
        .db "D", 0, HIGH(IO_LOOKUP_TREE_SPD << 1), LOW(IO_LOOKUP_TREE_SPD << 1)
        .db "H", 0, HIGH(IO_LOOKUP_TREE_SPH << 1), LOW(IO_LOOKUP_TREE_SPH << 1)
        .db "L", 0, HIGH(IO_LOOKUP_TREE_SPL << 1), LOW(IO_LOOKUP_TREE_SPL << 1)
        .db "M", 0, HIGH(IO_LOOKUP_TREE_SPM << 1), LOW(IO_LOOKUP_TREE_SPM << 1)
        .db "S", 0, HIGH(IO_LOOKUP_TREE_SPS << 1), LOW(IO_LOOKUP_TREE_SPS << 1)
        .db 0, 0
IO_LOOKUP_TREE_SPC:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_SPCR << 1), LOW(IO_LOOKUP_TREE_SPCR << 1)
        .db 0, 0
IO_LOOKUP_TREE_SPCR:
        .db HIGH(0x2D), LOW(0x2D)
        .db 0, 0
IO_LOOKUP_TREE_SPD:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_SPDR << 1), LOW(IO_LOOKUP_TREE_SPDR << 1)
        .db 0, 0
IO_LOOKUP_TREE_SPDR:
        .db HIGH(0x2F), LOW(0x2F)
        .db 0, 0
IO_LOOKUP_TREE_SPH:
        .db HIGH(0x5E), LOW(0x5E)
        .db 0, 0
IO_LOOKUP_TREE_SPL:
        .db HIGH(0x5D), LOW(0x5D)
        .db 0, 0
IO_LOOKUP_TREE_SPM:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_SPMC << 1), LOW(IO_LOOKUP_TREE_SPMC << 1)
        .db 0, 0
IO_LOOKUP_TREE_SPMC:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_SPMCR << 1), LOW(IO_LOOKUP_TREE_SPMCR << 1)
        .db 0, 0
IO_LOOKUP_TREE_SPMCR:
        .db HIGH(0x57), LOW(0x57)
        .db 0, 0
IO_LOOKUP_TREE_SPS:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_SPSR << 1), LOW(IO_LOOKUP_TREE_SPSR << 1)
        .db 0, 0
IO_LOOKUP_TREE_SPSR:
        .db HIGH(0x2E), LOW(0x2E)
        .db 0, 0
IO_LOOKUP_TREE_SR:
        .db HIGH(0x00), LOW(0x00)
        .db "E", 0, HIGH(IO_LOOKUP_TREE_SRE << 1), LOW(IO_LOOKUP_TREE_SRE << 1)
        .db 0, 0
IO_LOOKUP_TREE_SRE:
        .db HIGH(0x00), LOW(0x00)
        .db "G", 0, HIGH(IO_LOOKUP_TREE_SREG << 1), LOW(IO_LOOKUP_TREE_SREG << 1)
        .db 0, 0
IO_LOOKUP_TREE_SREG:
        .db HIGH(0x5F), LOW(0x5F)
        .db 0, 0
IO_LOOKUP_TREE_T:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_TC << 1), LOW(IO_LOOKUP_TREE_TC << 1)
        .db "I", 0, HIGH(IO_LOOKUP_TREE_TI << 1), LOW(IO_LOOKUP_TREE_TI << 1)
        .db "W", 0, HIGH(IO_LOOKUP_TREE_TW << 1), LOW(IO_LOOKUP_TREE_TW << 1)
        .db 0, 0
IO_LOOKUP_TREE_TC:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_TCC << 1), LOW(IO_LOOKUP_TREE_TCC << 1)
        .db "N", 0, HIGH(IO_LOOKUP_TREE_TCN << 1), LOW(IO_LOOKUP_TREE_TCN << 1)
        .db 0, 0
IO_LOOKUP_TREE_TCC:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_TCCR << 1), LOW(IO_LOOKUP_TREE_TCCR << 1)
        .db 0, 0
IO_LOOKUP_TREE_TCCR:
        .db HIGH(0x00), LOW(0x00)
        .db "0", 0, HIGH(IO_LOOKUP_TREE_TCCR0 << 1), LOW(IO_LOOKUP_TREE_TCCR0 << 1)
        .db "1", 0, HIGH(IO_LOOKUP_TREE_TCCR1 << 1), LOW(IO_LOOKUP_TREE_TCCR1 << 1)
        .db "2", 0, HIGH(IO_LOOKUP_TREE_TCCR2 << 1), LOW(IO_LOOKUP_TREE_TCCR2 << 1)
        .db 0, 0
IO_LOOKUP_TREE_TCCR0:
        .db HIGH(0x53), LOW(0x53)
        .db 0, 0
IO_LOOKUP_TREE_TCCR1:
        .db HIGH(0x00), LOW(0x00)
        .db "A", 0, HIGH(IO_LOOKUP_TREE_TCCR1A << 1), LOW(IO_LOOKUP_TREE_TCCR1A << 1)
        .db "B", 0, HIGH(IO_LOOKUP_TREE_TCCR1B << 1), LOW(IO_LOOKUP_TREE_TCCR1B << 1)
        .db 0, 0
IO_LOOKUP_TREE_TCCR1A:
        .db HIGH(0x4F), LOW(0x4F)
        .db 0, 0
IO_LOOKUP_TREE_TCCR1B:
        .db HIGH(0x4E), LOW(0x4E)
        .db 0, 0
IO_LOOKUP_TREE_TCCR2:
        .db HIGH(0x45), LOW(0x45)
        .db 0, 0
IO_LOOKUP_TREE_TCN:
        .db HIGH(0x00), LOW(0x00)
        .db "T", 0, HIGH(IO_LOOKUP_TREE_TCNT << 1), LOW(IO_LOOKUP_TREE_TCNT << 1)
        .db 0, 0
IO_LOOKUP_TREE_TCNT:
        .db HIGH(0x00), LOW(0x00)
        .db "0", 0, HIGH(IO_LOOKUP_TREE_TCNT0 << 1), LOW(IO_LOOKUP_TREE_TCNT0 << 1)
        .db "1", 0, HIGH(IO_LOOKUP_TREE_TCNT1 << 1), LOW(IO_LOOKUP_TREE_TCNT1 << 1)
        .db "2", 0, HIGH(IO_LOOKUP_TREE_TCNT2 << 1), LOW(IO_LOOKUP_TREE_TCNT2 << 1)
        .db 0, 0
IO_LOOKUP_TREE_TCNT0:
        .db HIGH(0x52), LOW(0x52)
        .db 0, 0
IO_LOOKUP_TREE_TCNT1:
        .db HIGH(0x00), LOW(0x00)
        .db "H", 0, HIGH(IO_LOOKUP_TREE_TCNT1H << 1), LOW(IO_LOOKUP_TREE_TCNT1H << 1)
        .db "L", 0, HIGH(IO_LOOKUP_TREE_TCNT1L << 1), LOW(IO_LOOKUP_TREE_TCNT1L << 1)
        .db 0, 0
IO_LOOKUP_TREE_TCNT1H:
        .db HIGH(0x4D), LOW(0x4D)
        .db 0, 0
IO_LOOKUP_TREE_TCNT1L:
        .db HIGH(0x4C), LOW(0x4C)
        .db 0, 0
IO_LOOKUP_TREE_TCNT2:
        .db HIGH(0x44), LOW(0x44)
        .db 0, 0
IO_LOOKUP_TREE_TI:
        .db HIGH(0x00), LOW(0x00)
        .db "F", 0, HIGH(IO_LOOKUP_TREE_TIF << 1), LOW(IO_LOOKUP_TREE_TIF << 1)
        .db "M", 0, HIGH(IO_LOOKUP_TREE_TIM << 1), LOW(IO_LOOKUP_TREE_TIM << 1)
        .db 0, 0
IO_LOOKUP_TREE_TIF:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_TIFR << 1), LOW(IO_LOOKUP_TREE_TIFR << 1)
        .db 0, 0
IO_LOOKUP_TREE_TIFR:
        .db HIGH(0x58), LOW(0x58)
        .db 0, 0
IO_LOOKUP_TREE_TIM:
        .db HIGH(0x00), LOW(0x00)
        .db "S", 0, HIGH(IO_LOOKUP_TREE_TIMS << 1), LOW(IO_LOOKUP_TREE_TIMS << 1)
        .db 0, 0
IO_LOOKUP_TREE_TIMS:
        .db HIGH(0x00), LOW(0x00)
        .db "K", 0, HIGH(IO_LOOKUP_TREE_TIMSK << 1), LOW(IO_LOOKUP_TREE_TIMSK << 1)
        .db 0, 0
IO_LOOKUP_TREE_TIMSK:
        .db HIGH(0x59), LOW(0x59)
        .db 0, 0
IO_LOOKUP_TREE_TW:
        .db HIGH(0x00), LOW(0x00)
        .db "A", 0, HIGH(IO_LOOKUP_TREE_TWA << 1), LOW(IO_LOOKUP_TREE_TWA << 1)
        .db "B", 0, HIGH(IO_LOOKUP_TREE_TWB << 1), LOW(IO_LOOKUP_TREE_TWB << 1)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_TWC << 1), LOW(IO_LOOKUP_TREE_TWC << 1)
        .db "D", 0, HIGH(IO_LOOKUP_TREE_TWD << 1), LOW(IO_LOOKUP_TREE_TWD << 1)
        .db "S", 0, HIGH(IO_LOOKUP_TREE_TWS << 1), LOW(IO_LOOKUP_TREE_TWS << 1)
        .db 0, 0
IO_LOOKUP_TREE_TWA:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_TWAR << 1), LOW(IO_LOOKUP_TREE_TWAR << 1)
        .db 0, 0
IO_LOOKUP_TREE_TWAR:
        .db HIGH(0x22), LOW(0x22)
        .db 0, 0
IO_LOOKUP_TREE_TWB:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_TWBR << 1), LOW(IO_LOOKUP_TREE_TWBR << 1)
        .db 0, 0
IO_LOOKUP_TREE_TWBR:
        .db HIGH(0x20), LOW(0x20)
        .db 0, 0
IO_LOOKUP_TREE_TWC:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_TWCR << 1), LOW(IO_LOOKUP_TREE_TWCR << 1)
        .db 0, 0
IO_LOOKUP_TREE_TWCR:
        .db HIGH(0x56), LOW(0x56)
        .db 0, 0
IO_LOOKUP_TREE_TWD:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_TWDR << 1), LOW(IO_LOOKUP_TREE_TWDR << 1)
        .db 0, 0
IO_LOOKUP_TREE_TWDR:
        .db HIGH(0x23), LOW(0x23)
        .db 0, 0
IO_LOOKUP_TREE_TWS:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_TWSR << 1), LOW(IO_LOOKUP_TREE_TWSR << 1)
        .db 0, 0
IO_LOOKUP_TREE_TWSR:
        .db HIGH(0x21), LOW(0x21)
        .db 0, 0
IO_LOOKUP_TREE_U:
        .db HIGH(0x00), LOW(0x00)
        .db "B", 0, HIGH(IO_LOOKUP_TREE_UB << 1), LOW(IO_LOOKUP_TREE_UB << 1)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_UC << 1), LOW(IO_LOOKUP_TREE_UC << 1)
        .db "D", 0, HIGH(IO_LOOKUP_TREE_UD << 1), LOW(IO_LOOKUP_TREE_UD << 1)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_UR << 1), LOW(IO_LOOKUP_TREE_UR << 1)
        .db 0, 0
IO_LOOKUP_TREE_UB:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_UBR << 1), LOW(IO_LOOKUP_TREE_UBR << 1)
        .db 0, 0
IO_LOOKUP_TREE_UBR:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_UBRR << 1), LOW(IO_LOOKUP_TREE_UBRR << 1)
        .db 0, 0
IO_LOOKUP_TREE_UBRR:
        .db HIGH(0x00), LOW(0x00)
        .db "H", 0, HIGH(IO_LOOKUP_TREE_UBRRH << 1), LOW(IO_LOOKUP_TREE_UBRRH << 1)
        .db "L", 0, HIGH(IO_LOOKUP_TREE_UBRRL << 1), LOW(IO_LOOKUP_TREE_UBRRL << 1)
        .db 0, 0
IO_LOOKUP_TREE_UBRRH:
        .db HIGH(0x40), LOW(0x40)
        .db 0, 0
IO_LOOKUP_TREE_UBRRL:
        .db HIGH(0x29), LOW(0x29)
        .db 0, 0
IO_LOOKUP_TREE_UC:
        .db HIGH(0x00), LOW(0x00)
        .db "S", 0, HIGH(IO_LOOKUP_TREE_UCS << 1), LOW(IO_LOOKUP_TREE_UCS << 1)
        .db 0, 0
IO_LOOKUP_TREE_UCS:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_UCSR << 1), LOW(IO_LOOKUP_TREE_UCSR << 1)
        .db 0, 0
IO_LOOKUP_TREE_UCSR:
        .db HIGH(0x00), LOW(0x00)
        .db "A", 0, HIGH(IO_LOOKUP_TREE_UCSRA << 1), LOW(IO_LOOKUP_TREE_UCSRA << 1)
        .db "B", 0, HIGH(IO_LOOKUP_TREE_UCSRB << 1), LOW(IO_LOOKUP_TREE_UCSRB << 1)
        .db 0, 0
IO_LOOKUP_TREE_UCSRA:
        .db HIGH(0x2B), LOW(0x2B)
        .db 0, 0
IO_LOOKUP_TREE_UCSRB:
        .db HIGH(0x2A), LOW(0x2A)
        .db 0, 0
IO_LOOKUP_TREE_UD:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_UDR << 1), LOW(IO_LOOKUP_TREE_UDR << 1)
        .db 0, 0
IO_LOOKUP_TREE_UDR:
        .db HIGH(0x2C), LOW(0x2C)
        .db 0, 0
IO_LOOKUP_TREE_UR:
        .db HIGH(0x00), LOW(0x00)
        .db "S", 0, HIGH(IO_LOOKUP_TREE_URS << 1), LOW(IO_LOOKUP_TREE_URS << 1)
        .db 0, 0
IO_LOOKUP_TREE_URS:
        .db HIGH(0x00), LOW(0x00)
        .db "E", 0, HIGH(IO_LOOKUP_TREE_URSE << 1), LOW(IO_LOOKUP_TREE_URSE << 1)
        .db 0, 0
IO_LOOKUP_TREE_URSE:
        .db HIGH(0x00), LOW(0x00)
        .db "L", 0, HIGH(IO_LOOKUP_TREE_URSEL << 1), LOW(IO_LOOKUP_TREE_URSEL << 1)
        .db 0, 0
IO_LOOKUP_TREE_URSEL:
        .db HIGH(0x40), LOW(0x40)
        .db 0, 0
IO_LOOKUP_TREE_W:
        .db HIGH(0x00), LOW(0x00)
        .db "D", 0, HIGH(IO_LOOKUP_TREE_WD << 1), LOW(IO_LOOKUP_TREE_WD << 1)
        .db 0, 0
IO_LOOKUP_TREE_WD:
        .db HIGH(0x00), LOW(0x00)
        .db "T", 0, HIGH(IO_LOOKUP_TREE_WDT << 1), LOW(IO_LOOKUP_TREE_WDT << 1)
        .db 0, 0
IO_LOOKUP_TREE_WDT:
        .db HIGH(0x00), LOW(0x00)
        .db "C", 0, HIGH(IO_LOOKUP_TREE_WDTC << 1), LOW(IO_LOOKUP_TREE_WDTC << 1)
        .db 0, 0
IO_LOOKUP_TREE_WDTC:
        .db HIGH(0x00), LOW(0x00)
        .db "R", 0, HIGH(IO_LOOKUP_TREE_WDTCR << 1), LOW(IO_LOOKUP_TREE_WDTCR << 1)
        .db 0, 0
IO_LOOKUP_TREE_WDTCR:
        .db HIGH(0x41), LOW(0x41)
        .db 0, 0