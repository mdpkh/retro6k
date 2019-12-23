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
  .blk $20,$01 ; (8K ROM)
  .blk $80,$00 ; (32K floating)
headerend:
  .org $2000 ; actual data contained on emulated cartridge starts here
cartromstart:
  .text "By2De Test" ; must be 10 bytes
  .text "\x47\xA9\x0C\x1E" ; cartridge signature, must be 47 A9 0C 1E
  .word entry ; cartridge code entry point
interruptroutine: ; this label must equal $2010
  RTI
BYTE2DECP  = $F259
BYTE2DECN  = $F20F
BYTE2DECUA = $F248
BYTE2DECUS = $F237
BYTE2DECS  = $F23C
BYTE2DECBP = $F24A
FVMCDEST   = $03F8
FVMCSRC    = $03F9
WAITVSCAN  = $F1D7
entry:
  LDA #$20
  STA $0400
  STA $0405
  STA $040A
  STA $040F
  STA $0414
  STA $0419
  STA $041E
  STA $041F
  STA $0420
  STA $0425
  STA $042A
  STA $042F
  STA $0434
  STA $0439
  STA $043E
  STA $043F
  STA $0440
  STA $0445
  STA $044A
  STA $044F
  STA $0454
  STA $0459
  STA $045E
  STA $045F
  STA $0460
  STA $0465
  STA $046A
  STA $046F
  STA $0474
  STA $0479
  STA $047E
  STA $047F
  STA $0480
  STA $0485
  STA $048A
  STA $048F
  STA $0494
  STA $0499
  STA $049E
  STA $049F
  STA $04A0
  STA $04A5
  STA $04AA
  STA $04AF
  STA $04B4
  STA $04B9
  STA $04BE
  STA $04BF
  STA $04C0
  STA $04C5
  STA $04CA
  STA $04CF
  STA $04D4
  STA $04D9
  STA $04DE
  STA $04DF
  STA $04E0
  STA $04E5
  STA $04EA
  STA $04EF
  STA $04F4
  STA $04F9
  STA $04FE
  STA $04FF
  STA $0500
  STA $0505
  STA $050A
  STA $050F
  STA $0514
  STA $0519
  STA $051E
  STA $051F
  STA $0520
  STA $0525
  STA $052A
  STA $052F
  STA $0534
  STA $0539
  STA $053E
  STA $053F
  STA $0540
  STA $0545
  STA $054A
  STA $054F
  STA $0554
  STA $0559
  STA $055E
  STA $055F
  STA $0560
  STA $0565
  STA $056A
  STA $056F
  STA $0574
  STA $0579
  STA $057E
  STA $057F
  STA $0580
  STA $0585
  STA $058A
  STA $058F
  STA $0594
  STA $0599
  STA $059E
  STA $059F
  STA $05A0
  STA $05A5
  STA $05AA
  STA $05AF
  STA $05B4
  STA $05B9
  STA $05BE
  STA $05BF
  STA $05C0
  STA $05C5
  STA $05CA
  STA $05CF
  STA $05D4
  STA $05D9
  STA $05DE
  STA $05DF
  STA $05E0
  STA $05E5
  STA $05EA
  STA $05EF
  STA $05F4
  STA $05F9
  STA $05FE
  STA $05FF
  ; copy system font
  LDY #$E2 ; system font source page start
  LDA #$12 ; video RAM font page start
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
  CMP #$18 ; compare against stop page number
  BNE copyfontpageloop
  JSR WAITVSCAN ; wait for vertical retrace
  ; initialize screen display
  LDX #$00
initscreenloop:
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
  ; screen lines 0-3, 8-11 attribute bit 2: 0/0 0/0 1/1 1/1
  STA $0C80,X
  STA $0D00,X
  LDA #$FF ; screen lines 4-7, 12-15 attribute bit 2: 1/1 1/1 1/1 1/1
  STA $0CC0,X
  STA $0D40,X
  LDA #$FF ; screen lines 16&17 attribute bit 2, sl 0-17 ab 3: 1/1 1/1 1/1 1/1
  STA $0D80,X
  STA $0DC0,X
  STA $0E00,X
  STA $0E40,X
  STA $0E80,X
  LDA #$00 ; screen lines 0-15 attribute bit 4: 0/0 0/0 0/0 0/0
  STA $0EC0,X
  STA $0F00,X
  STA $0F40,X
  STA $0F80,X
  LDA datasyscolors-$20,X ; sl 16&17 ab 4, palette colors
  STA $0FC0,X
  INX
  TXA
  AND #$07
  CMP #$00
  BNE initscreenloop
  TXA
  JSR WAITVSCAN
  CMP #$40
  BNE initscreenloop
  ;--------------------------------------------------------------------
  LDA #$20
  CLC
  CLV
  JSR BYTE2DECP
  LDA $8C
  STA $0401
  LDA $8D
  STA $0402
  LDA $8E
  STA $0403
  LDA $8F
  STA $0404
  LDA #$20
  CLC
  CLV
  JSR BYTE2DECN
  LDA $8C
  STA $0406
  LDA $8D
  STA $0407
  LDA $8E
  STA $0408
  LDA $8F
  STA $0409
  LDA #$20
  CLC
  CLV
  JSR BYTE2DECUA
  LDA $8C
  STA $040B
  LDA $8D
  STA $040C
  LDA $8E
  STA $040D
  LDA $8F
  STA $040E
  LDA #$20
  CLC
  CLV
  JSR BYTE2DECUS
  LDA $8C
  STA $0410
  LDA $8D
  STA $0411
  LDA $8E
  STA $0412
  LDA $8F
  STA $0413
  LDA #$20
  CLC
  CLV
  JSR BYTE2DECS
  LDA $8C
  STA $0415
  LDA $8D
  STA $0416
  LDA $8E
  STA $0417
  LDA $8F
  STA $0418
  LDA #$20
  CLC
  CLV
  JSR BYTE2DECBP
  LDA $8C
  STA $041A
  LDA $8D
  STA $041B
  LDA $8E
  STA $041C
  LDA $8F
  STA $041D
  ;--------------------------------------------------------------------
  LDA #$20
  SEC
  CLV
  JSR BYTE2DECP
  LDA $8C
  STA $0421
  LDA $8D
  STA $0422
  LDA $8E
  STA $0423
  LDA $8F
  STA $0424
  LDA #$20
  SEC
  CLV
  JSR BYTE2DECN
  LDA $8C
  STA $0426
  LDA $8D
  STA $0427
  LDA $8E
  STA $0428
  LDA $8F
  STA $0429
  LDA #$20
  SEC
  CLV
  JSR BYTE2DECUA
  LDA $8C
  STA $042B
  LDA $8D
  STA $042C
  LDA $8E
  STA $042D
  LDA $8F
  STA $042E
  LDA #$20
  SEC
  CLV
  JSR BYTE2DECUS
  LDA $8C
  STA $0430
  LDA $8D
  STA $0431
  LDA $8E
  STA $0432
  LDA $8F
  STA $0433
  LDA #$20
  SEC
  CLV
  JSR BYTE2DECS
  LDA $8C
  STA $0435
  LDA $8D
  STA $0436
  LDA $8E
  STA $0437
  LDA $8F
  STA $0438
  LDA #$20
  SEC
  CLV
  JSR BYTE2DECBP
  LDA $8C
  STA $043A
  LDA $8D
  STA $043B
  LDA $8E
  STA $043C
  LDA $8F
  STA $043D
  ;--------------------------------------------------------------------
  LDA #$80
  ADC #$80
  LDA #$20
  CLC
  JSR BYTE2DECP
  LDA $8C
  STA $0441
  LDA $8D
  STA $0442
  LDA $8E
  STA $0443
  LDA $8F
  STA $0444
  LDA #$80
  ADC #$80
  LDA #$20
  CLC
  JSR BYTE2DECN
  LDA $8C
  STA $0446
  LDA $8D
  STA $0447
  LDA $8E
  STA $0448
  LDA $8F
  STA $0449
  LDA #$80
  ADC #$80
  LDA #$20
  CLC
  JSR BYTE2DECUA
  LDA $8C
  STA $044B
  LDA $8D
  STA $044C
  LDA $8E
  STA $044D
  LDA $8F
  STA $044E
  LDA #$80
  ADC #$80
  LDA #$20
  CLC
  JSR BYTE2DECUS
  LDA $8C
  STA $0450
  LDA $8D
  STA $0451
  LDA $8E
  STA $0452
  LDA $8F
  STA $0453
  LDA #$80
  ADC #$80
  LDA #$20
  CLC
  JSR BYTE2DECS
  LDA $8C
  STA $0455
  LDA $8D
  STA $0456
  LDA $8E
  STA $0457
  LDA $8F
  STA $0458
  LDA #$80
  ADC #$80
  LDA #$20
  CLC
  JSR BYTE2DECBP
  LDA $8C
  STA $045A
  LDA $8D
  STA $045B
  LDA $8E
  STA $045C
  LDA $8F
  STA $045D
  ;--------------------------------------------------------------------
  LDA #$80
  ADC #$80
  LDA #$20
  SEC
  JSR BYTE2DECP
  LDA $8C
  STA $0461
  LDA $8D
  STA $0462
  LDA $8E
  STA $0463
  LDA $8F
  STA $0464
  LDA #$80
  ADC #$80
  LDA #$20
  SEC
  JSR BYTE2DECN
  LDA $8C
  STA $0466
  LDA $8D
  STA $0467
  LDA $8E
  STA $0468
  LDA $8F
  STA $0469
  LDA #$80
  ADC #$80
  LDA #$20
  SEC
  JSR BYTE2DECUA
  LDA $8C
  STA $046B
  LDA $8D
  STA $046C
  LDA $8E
  STA $046D
  LDA $8F
  STA $046E
  LDA #$80
  ADC #$80
  LDA #$20
  SEC
  JSR BYTE2DECUS
  LDA $8C
  STA $0470
  LDA $8D
  STA $0471
  LDA $8E
  STA $0472
  LDA $8F
  STA $0473
  LDA #$80
  ADC #$80
  LDA #$20
  SEC
  JSR BYTE2DECS
  LDA $8C
  STA $0475
  LDA $8D
  STA $0476
  LDA $8E
  STA $0477
  LDA $8F
  STA $0478
  LDA #$80
  ADC #$80
  LDA #$20
  SEC
  JSR BYTE2DECBP
  LDA $8C
  STA $047A
  LDA $8D
  STA $047B
  LDA $8E
  STA $047C
  LDA $8F
  STA $047D
  ;--------------------------------------------------------------------
  LDA #$60
  CLC
  CLV
  JSR BYTE2DECP
  LDA $8C
  STA $0481
  LDA $8D
  STA $0482
  LDA $8E
  STA $0483
  LDA $8F
  STA $0484
  LDA #$60
  CLC
  CLV
  JSR BYTE2DECN
  LDA $8C
  STA $0486
  LDA $8D
  STA $0487
  LDA $8E
  STA $0488
  LDA $8F
  STA $0489
  LDA #$60
  CLC
  CLV
  JSR BYTE2DECUA
  LDA $8C
  STA $048B
  LDA $8D
  STA $048C
  LDA $8E
  STA $048D
  LDA $8F
  STA $048E
  LDA #$60
  CLC
  CLV
  JSR BYTE2DECUS
  LDA $8C
  STA $0490
  LDA $8D
  STA $0491
  LDA $8E
  STA $0492
  LDA $8F
  STA $0493
  LDA #$60
  CLC
  CLV
  JSR BYTE2DECS
  LDA $8C
  STA $0495
  LDA $8D
  STA $0496
  LDA $8E
  STA $0497
  LDA $8F
  STA $0498
  LDA #$60
  CLC
  CLV
  JSR BYTE2DECBP
  LDA $8C
  STA $049A
  LDA $8D
  STA $049B
  LDA $8E
  STA $049C
  LDA $8F
  STA $049D
  ;--------------------------------------------------------------------
  LDA #$60
  SEC
  CLV
  JSR BYTE2DECP
  LDA $8C
  STA $04A1
  LDA $8D
  STA $04A2
  LDA $8E
  STA $04A3
  LDA $8F
  STA $04A4
  LDA #$60
  SEC
  CLV
  JSR BYTE2DECN
  LDA $8C
  STA $04A6
  LDA $8D
  STA $04A7
  LDA $8E
  STA $04A8
  LDA $8F
  STA $04A9
  LDA #$60
  SEC
  CLV
  JSR BYTE2DECUA
  LDA $8C
  STA $04AB
  LDA $8D
  STA $04AC
  LDA $8E
  STA $04AD
  LDA $8F
  STA $04AE
  LDA #$60
  SEC
  CLV
  JSR BYTE2DECUS
  LDA $8C
  STA $04B0
  LDA $8D
  STA $04B1
  LDA $8E
  STA $04B2
  LDA $8F
  STA $04B3
  LDA #$60
  SEC
  CLV
  JSR BYTE2DECS
  LDA $8C
  STA $04B5
  LDA $8D
  STA $04B6
  LDA $8E
  STA $04B7
  LDA $8F
  STA $04B8
  LDA #$60
  SEC
  CLV
  JSR BYTE2DECBP
  LDA $8C
  STA $04BA
  LDA $8D
  STA $04BB
  LDA $8E
  STA $04BC
  LDA $8F
  STA $04BD
  ;--------------------------------------------------------------------
  LDA #$80
  ADC #$80
  LDA #$60
  CLC
  JSR BYTE2DECP
  LDA $8C
  STA $04C1
  LDA $8D
  STA $04C2
  LDA $8E
  STA $04C3
  LDA $8F
  STA $04C4
  LDA #$80
  ADC #$80
  LDA #$60
  CLC
  JSR BYTE2DECN
  LDA $8C
  STA $04C6
  LDA $8D
  STA $04C7
  LDA $8E
  STA $04C8
  LDA $8F
  STA $04C9
  LDA #$80
  ADC #$80
  LDA #$60
  CLC
  JSR BYTE2DECUA
  LDA $8C
  STA $04CB
  LDA $8D
  STA $04CC
  LDA $8E
  STA $04CD
  LDA $8F
  STA $04CE
  LDA #$80
  ADC #$80
  LDA #$60
  CLC
  JSR BYTE2DECUS
  LDA $8C
  STA $04D0
  LDA $8D
  STA $04D1
  LDA $8E
  STA $04D2
  LDA $8F
  STA $04D3
  LDA #$80
  ADC #$80
  LDA #$60
  CLC
  JSR BYTE2DECS
  LDA $8C
  STA $04D5
  LDA $8D
  STA $04D6
  LDA $8E
  STA $04D7
  LDA $8F
  STA $04D8
  LDA #$80
  ADC #$80
  LDA #$60
  CLC
  JSR BYTE2DECBP
  LDA $8C
  STA $04DA
  LDA $8D
  STA $04DB
  LDA $8E
  STA $04DC
  LDA $8F
  STA $04DD
  ;--------------------------------------------------------------------
  LDA #$80
  ADC #$80
  LDA #$60
  SEC
  JSR BYTE2DECP
  LDA $8C
  STA $04E1
  LDA $8D
  STA $04E2
  LDA $8E
  STA $04E3
  LDA $8F
  STA $04E4
  LDA #$80
  ADC #$80
  LDA #$60
  SEC
  JSR BYTE2DECN
  LDA $8C
  STA $04E6
  LDA $8D
  STA $04E7
  LDA $8E
  STA $04E8
  LDA $8F
  STA $04E9
  LDA #$80
  ADC #$80
  LDA #$60
  SEC
  JSR BYTE2DECUA
  LDA $8C
  STA $04EB
  LDA $8D
  STA $04EC
  LDA $8E
  STA $04ED
  LDA $8F
  STA $04EE
  LDA #$80
  ADC #$80
  LDA #$60
  SEC
  JSR BYTE2DECUS
  LDA $8C
  STA $04F0
  LDA $8D
  STA $04F1
  LDA $8E
  STA $04F2
  LDA $8F
  STA $04F3
  LDA #$80
  ADC #$80
  LDA #$60
  SEC
  JSR BYTE2DECS
  LDA $8C
  STA $04F5
  LDA $8D
  STA $04F6
  LDA $8E
  STA $04F7
  LDA $8F
  STA $04F8
  LDA #$80
  ADC #$80
  LDA #$60
  SEC
  JSR BYTE2DECBP
  LDA $8C
  STA $04FA
  LDA $8D
  STA $04FB
  LDA $8E
  STA $04FC
  LDA $8F
  STA $04FD
  ;--------------------------------------------------------------------
  LDA #$A0
  CLC
  CLV
  JSR BYTE2DECP
  LDA $8C
  STA $0501
  LDA $8D
  STA $0502
  LDA $8E
  STA $0503
  LDA $8F
  STA $0504
  LDA #$A0
  CLC
  CLV
  JSR BYTE2DECN
  LDA $8C
  STA $0506
  LDA $8D
  STA $0507
  LDA $8E
  STA $0508
  LDA $8F
  STA $0509
  LDA #$A0
  CLC
  CLV
  JSR BYTE2DECUA
  LDA $8C
  STA $050B
  LDA $8D
  STA $050C
  LDA $8E
  STA $050D
  LDA $8F
  STA $050E
  LDA #$A0
  CLC
  CLV
  JSR BYTE2DECUS
  LDA $8C
  STA $0510
  LDA $8D
  STA $0511
  LDA $8E
  STA $0512
  LDA $8F
  STA $0513
  LDA #$A0
  CLC
  CLV
  JSR BYTE2DECS
  LDA $8C
  STA $0515
  LDA $8D
  STA $0516
  LDA $8E
  STA $0517
  LDA $8F
  STA $0518
  LDA #$A0
  CLC
  CLV
  JSR BYTE2DECBP
  LDA $8C
  STA $051A
  LDA $8D
  STA $051B
  LDA $8E
  STA $051C
  LDA $8F
  STA $051D
  ;--------------------------------------------------------------------
  LDA #$A0
  SEC
  CLV
  JSR BYTE2DECP
  LDA $8C
  STA $0521
  LDA $8D
  STA $0522
  LDA $8E
  STA $0523
  LDA $8F
  STA $0524
  LDA #$A0
  SEC
  CLV
  JSR BYTE2DECN
  LDA $8C
  STA $0526
  LDA $8D
  STA $0527
  LDA $8E
  STA $0528
  LDA $8F
  STA $0529
  LDA #$A0
  SEC
  CLV
  JSR BYTE2DECUA
  LDA $8C
  STA $052B
  LDA $8D
  STA $052C
  LDA $8E
  STA $052D
  LDA $8F
  STA $052E
  LDA #$A0
  SEC
  CLV
  JSR BYTE2DECUS
  LDA $8C
  STA $0530
  LDA $8D
  STA $0531
  LDA $8E
  STA $0532
  LDA $8F
  STA $0533
  LDA #$A0
  SEC
  CLV
  JSR BYTE2DECS
  LDA $8C
  STA $0535
  LDA $8D
  STA $0536
  LDA $8E
  STA $0537
  LDA $8F
  STA $0538
  LDA #$A0
  SEC
  CLV
  JSR BYTE2DECBP
  LDA $8C
  STA $053A
  LDA $8D
  STA $053B
  LDA $8E
  STA $053C
  LDA $8F
  STA $053D
  ;--------------------------------------------------------------------
  LDA #$50
  ADC #$50
  CLC
  JSR BYTE2DECP
  LDA $8C
  STA $0541
  LDA $8D
  STA $0542
  LDA $8E
  STA $0543
  LDA $8F
  STA $0544
  LDA #$50
  ADC #$50
  CLC
  JSR BYTE2DECN
  LDA $8C
  STA $0546
  LDA $8D
  STA $0547
  LDA $8E
  STA $0548
  LDA $8F
  STA $0549
  LDA #$50
  ADC #$50
  CLC
  JSR BYTE2DECUA
  LDA $8C
  STA $054B
  LDA $8D
  STA $054C
  LDA $8E
  STA $054D
  LDA $8F
  STA $054E
  LDA #$50
  ADC #$50
  CLC
  JSR BYTE2DECUS
  LDA $8C
  STA $0550
  LDA $8D
  STA $0551
  LDA $8E
  STA $0552
  LDA $8F
  STA $0553
  LDA #$50
  ADC #$50
  CLC
  JSR BYTE2DECS
  LDA $8C
  STA $0555
  LDA $8D
  STA $0556
  LDA $8E
  STA $0557
  LDA $8F
  STA $0558
  LDA #$50
  ADC #$50
  CLC
  JSR BYTE2DECBP
  LDA $8C
  STA $055A
  LDA $8D
  STA $055B
  LDA $8E
  STA $055C
  LDA $8F
  STA $055D
  ;--------------------------------------------------------------------
  LDA #$50
  ADC #$50
  SEC
  JSR BYTE2DECP
  LDA $8C
  STA $0561
  LDA $8D
  STA $0562
  LDA $8E
  STA $0563
  LDA $8F
  STA $0564
  LDA #$50
  ADC #$50
  SEC
  JSR BYTE2DECN
  LDA $8C
  STA $0566
  LDA $8D
  STA $0567
  LDA $8E
  STA $0568
  LDA $8F
  STA $0569
  LDA #$50
  ADC #$50
  SEC
  JSR BYTE2DECUA
  LDA $8C
  STA $056B
  LDA $8D
  STA $056C
  LDA $8E
  STA $056D
  LDA $8F
  STA $056E
  LDA #$50
  ADC #$50
  SEC
  JSR BYTE2DECUS
  LDA $8C
  STA $0570
  LDA $8D
  STA $0571
  LDA $8E
  STA $0572
  LDA $8F
  STA $0573
  LDA #$50
  ADC #$50
  SEC
  JSR BYTE2DECS
  LDA $8C
  STA $0575
  LDA $8D
  STA $0576
  LDA $8E
  STA $0577
  LDA $8F
  STA $0578
  LDA #$50
  ADC #$50
  SEC
  JSR BYTE2DECBP
  LDA $8C
  STA $057A
  LDA $8D
  STA $057B
  LDA $8E
  STA $057C
  LDA $8F
  STA $057D
  ;--------------------------------------------------------------------
  LDA #$E0
  CLC
  CLV
  JSR BYTE2DECP
  LDA $8C
  STA $0581
  LDA $8D
  STA $0582
  LDA $8E
  STA $0583
  LDA $8F
  STA $0584
  LDA #$E0
  CLC
  CLV
  JSR BYTE2DECN
  LDA $8C
  STA $0586
  LDA $8D
  STA $0587
  LDA $8E
  STA $0588
  LDA $8F
  STA $0589
  LDA #$E0
  CLC
  CLV
  JSR BYTE2DECUA
  LDA $8C
  STA $058B
  LDA $8D
  STA $058C
  LDA $8E
  STA $058D
  LDA $8F
  STA $058E
  LDA #$E0
  CLC
  CLV
  JSR BYTE2DECUS
  LDA $8C
  STA $0590
  LDA $8D
  STA $0591
  LDA $8E
  STA $0592
  LDA $8F
  STA $0593
  LDA #$E0
  CLC
  CLV
  JSR BYTE2DECS
  LDA $8C
  STA $0595
  LDA $8D
  STA $0596
  LDA $8E
  STA $0597
  LDA $8F
  STA $0598
  LDA #$E0
  CLC
  CLV
  JSR BYTE2DECBP
  LDA $8C
  STA $059A
  LDA $8D
  STA $059B
  LDA $8E
  STA $059C
  LDA $8F
  STA $059D
  ;--------------------------------------------------------------------
  LDA #$E0
  SEC
  CLV
  JSR BYTE2DECP
  LDA $8C
  STA $05A1
  LDA $8D
  STA $05A2
  LDA $8E
  STA $05A3
  LDA $8F
  STA $05A4
  LDA #$E0
  SEC
  CLV
  JSR BYTE2DECN
  LDA $8C
  STA $05A6
  LDA $8D
  STA $05A7
  LDA $8E
  STA $05A8
  LDA $8F
  STA $05A9
  LDA #$E0
  SEC
  CLV
  JSR BYTE2DECUA
  LDA $8C
  STA $05AB
  LDA $8D
  STA $05AC
  LDA $8E
  STA $05AD
  LDA $8F
  STA $05AE
  LDA #$E0
  SEC
  CLV
  JSR BYTE2DECUS
  LDA $8C
  STA $05B0
  LDA $8D
  STA $05B1
  LDA $8E
  STA $05B2
  LDA $8F
  STA $05B3
  LDA #$E0
  SEC
  CLV
  JSR BYTE2DECS
  LDA $8C
  STA $05B5
  LDA $8D
  STA $05B6
  LDA $8E
  STA $05B7
  LDA $8F
  STA $05B8
  LDA #$E0
  SEC
  CLV
  JSR BYTE2DECBP
  LDA $8C
  STA $05BA
  LDA $8D
  STA $05BB
  LDA $8E
  STA $05BC
  LDA $8F
  STA $05BD
  ;--------------------------------------------------------------------
  LDA #$70
  ADC #$70
  CLC
  JSR BYTE2DECP
  LDA $8C
  STA $05C1
  LDA $8D
  STA $05C2
  LDA $8E
  STA $05C3
  LDA $8F
  STA $05C4
  LDA #$70
  ADC #$70
  CLC
  JSR BYTE2DECN
  LDA $8C
  STA $05C6
  LDA $8D
  STA $05C7
  LDA $8E
  STA $05C8
  LDA $8F
  STA $05C9
  LDA #$70
  ADC #$70
  CLC
  JSR BYTE2DECUA
  LDA $8C
  STA $05CB
  LDA $8D
  STA $05CC
  LDA $8E
  STA $05CD
  LDA $8F
  STA $05CE
  LDA #$70
  ADC #$70
  CLC
  JSR BYTE2DECUS
  LDA $8C
  STA $05D0
  LDA $8D
  STA $05D1
  LDA $8E
  STA $05D2
  LDA $8F
  STA $05D3
  LDA #$70
  ADC #$70
  CLC
  JSR BYTE2DECS
  LDA $8C
  STA $05D5
  LDA $8D
  STA $05D6
  LDA $8E
  STA $05D7
  LDA $8F
  STA $05D8
  LDA #$70
  ADC #$70
  CLC
  JSR BYTE2DECBP
  LDA $8C
  STA $05DA
  LDA $8D
  STA $05DB
  LDA $8E
  STA $05DC
  LDA $8F
  STA $05DD
  ;--------------------------------------------------------------------
  LDA #$70
  ADC #$70
  SEC
  JSR BYTE2DECP
  LDA $8C
  STA $05E1
  LDA $8D
  STA $05E2
  LDA $8E
  STA $05E3
  LDA $8F
  STA $05E4
  LDA #$70
  ADC #$70
  SEC
  JSR BYTE2DECN
  LDA $8C
  STA $05E6
  LDA $8D
  STA $05E7
  LDA $8E
  STA $05E8
  LDA $8F
  STA $05E9
  LDA #$70
  ADC #$70
  SEC
  JSR BYTE2DECUA
  LDA $8C
  STA $05EB
  LDA $8D
  STA $05EC
  LDA $8E
  STA $05ED
  LDA $8F
  STA $05EE
  LDA #$70
  ADC #$70
  SEC
  JSR BYTE2DECUS
  LDA $8C
  STA $05F0
  LDA $8D
  STA $05F1
  LDA $8E
  STA $05F2
  LDA $8F
  STA $05F3
  LDA #$70
  ADC #$70
  SEC
  JSR BYTE2DECS
  LDA $8C
  STA $05F5
  LDA $8D
  STA $05F6
  LDA $8E
  STA $05F7
  LDA $8F
  STA $05F8
  LDA #$70
  ADC #$70
  SEC
  JSR BYTE2DECBP
  LDA $8C
  STA $05FA
  LDA $8D
  STA $05FB
  LDA $8E
  STA $05FC
  LDA $8F
  STA $05FD
  ;--------------------------------------------------------------------
  LDA #$01
  STA $03FF ; debug break
  ;--------------------------------------------------------------------
  LDY #$04 ; general memory page
  LDA #$08 ; character page
  JSR WAITVSCAN ; wait for vertical retrace (preserve A & Y registers please)
  ; enable fast video memory copy
  CLC
  STA FVMCDEST
  STY FVMCSRC
  STA $00
  STA $01
  STA $02
  STA $03
  STA $04
  STA $05
  STA $06
  STA $07
  STA $08
  STA $09
  STA $0A
  STA $0B
  STA $0C
  STA $0D
  STA $0E
  STA $0F
  STA $10
  STA $11
  STA $12
  STA $13
  STA $14
  STA $15
  STA $16
  STA $17
  STA $18
  STA $19
  STA $1A
  STA $1B
  STA $1C
  STA $1D
  STA $1E
  STA $1F
  STA $20
  STA $21
  STA $22
  STA $23
  STA $24
  STA $25
  STA $26
  STA $27
  STA $28
  STA $29
  STA $2A
  STA $2B
  STA $2C
  STA $2D
  STA $2E
  STA $2F
  STA $30
  STA $31
  STA $32
  STA $33
  STA $34
  STA $35
  STA $36
  STA $37
  STA $38
  STA $39
  STA $3A
  STA $3B
  STA $3C
  STA $3D
  STA $3E
  STA $3F
  STA $40
  STA $41
  STA $42
  STA $43
  STA $44
  STA $45
  STA $46
  STA $47
  STA $48
  STA $49
  STA $4A
  STA $4B
  STA $4C
  STA $4D
  STA $4E
  STA $4F
  STA $50
  STA $51
  STA $52
  STA $53
  STA $54
  STA $55
  STA $56
  STA $57
  STA $58
  STA $59
  STA $5A
  STA $5B
  STA $5C
  STA $5D
  STA $5E
  STA $5F
  STA $60
  STA $61
  STA $62
  STA $63
  STA $64
  STA $65
  STA $66
  STA $67
  STA $68
  STA $69
  STA $6A
  STA $6B
  STA $6C
  STA $6D
  STA $6E
  STA $6F
  STA $70
  STA $71
  STA $72
  STA $73
  STA $74
  STA $75
  STA $76
  STA $77
  STA $78
  STA $79
  STA $7A
  STA $7B
  STA $7C
  STA $7D
  STA $7E
  STA $7F
  STA $80
  STA $81
  STA $82
  STA $83
  STA $84
  STA $85
  STA $86
  STA $87
  STA $88
  STA $89
  STA $8A
  STA $8B
  STA $8C
  STA $8D
  STA $8E
  STA $8F
  STA $90
  STA $91
  STA $92
  STA $93
  STA $94
  STA $95
  STA $96
  STA $97
  STA $98
  STA $99
  STA $9A
  STA $9B
  STA $9C
  STA $9D
  STA $9E
  STA $9F
  STA $A0
  STA $A1
  STA $A2
  STA $A3
  STA $A4
  STA $A5
  STA $A6
  STA $A7
  STA $A8
  STA $A9
  STA $AA
  STA $AB
  STA $AC
  STA $AD
  STA $AE
  STA $AF
  STA $B0
  STA $B1
  STA $B2
  STA $B3
  STA $B4
  STA $B5
  STA $B6
  STA $B7
  STA $B8
  STA $B9
  STA $BA
  STA $BB
  STA $BC
  STA $BD
  STA $BE
  STA $BF
  STA $C0
  STA $C1
  STA $C2
  STA $C3
  STA $C4
  STA $C5
  STA $C6
  STA $C7
  STA $C8
  STA $C9
  STA $CA
  STA $CB
  STA $CC
  STA $CD
  STA $CE
  STA $CF
  STA $D0
  STA $D1
  STA $D2
  STA $D3
  STA $D4
  STA $D5
  STA $D6
  STA $D7
  STA $D8
  STA $D9
  STA $DA
  STA $DB
  STA $DC
  STA $DD
  STA $DE
  STA $DF
  STA $E0
  STA $E1
  STA $E2
  STA $E3
  STA $E4
  STA $E5
  STA $E6
  STA $E7
  STA $E8
  STA $E9
  STA $EA
  STA $EB
  STA $EC
  STA $ED
  STA $EE
  STA $EF
  STA $F0
  STA $F1
  STA $F2
  STA $F3
  STA $F4
  STA $F5
  STA $F6
  STA $F7
  STA $F8
  STA $F9
  STA $FA
  STA $FB
  STA $FC
  STA $FD
  STA $FE
  STA $FF
  LDY #$05 ; general memory page
  LDA #$09 ; character page
  JSR WAITVSCAN ; wait for vertical retrace (preserve A & Y registers please)
  CLC
  STA FVMCDEST
  STY FVMCSRC
  STA $00
  STA $01
  STA $02
  STA $03
  STA $04
  STA $05
  STA $06
  STA $07
  STA $08
  STA $09
  STA $0A
  STA $0B
  STA $0C
  STA $0D
  STA $0E
  STA $0F
  STA $10
  STA $11
  STA $12
  STA $13
  STA $14
  STA $15
  STA $16
  STA $17
  STA $18
  STA $19
  STA $1A
  STA $1B
  STA $1C
  STA $1D
  STA $1E
  STA $1F
  STA $20
  STA $21
  STA $22
  STA $23
  STA $24
  STA $25
  STA $26
  STA $27
  STA $28
  STA $29
  STA $2A
  STA $2B
  STA $2C
  STA $2D
  STA $2E
  STA $2F
  STA $30
  STA $31
  STA $32
  STA $33
  STA $34
  STA $35
  STA $36
  STA $37
  STA $38
  STA $39
  STA $3A
  STA $3B
  STA $3C
  STA $3D
  STA $3E
  STA $3F
  STA $40
  STA $41
  STA $42
  STA $43
  STA $44
  STA $45
  STA $46
  STA $47
  STA $48
  STA $49
  STA $4A
  STA $4B
  STA $4C
  STA $4D
  STA $4E
  STA $4F
  STA $50
  STA $51
  STA $52
  STA $53
  STA $54
  STA $55
  STA $56
  STA $57
  STA $58
  STA $59
  STA $5A
  STA $5B
  STA $5C
  STA $5D
  STA $5E
  STA $5F
  STA $60
  STA $61
  STA $62
  STA $63
  STA $64
  STA $65
  STA $66
  STA $67
  STA $68
  STA $69
  STA $6A
  STA $6B
  STA $6C
  STA $6D
  STA $6E
  STA $6F
  STA $70
  STA $71
  STA $72
  STA $73
  STA $74
  STA $75
  STA $76
  STA $77
  STA $78
  STA $79
  STA $7A
  STA $7B
  STA $7C
  STA $7D
  STA $7E
  STA $7F
  STA $80
  STA $81
  STA $82
  STA $83
  STA $84
  STA $85
  STA $86
  STA $87
  STA $88
  STA $89
  STA $8A
  STA $8B
  STA $8C
  STA $8D
  STA $8E
  STA $8F
  STA $90
  STA $91
  STA $92
  STA $93
  STA $94
  STA $95
  STA $96
  STA $97
  STA $98
  STA $99
  STA $9A
  STA $9B
  STA $9C
  STA $9D
  STA $9E
  STA $9F
  STA $A0
  STA $A1
  STA $A2
  STA $A3
  STA $A4
  STA $A5
  STA $A6
  STA $A7
  STA $A8
  STA $A9
  STA $AA
  STA $AB
  STA $AC
  STA $AD
  STA $AE
  STA $AF
  STA $B0
  STA $B1
  STA $B2
  STA $B3
  STA $B4
  STA $B5
  STA $B6
  STA $B7
  STA $B8
  STA $B9
  STA $BA
  STA $BB
  STA $BC
  STA $BD
  STA $BE
  STA $BF
  STA $C0
  STA $C1
  STA $C2
  STA $C3
  STA $C4
  STA $C5
  STA $C6
  STA $C7
  STA $C8
  STA $C9
  STA $CA
  STA $CB
  STA $CC
  STA $CD
  STA $CE
  STA $CF
  STA $D0
  STA $D1
  STA $D2
  STA $D3
  STA $D4
  STA $D5
  STA $D6
  STA $D7
  STA $D8
  STA $D9
  STA $DA
  STA $DB
  STA $DC
  STA $DD
  STA $DE
  STA $DF
  STA $E0
  STA $E1
  STA $E2
  STA $E3
  STA $E4
  STA $E5
  STA $E6
  STA $E7
  STA $E8
  STA $E9
  STA $EA
  STA $EB
  STA $EC
  STA $ED
  STA $EE
  STA $EF
  STA $F0
  STA $F1
  STA $F2
  STA $F3
  STA $F4
  STA $F5
  STA $F6
  STA $F7
  STA $F8
  STA $F9
  STA $FA
  STA $FB
  STA $FC
  STA $FD
  STA $FE
  STA $FF
  LDA #$00
  STA FVMCDEST
  STA FVMCSRC
  ;--------------------------------------------------------------------
halt:
  JMP halt ; do-nothing infinite loop
  .align 5
datasyscolors:
  .byte $00,$11,$22,$33,$44,$55,$66,$77
  .byte $80,$91,$A2,$B3,$C4,$D5,$E6,$F7
  .byte $08,$19,$2A,$3B,$4C,$5D,$6E,$7F
  .byte $88,$99,$AA,$BB,$CC,$DD,$EE,$FF

  .align 12
