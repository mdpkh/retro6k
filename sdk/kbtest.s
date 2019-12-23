VIDEOSTATE = $02FC
FVMCDEST = $03F8
FVMCSRC = $03F9
BYTETODEC = $F259
WAITVSCAN = $F1D7
  .org $1F4C ; this may need to be adjusted to allow room for header
header:
  .text "\x47\xA9\x02\x6A\xBB\x47\xF3\xA7" ; cartridge file format 1986c
  .word pflags-header ; file offset (LE) to base address space page flags
  .word $0000 ; upper bytes of 4-byte number
  .word cartromstart-header ; file offset (LE) to base address space data start
  .word $0000 ; upper bytes of 4-byte number
extensions:
  .text "\x00\x00\x00\x00" ; extensions format not yet defined
pflags:
  ; flags for pages $20xx-$BFxx
  ; 00: floating (no memory installed at this address)
  ; 01: ROM
  ; 02: RAM
  ; 03: NVM (persistent storage in cartridge)
  ; 04: bank-switched flags
  .blk $10,$01 ; (4K ROM)
  .blk $90,$00 ; (36K floating)
  .org $2000 ; actual data contained on emulated cartridge starts here
cartromstart:
  .text " Kb  Test " ; must be 10 bytes
  .text "\x47\xA9\x0C\x1E" ; cartridge signature, must be 47 A9 0C 1E
  .word entry ; cartridge code entry point
interruptroutine: ; this label must equal $2010
  RTI
entry:
  ; copy system font
  LDY #$E0 ; system font source page start
  LDA #$10 ; video RAM font page start
copyfontpageloop:
  JSR WAITVSCAN ; wait for vertical retrace (preserve A & Y registers please)
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
  ; initialize colors on output area of screen
  JSR WAITVSCAN
  LDA #$73 ; top left corner
  STA $0A8D
  STA $0BAD
  STA $0CCD
  STA $0DED
  STA $0F0D
  LDA #$5C ; top edge & top content row
  STA $0A8E
  STA $0A8F
  STA $0A90
  STA $0A91
  STA $0BAE
  STA $0BAF
  STA $0BB0
  STA $0BB1
  STA $0CCE
  STA $0CCF
  STA $0CD0
  STA $0CD1
  STA $0DEE
  STA $0DEF
  STA $0DF0
  STA $0DF1
  STA $0F0E
  STA $0F0F
  STA $0F10
  STA $0F11
  LDA #$DC ; top right corner
  STA $0A92
  STA $0BB2
  STA $0CD2
  STA $0DF2
  STA $0F12
  LDA #$3B ; bottom left corner
  STA $0AAD
  STA $0BCD
  STA $0CED
  STA $0E0D
  STA $0F2D
  LDA #$CA ; bottom content row & bottom edge
  STA $0AAE
  STA $0AAF
  STA $0AB0
  STA $0AB1
  STA $0BCE
  STA $0BCF
  STA $0BD0
  STA $0BD1
  STA $0CEE
  STA $0CEF
  STA $0CF0
  STA $0CF1
  STA $0E0E
  STA $0E0F
  STA $0E10
  STA $0E11
  STA $0F2E
  STA $0F2F
  STA $0F30
  STA $0F31
  LDA #$CE ; bottom right corner
  STA $0AB2
  STA $0BD2
  STA $0CF2
  STA $0E12
  STA $0F32
  ; initialize border blocks
  LDA #$00 ; block divided into quarters
  STA $088D
  STA $088E
  STA $088F
  STA $0890
  STA $0891
  STA $0892
  STA $08AD
  STA $08B2
  STA $08CD
  STA $08D2
  STA $08ED
  STA $08EE
  STA $08EF
  STA $08F0
  STA $08F1
  STA $08F2
  LDA #$20 ; space
  STA $08D1
resetkey:
  LDA #$20
newkey:
  TAY ; save a copy of keypress value
  CLV ; treat it as a positive value in subroutine
  JSR BYTETODEC ; convert to decimal
  TYA ; restore keypress value into Accumulator
  LDY #$10 ; init video frame countup to -240
  JSR WAITVSCAN
  LDX $08AF
  STX $08AE
  LDX $08B0
  STX $08AF
  LDX $08B1
  STX $08B0
  STA $08B1
  LDA $8D
  STA $08CE
  LDA $8E
  STA $08CF
  LDA $8F
  STA $08D0
mainloop:
  LDA $0244 ; read and clear keypress register
  CMP #$00
  BNE newkey
  JSR WAITVSCAN
  INY
  BEQ resetkey
  JMP mainloop
  .align 12 ; fill a 4KB chunk
