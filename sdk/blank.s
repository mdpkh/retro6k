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
  .text "blank cart" ; must be 10 bytes
  .text "\x47\xA9\x0C\x1E" ; cartridge signature, must be 47 A9 0C 1E
  .word entry ; cartridge code entry point
interruptroutine: ; this label must equal $2010
  RTI
entry:
  JMP entry ; do-nothing infinite loop
  .align 12
