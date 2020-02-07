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
headerend:
  .org $2000 ; actual data contained on emulated cartridge starts here
cartromstart:
  .text "Sound Test" ; must be 10 bytes
  .text "\x47\xA9\x0C\x1E" ; cartridge signature, must be 47 A9 0C 1E
  .word entry ; cartridge code entry point
interruptroutine: ; this label must equal $2010
  RTI
entry:
  ; initialize pitch counter
  LDA #$00
  STA $00
  ; initialize volume levels
  LDA #$E2
  STA $18
  STA $0388
  STA $0389
  STA $038A
  STA $038B
  ; initialize voice types
  LDA #$40
  STA $038C
loopstart:
  CLC
  ; get pitch counter, add 256, store in 10-11 and 12-13
  LDA $00
  STA $10
  STA $12
  LDA #$01
  STA $11
  STA $13
  ; double 12-13
  ASL $12
  ROL $13
  CLC
  ; add 10-11 + 12-13, store in 14-15
  LDA $10
  ADC $12
  STA $14
  LDA $11
  ADC $13
  STA $15
  CLC
  ; copy 12-13 to 16-17
  LDA $12
  STA $16
  LDA $13
  STA $17
  ; double 16-17
  ASL $16
  ROL $17
  CLC
  ; update frequency registers
  LDA $10
  STA $0380
  LDA $11
  STA $0381
  LDA $12
  STA $0382
  LDA $13
  STA $0383
  LDA $14
  STA $0384
  LDA $15
  STA $0385
  LDA $16
  STA $0386
  LDA $17
  STA $0387
  ; wait a little
  LDA #$CC
  STA $20
  STA $21
waitloop:
  CLC
  LDA $20
  ADC #$01
  STA $20
  LDA $21
  ADC #$00
  STA $21
  BNE waitloop
  LDA $20
  BNE waitloop
  ; increment pitch counter
  INC $00
  BNE loopstart
  ; if pitch has rolled over, swap stereo channels
  LDA $18
  EOR #$CC
  STA $18
  STA $0388
  STA $0389
  STA $038A
  STA $038B
  JMP loopstart
  .align 12
