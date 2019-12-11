VIDEOSTATE = $02FC
FVMCDEST = $03F8
FVMCSRC = $03F9
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
  .text "`R6k Demo`" ; must be 10 bytes
  .text "\x47\xA9\x0C\x1E" ; cartridge signature, must be 47 A9 0C 1E
  .word entry ; cartridge code entry point
interruptroutine: ; this label must equal $2010
  RTI
entry:
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
halt:
  JMP halt
waitvscan:
  TAX
waitvscanloop:
  LDA VIDEOSTATE
  AND #$30
  CMP #$20
  BNE waitvscanloop
  TXA
  RTS
  .align 12
