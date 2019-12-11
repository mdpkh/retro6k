; Retro6K System Firmware
; special system addresses
VIDEOSTATE = $02FC
FVMCDEST = $03F8
FVMCSRC = $03F9
; code starts here
  .org $F000
  NOP
  NOP
  NOP
  NOP
entry: ; system power-on or reset
  ; misc initialization
  SEI
  CLD
  LDA #$00
  STA $03F8
  JSR waitvscan
  ; display cartridge identification in bottom of screen
  LDA #$20
  STA $0A0A
  LDA $2000
  STA $0A0B
  LDA $2001
  STA $0A0C
  LDA $2002
  STA $0A0D
  LDA $2003
  STA $0A0E
  LDA $2004
  STA $0A0F
  LDA $2005
  STA $0A10
  LDA $2006
  STA $0A11
  LDA $2007
  STA $0A12
  LDA $2008
  STA $0A13
  LDA $2009
  STA $0A14
  LDA #$20
  STA $0A15
  LDX #$00
cartidcolloop:
  LDA #$0F
  STA $0B2A,X
  STA $0C4A,X
  STA $0D6A,X
  LDA #$05
  STA $0E8A,X
  STA $0FAA,X
  LDA #$35
  STA $0B4A,X
  STA $0C6A,X
  STA $0D8A,X
  LDA #$0A
  STA $0EAA,X
  STA $0FCA,X
  LDA #$20
  STA $09EA,X
  STA $0A2A,X
  INX
  TXA
  CMP #$0C
  BNE cartidcolloop
  LDA #$00
  STA $0FE0
  STA $0FE1
  STA $0FE2
  STA $0FE3
  STA $0FE4
  STA $0FE5
  STA $0FE6
  STA $0FE8
  STA $0FE9
  STA $0FEA
  STA $0FEB
  STA $0FEC
  STA $0FED
  STA $0FEE
  STA $0FEF
  STA $0FF0
  STA $0FF1
  STA $0FF2
  STA $0FF3
  STA $0FF4
  STA $0FF5
  STA $0FF6
  STA $0FF7
  STA $0FF9
  STA $0FFA
  STA $0FFB
  STA $0FFC
  STA $0FFD
  STA $0FFE
  LDA #$0F
  STA $0FE7
  LDA #$F0
  STA $0FF8
  LDA #$FF
  STA $0FFF
  ; detect presence of valid cartridge by checking for magic bytes 47 A9 0C 1E
  LDA $200A
  CMP #$47
  BNE nocart
  LDA $200B
  CMP #$A9
  BNE nocart
  LDA $200C
  CMP #$0C
  BNE nocart
  LDA $200D
  CMP #$1E
  BNE nocart
  ; if we got this far, jump to cartridge start vector
  JMP ($200E)
nocart: ; no cartridge inserted
  ; copy system font
  LDY #$E0 ; system font source page start
  LDA #$10 ; video RAM font page start
copyfontpageloop:
  JSR waitvscan ; wait for vertical retrace (preserve A & Y registers please)
  ; enable fast video memory copy
  CLC
  STA FVMCDEST
  STY FVMCSRC
  LDX #$00
copyfontbyteloop:
  STA $00,X ; actual address and value written is overridden by fast video memory copy circuit
  INX
  STA $00,X
  INX
  STA $00,X
  INX
  STA $00,X
  INX
  BNE copyfontbyteloop
  ; disable fast video memory copy
  ; X should be zero
  STX FVMCDEST
  INY ; increment source page
  CLC
  ADC #$01 ; increment destination page
  CMP #$20 ; compare against stop page number
  BNE copyfontpageloop
  JSR waitvscan ; wait for vertical retrace
  ; initialize screen display
  LDX #$00
initscreenloop:
  LDA datasystext0,X ; screen lines 0&1: system startup text
  STA $0800,X
  LDA datasystext1,X ; screen lines 2&3: system startup text
  STA $0840,X
  LDA datasystext2,X ; screen lines 4&5: system startup text
  STA $0880,X
  LDA datasystext3,X ; screen lines 6&7: system startup text
  STA $08C0,X
  LDA #$20 ; screen lines 8-15: blank
  STA $0900,X
  STA $0940,X
  STA $0980,X
  STA $09C0,X
  LDA #$96 ; screen lines 16&17: shade blocks
  STA $0A00,X
  LDA #$CF ; screen lines 0-15 attribute bit 0: 0/1 0/1 1/1 1/1
  STA $0A40,X
  STA $0A80,X
  STA $0AC0,X
  STA $0B00,X
  LDA #$CC ; screen lines 16&17 attribute bit 0, sl 0&1 ab 1: 0/0 0/0 1/1 1/1
  STA $0B40,X
  ; screen lines 2-17 attribute bit 1: 0/0 0/0 1/1 1/1
  STA $0B80,X
  STA $0BC0,X
  STA $0C00,X
  STA $0C40,X
  ; screen lines 0-15 attribute bit 2: 0/0 0/0 1/1 1/1
  STA $0C80,X
  STA $0CC0,X
  STA $0D00,X
  STA $0D40,X
  ; screen lines 16&17 attribute bit 2, sl 0-5 ab 3: 0/0 0/0 1/1 1/1
  STA $0D80,X
  STA $0DC0,X
  LDA #$00 ; screen lines 6-17 attribute bit 3: 0/0 0/0 0/0 0/0
  STA $0E00,X
  STA $0E40,X
  STA $0E80,X
  LDA #$CC ; screen lines 0-3 attribute bit 4: 0/0 0/0 1/1 1/1
  STA $0EC0,X
  LDA #$C0 ; screen lines 4-7 attribute bit 4: 0/0 0/0 1/0 1/0
  STA $0F00,X
  LDA #$00 ; screen lines 8-15 attribute bit 4: 0/0 0/0 0/0 0/0
  STA $0F40,X
  STA $0F80,X
  LDA datasyscolors,X ; sl 16&17 ab 4, palette colors
  STA $0FC0,X
  INX
  TXA
  AND #$07
  CMP #$00
  BNE initscreenloop
  TXA
  JSR waitvscan
  CMP #$40
  BNE initscreenloop
halt:
  JMP halt

waitvscan:
; Subroutine: Wait for start of vertical blanking interval.
; Side effects: X register is clobbered.
  TAX
waitvscanloop:
  LDA VIDEOSTATE
  AND #$30
  CMP #$20
  BNE waitvscanloop
  TXA
  RTS

bytetohex:
; Subroutine: Convert value in Accumulator to uppercase hexadecimal.
; Side effects: result stored in zp$86,$87, neg digit count stored in X
  TAX
  AND #$0F ; extract low nybble
  ADC #$30 ; convert to numeric character
  CMP #$3A
  BMI bytetohexb1
  ADC #$07 ; if low nybble > 9, convert to letter
bytetohexb1:
  STA $87 ; write low nybble character
  TXA
  AND #$F0 ; extract high nybble
  BEQ bytetohexb3 ; if high nybble is zero, jump
  LSR
  LSR
  LSR
  LSR ; shift nybble into place
  ADC #$30 ; convert to numeric character
  CMP #$3A
  BMI bytetohexb2
  ADC #$07 ; if high nybble > 9, convert to letter
bytetohexb2:
  STA $86 ; write high nybble character
  TXA ; restore Accumulator
  LDX #$FE ; set X to -2 (2 digits)
  RTS
bytetohexb3:
  LDA #$30
  STA $86 ; write high zero
  TXA ; restore Accumulator
  LDX #$FF ; set X to -1 (1 digit)
  RTS

bytetodecb8:
  EOR #$FF ; find negative number
  CLC
  ADC #$01
  BCS bytetodecb10 ; negative zero? write literal "-256" instead
  STA $8F ; store negative number in zeropage
  LDA #$60 ; load minus sign for sign column
  JMP bytetodecj9
bytetodecb10:
  STA $8C ; output - (sign column)
  LDA #$32
  STA $8D ; output 2 (hundreds column)
  LDA #$35
  STA $8E ; output 5 (hundreds column)
  LDA #$36
  STA $8F ; output 6 (hundreds column)
  LDA #$00 ; restore Accumulator
  LDX #$FD ; set X to -3 (3 digits)
  RTS
bytetodec:
; Subroutine: Convert value in Accumulator to decimal. If V flag set, display as negative number.
; Side effects: result stored in zp$8C,$8D,$8E,$8F, neg digit count stored in X, accumulator clobbered
  BVS bytetodecb8 ; set up for negative number
  STA $8F ; place input in zeropage (temporarily using units digit output byte)
  LDA #$20 ; load space for sign column
bytetodecj9: ; come from negative number setup
  STA $8C ; output sign column
  SED
  LDA #$00 ; initialize units and tens digits in accumulator
  STA $8D ; initialize hundreds digit in output position
  LSR $8F ; shift input
  BCC bytetodecb0
  CLC
  ADC #$01 ; add bit value
bytetodecb0:
  LSR $8F ; shift input
  BCC bytetodecb1
  CLC
  ADC #$02 ; add bit value
bytetodecb1:
  LSR $8F ; shift input
  BCC bytetodecb2
  CLC
  ADC #$04 ; add bit value
bytetodecb2:
  LSR $8F ; shift input
  BCC bytetodecb3
  CLC
  ADC #$08 ; add bit value
bytetodecb3:
  LSR $8F ; shift input
  BCC bytetodecb4
  CLC
  ADC #$16 ; add bit value
bytetodecb4:
  LSR $8F ; shift input
  BCC bytetodecb5
  CLC
  ADC #$32 ; add bit value
bytetodecb5:
  LSR $8F ; shift input
  BCC bytetodecb6
  CLC
  ADC #$64 ; add bit value
  BCC bytetodecb6
  CLC
  INC $8D ; increment hundreds digit if this bit caused a carry
bytetodecb6:
  LSR $8F ; shift input
  BCC bytetodecb7
  CLC
  INC $8D ; increment hundreds digit
  ADC #$28 ; add 28
  BCC bytetodecb7
  CLC
  INC $8D ; increment hundreds digit if the 28 caused another carry
bytetodecb7:
  CLD
  LDX $8D ; is hundreds digit zero?
  BNE bytetodecb13
  LDX #$30
  STX $8D ; output numeral zero in hundreds column
  TAX ; move units and tens digits out of the way
  AND #$F0 ; retrieve tens digit; is it zero?
  BNE bytetodecb12
  TXA
  LDX #$30
  STX $8E ; output numeral zero in tens column
  AND #$0F ; retrieve units digit
  ADC #$30 ; convert to numeral
  STA $8F ; output units digit
  LDX #$FF ; set X to -1 (1 digit)
  RTS
bytetodecb12:
  LSR
  LSR
  LSR
  LSR ; shift digit into place
  ADC #$30 ; convert to numeral
  STA $8E ; output tens digit
  TXA
  AND #$0F ; retrieve units digit
  ADC #$30 ; convert to numeral
  STA $8F ; output units digit
  LDX #$FE ; set X to -2 (2 digits)
  RTS
bytetodecb13:
  TAX ; move units and tens digits out of the way
  LDA $8D ; retrieve hundreds digit
  ADC #$30 ; convert to numeral
  STA $8D ; output hundreds digit
  TXA
  AND #$F0 ; retrieve tens digit
  LSR
  LSR
  LSR
  LSR ; shift digit into place
  ADC #$30 ; convert to numeral
  STA $8E ; output tens digit
  TXA
  AND #$0F ; retrieve units digit
  ADC #$30 ; convert to numeral
  STA $8F ; output units digit
  LDX #$FD ; set X to -3 (3 digits)
  RTS

nmicode: ; empty, because NMI not presently used
  RTI

  .align 8
datasystext0:
  .text "\056 \256  \256  \056  \256  \256  \256  \256  \056  \256  \256 \056"
  .text "                                "
datasystext1:
  .text "\256  Retro 6K Fantasy Computer\021  \256"
  .text "                                "
datasystext2:
  .text "\056 \256  \256  \056  \256  \256  \256  \256  \056  \256  \256 \056"
  .text "   Please insert a cartridge>   "
datasystext3:
  .text "     Firmware version 1985b     "
  .text " \2742019 Maggie David P>K> Haynes "
defaultpalpage:
  .text "This block  of text  is filler> "
  .text "There  are one  hundred  ninety`"
  .text "two  bytes of  filler  here  so "
  .text "that the system default palette "
  .text "will occupy positions  E0-FF of "
  .text "a code page<  facilitating FVMC>"
datasyscolors:
  .text " Barack Obama says trans rights!"
  .byte $00,$81,$84,$0D,$02,$83,$86,$0F
  .byte $D0,$59,$5C,$DD,$D2,$5B,$5E,$DF
  .byte $20,$A1,$A4,$2D,$22,$A3,$A6,$2F
  .byte $F0,$79,$7C,$FD,$F2,$7B,$7E,$FF
  
; Utility subroutine addresses
  .org $FF80
  .word waitvscan ; ($FF80)
  .word bytetohex ; ($FF82)
  .word bytetodec ; ($FF84)

; 6502 program counter initialization vectors
  .org $FFFA
  .word nmicode ; NMI vector, in firmware
  .word entry ; reset vector, in firmware
  .word $2010 ; interrupt request vector, in cartridge