	.module spi_text
	.optsdcc -mmcs51 --model-small
	

	.globl _SPI_receive
	.globl _SPI_transfer
	.globl _SPI_send_bit
	.globl _SPI_disable
	.globl _SPI_enable
	.globl _SPI_init
	
	.globl _P1_7
	.globl _P1_6
	.globl _P1_5
	.globl _P1_4

_P1_4	=	0x0094
_P1_5	=	0x0095
_P1_6	=	0x0096
_P1_7	=	0x0097

;--------------------------------------------------------
; overlayable register banks
;--------------------------------------------------------
	.area REG_BANK_0	(REL,OVR,DATA)
	.ds 8
;--------------------------------------------------------
; internal ram data
;--------------------------------------------------------
	.area DSEG    (DATA)
;--------------------------------------------------------
; overlayable items in internal ram 
;--------------------------------------------------------
	.area	OSEG    (OVR,DATA)
;--------------------------------------------------------

;--------------------------------------------------------
; indirectly addressable internal ram data
;--------------------------------------------------------
	.area ISEG    (DATA)
;--------------------------------------------------------
; absolute internal ram data
;--------------------------------------------------------
	.area IABS    (ABS,DATA)
	.area IABS    (ABS,DATA)
;--------------------------------------------------------
; bit data
;--------------------------------------------------------
	.area BSEG    (BIT)
;--------------------------------------------------------
; paged external ram data
;--------------------------------------------------------
	.area PSEG    (PAG,XDATA)
;--------------------------------------------------------
; external ram data
;--------------------------------------------------------
	.area XSEG    (XDATA)
_SPI_receive_received_data_1_10:
	.ds 1
;--------------------------------------------------------
; global & static initialisations
;--------------------------------------------------------
	.area HOME    (CODE)
	.area GSINIT  (CODE)
	.area GSFINAL (CODE)
	.area GSINIT  (CODE)
	.globl __sdcc_gsinit_startup
	.globl __sdcc_program_startup
	.globl __start__stack
	.globl __mcs51_genXINIT
	.globl __mcs51_genXRAMCLEAR
	.globl __mcs51_genRAMCLEAR
	.area GSFINAL (CODE)
	ljmp	__sdcc_program_startup
;--------------------------------------------------------
; Home
;--------------------------------------------------------
	.area HOME    (CODE)
	.area HOME    (CODE)
;--------------------------------------------------------
; code
;--------------------------------------------------------
	.area CSEG    (CODE)
;------------------------------------------------------------
;Allocation info for local variables in function 'SPI_init'
;------------------------------------------------------------
;	spi_text.c:7: void SPI_init()
;	-----------------------------------------
;	 function SPI_init
;	-----------------------------------------
_SPI_init:
	ar7 = 0x07
	ar6 = 0x06
	ar5 = 0x05
	ar4 = 0x04
	ar3 = 0x03
	ar2 = 0x02
	ar1 = 0x01
	ar0 = 0x00
	SETB	_P1_5
	SETB	_P1_7
	SETB	_P1_4
	RET

_SPI_enable:
	CLR	_P1_4
	CLR	_P1_7
	RET

_SPI_disable:
	SETB	_P1_4
	SETB	_P1_7
	RET

_SPI_send_bit: 
	MOV	A,DPL
	ADD	A,#0xff
	MOV	_P1_5,c   
	SETB _P1_7  
	CLR _P1_7
	RET

_SPI_transfer:
    MOV     R7, DPL                ; Load data into R7
    MOV     R6, #0x00              ; Initialize loop counter to 0

SPI_transfer_loop:
    MOV     A, R7                  ; Load data byte into accumulator
    JNB     ACC.7, send_zero       ; Check MSB (A.7), if 0, jump to send_zero
    MOV     R5, #0x01              ; If MSB is 1, set R5 to 1
    SJMP    send_bit               

send_zero:
    MOV     R5, #0x00              

send_bit:
    MOV     DPL, R5                ; Prepare the bit in DPL for _SPI_send_bit
    ACALL   _SPI_send_bit          ; Call _SPI_send_bit with the MSB

    MOV     A, R7                  ; Load data byte into accumulator
    RL      A                      ; Rotate left (shift left) by one bit
    MOV     R7, A                  ; Store shifted data back into R7

    INC     R6                    
    CJNE    R6, #0x08, SPI_transfer_loop 

    RET                             

_SPI_receive:
    MOV     A, #0x00          ; Clear accumulator (to store received data)
    MOV     R7, #8            ; Set loop counter for 8 bits

Receive_Loop:
    CLR     C                 ; Clear carry to prepare for next bit
    JB      _P1_6, SetBit     ; If MISO is high, set carry

SkipSetBit:
    SJMP    Continue          ; No operation here if MISO is low; carry remains clear

SetBit:
    SETB    C                 ; Set carry if MISO is high

Continue:
    RLC     A                 ; Rotate left through carry (builds received byte)

    SETB    _P1_7              ; Set SCLK high
    CLR     _P1_7             ; Set SCLK low

    DJNZ    R7, Receive_Loop  ; Decrement counter and loop until 8 bits are done

    MOV     DPL, A            ; Move received byte to DPL for return
    RET
