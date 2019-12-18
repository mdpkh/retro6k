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
  ; initialize beat counter
  LDA #$00
  STA $00
  ; initialize voice types
  LDA #$01
  STA $038C
  ; initialize scv map
  LDA #$E1
  STA $038D
  ; reset scheduled sound registers
  LDA #$FC
  STA $038E
loopstart:
  ; schedule a drum beat on voice 0, schedule controller 0
  LDX #$00 ; frequency low
  LDY #$40 ; frequency high
  LDA #$00 ; schedule page
  STA $038F
  LDA #$04 ; duration low
  STA $0390
  LDA #$00 ; duration high
  STA $0391
  LDA #$FF ; volume
  STA $03C0
  STX $03A0
  STY $03A1
  LDA #$09 ; duration low
  STA $0392
  LDA #$00 ; duration high
  STA $0393
  LDA #$EE ; volume
  STA $03C1
  STX $03A2
  STY $03A3
  LDA #$0A ; duration low
  STA $0394
  LDA #$00 ; duration high
  STA $0395
  LDA #$DD ; volume
  STA $03C2
  STX $03A4
  STY $03A5
  LDA #$0A ; duration low
  STA $0396
  LDA #$00 ; duration high
  STA $0397
  LDA #$CC ; volume
  STA $03C3
  STX $03A6
  STY $03A7
  LDA #$01 ; schedule page
  STA $038F
  LDA #$0C ; duration low
  STA $0390
  LDA #$00 ; duration high
  STA $0391
  LDA #$BB ; volume
  STA $03C0
  STX $03A0
  STY $03A1
  LDA #$0C ; duration low
  STA $0392
  LDA #$00 ; duration high
  STA $0393
  LDA #$AA ; volume
  STA $03C1
  STX $03A2
  STY $03A3
  LDA #$0E ; duration low
  STA $0394
  LDA #$00 ; duration high
  STA $0395
  LDA #$99 ; volume
  STA $03C2
  STX $03A4
  STY $03A5
  LDA #$10 ; duration low
  STA $0396
  LDA #$00 ; duration high
  STA $0397
  LDA #$88 ; volume
  STA $03C3
  STX $03A6
  STY $03A7
  LDA #$02 ; schedule page
  STA $038F
  LDA #$12 ; duration low
  STA $0390
  LDA #$00 ; duration high
  STA $0391
  LDA #$77 ; volume
  STA $03C0
  STX $03A0
  STY $03A1
  LDA #$14 ; duration low
  STA $0392
  LDA #$00 ; duration high
  STA $0393
  LDA #$66 ; volume
  STA $03C1
  STX $03A2
  STY $03A3
  LDA #$19 ; duration low
  STA $0394
  LDA #$00 ; duration high
  STA $0395
  LDA #$55 ; volume
  STA $03C2
  STX $03A4
  STY $03A5
  LDA #$20 ; duration low
  STA $0396
  LDA #$00 ; duration high
  STA $0397
  LDA #$44 ; volume
  STA $03C3
  STX $03A6
  STY $03A7
  LDA #$03 ; schedule page
  STA $038F
  LDA #$2A ; duration low
  STA $0390
  LDA #$00 ; duration high
  STA $0391
  LDA #$33 ; volume
  STA $03C0
  STX $03A0
  STY $03A1
  LDA #$40 ; duration low
  STA $0392
  LDA #$00 ; duration high
  STA $0393
  LDA #$22 ; volume
  STA $03C1
  STX $03A2
  STY $03A3
  LDA #$89 ; duration low
  STA $0394
  LDA #$00 ; duration high
  STA $0395
  LDA #$11 ; volume
  STA $03C2
  STX $03A4
  STY $03A5
  ; clear CND 0
  LDA #$01
  STA $038E
  ; check beat counter
  LDA $00
  AND #$03
  BNE beatonly
  ; on every 4th beat,
  ; schedule a G-note on voice 1, schedule controller 1
  LDX #$0E ; frequency low
  LDY #$03 ; frequency high
  LDA #$00 ; schedule page
  STA $038F
  LDA #$0C ; duration low
  STA $0398
  LDA #$00 ; duration high
  STA $0399
  LDA #$FF ; volume
  STA $03C4
  STX $03A8
  STY $03A9
  LDA #$1B ; duration low
  STA $039A
  LDA #$00 ; duration high
  STA $039B
  LDA #$EE ; volume
  STA $03C5
  STX $03AA
  STY $03AB
  LDA #$1E ; duration low
  STA $039C
  LDA #$00 ; duration high
  STA $039D
  LDA #$DD ; volume
  STA $03C6
  STX $03AC
  STY $03AD
  LDA #$1E ; duration low
  STA $039E
  LDA #$00 ; duration high
  STA $039F
  LDA #$CC ; volume
  STA $03C7
  STX $03AE
  STY $03AF
  LDA #$01 ; schedule page
  STA $038F
  LDA #$24 ; duration low
  STA $0398
  LDA #$00 ; duration high
  STA $0399
  LDA #$BB ; volume
  STA $03C4
  STX $03A8
  STY $03A9
  LDA #$24 ; duration low
  STA $039A
  LDA #$00 ; duration high
  STA $039B
  LDA #$AA ; volume
  STA $03C5
  STX $03AA
  STY $03AB
  LDA #$2A ; duration low
  STA $039C
  LDA #$00 ; duration high
  STA $039D
  LDA #$99 ; volume
  STA $03C6
  STX $03AC
  STY $03AD
  LDA #$30 ; duration low
  STA $039E
  LDA #$00 ; duration high
  STA $039F
  LDA #$88 ; volume
  STA $03C7
  STX $03AE
  STY $03AF
  LDA #$02 ; schedule page
  STA $038F
  LDA #$36 ; duration low
  STA $0398
  LDA #$00 ; duration high
  STA $0399
  LDA #$77 ; volume
  STA $03C4
  STX $03A8
  STY $03A9
  LDA #$3C ; duration low
  STA $039A
  LDA #$00 ; duration high
  STA $039B
  LDA #$66 ; volume
  STA $03C5
  STX $03AA
  STY $03AB
  LDA #$4B ; duration low
  STA $039C
  LDA #$00 ; duration high
  STA $039D
  LDA #$55 ; volume
  STA $03C6
  STX $03AC
  STY $03AD
  LDA #$60 ; duration low
  STA $039E
  LDA #$00 ; duration high
  STA $039F
  LDA #$44 ; volume
  STA $03C7
  STX $03AE
  STY $03AF
  LDA #$03 ; schedule page
  STA $038F
  LDA #$7E; duration low
  STA $0398
  LDA #$00 ; duration high
  STA $0399
  LDA #$33 ; volume
  STA $03C4
  STX $03A8
  STY $03A9
  LDA #$C0 ; duration low
  STA $039A
  LDA #$00 ; duration high
  STA $039B
  LDA #$22 ; volume
  STA $03C5
  STX $03AA
  STY $03AB
  LDA #$9B ; duration low
  STA $039C
  LDA #$01 ; duration high
  STA $039D
  LDA #$11 ; volume
  STA $03C6
  STX $03AC
  STY $03AD
  ; check beat counter
  LDA $00
  AND #$0F
  BNE beatonly
  ; on every 8th beat,
  ; schedule an E-note on voice 2, (schedule controller 1)
  LDX #$48 ; frequency low
  LDY #$01 ; frequency high
  LDA #$00 ; schedule page
  STA $038F
  LDA #$FF ; volume
  STA $03C8
  STX $03B0
  STY $03B1
  LDA #$EE ; volume
  STA $03C9
  STX $03B2
  STY $03B3
  LDA #$DD ; volume
  STA $03CA
  STX $03B4
  STY $03B5
  LDA #$CC ; volume
  STA $03CB
  STX $03B6
  STY $03B7
  LDA #$01 ; schedule page
  STA $038F
  LDA #$BB ; volume
  STA $03C8
  STX $03B0
  STY $03B1
  LDA #$AA ; volume
  STA $03C9
  STX $03B2
  STY $03B3
  LDA #$99 ; volume
  STA $03CA
  STX $03B4
  STY $03B5
  LDA #$88 ; volume
  STA $03CB
  STX $03B6
  STY $03B7
  LDA #$02 ; schedule page
  STA $038F
  LDA #$77 ; volume
  STA $03C9
  STX $03B0
  STY $03B1
  LDA #$66 ; volume
  STA $03C9
  STX $03B2
  STY $03B3
  LDA #$55 ; volume
  STA $03CA
  STX $03B4
  STY $03B5
  LDA #$44 ; volume
  STA $03CB
  STX $03B6
  STY $03B7
  LDA #$03 ; schedule page
  STA $038F
  LDA #$33 ; volume
  STA $03C8
  STX $03B0
  STY $03B1
  LDA #$22 ; volume
  STA $03C9
  STX $03B2
  STY $03B3
  LDA #$11 ; volume
  STA $03CA
  STX $03B4
  STY $03B5
  ; on every 16th beat,
  ; schedule a C-note on voice 3, (schedule controller 1)
  LDX #$05 ; frequency low
  LDY #$01 ; frequency high
  LDA #$00 ; schedule page
  STA $038F
  LDA #$FF ; volume
  STA $03CC
  STX $03B8
  STY $03B9
  LDA #$EE ; volume
  STA $03CD
  STX $03BA
  STY $03BB
  LDA #$DD ; volume
  STA $03CE
  STX $03BC
  STY $03BD
  LDA #$CC ; volume
  STA $03CF
  STX $03BE
  STY $03BF
  LDA #$01 ; schedule page
  STA $038F
  LDA #$BB ; volume
  STA $03CC
  STX $03B8
  STY $03B9
  LDA #$AA ; volume
  STA $03CD
  STX $03BA
  STY $03BB
  LDA #$99 ; volume
  STA $03CE
  STX $03BC
  STY $03BD
  LDA #$88 ; volume
  STA $03CF
  STX $03BE
  STY $03BF
  LDA #$02 ; schedule page
  STA $038F
  LDA #$77 ; volume
  STA $03CC
  STX $03B8
  STY $03B9
  LDA #$66 ; volume
  STA $03CD
  STX $03BA
  STY $03BB
  LDA #$55 ; volume
  STA $03CE
  STX $03BC
  STY $03BD
  LDA #$44 ; volume
  STA $03CF
  STX $03BE
  STY $03BF
  LDA #$03 ; schedule page
  STA $038F
  LDA #$33 ; volume
  STA $03CC
  STX $03B8
  STY $03B9
  LDA #$22 ; volume
  STA $03CD
  STX $03BA
  STY $03BB
  LDA #$11 ; volume
  STA $03CE
  STX $03BC
  STY $03BD
beatonly:
  ; debug break
  ;LDA #$01
  ;STA $03FF
  ; clear CND 1
  LDA #$02
  STA $038E
  ; wait a little before repeating loop
  LDA #$FE
  STA $20
  STA $21
  STA $22
  INC $00
waitloop:
  CLC
  LDA $20
  ADC #$01
  STA $20
  LDA $21
  ADC #$00
  STA $21
  LDA $22
  ADC #$00
  STA $22
  BNE waitloop
  LDA $21
  BNE waitloop
  LDA $20
  BNE waitloop
  ; set palette to beat counter
waithscan:
  LDA $02FC
  AND #$03
  CMP #$02
  BNE waithscan
  LDA $00
  STA $0FE0
  JMP loopstart
  .align 12
