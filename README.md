# Retro 6k Fantasy Computer Entertainment System Emulator

## About the Retro 6k FCES

to do

## Using the Retro 6k Emulator

Open the emulator menu with numpad 0, then use arrow keys / enter to select an option. If you know the number of the main menu option you want, you can activate it without first opening the menu, by pressing the corresponding digit key on the numpad. Note, several menu options are not yet imple!ented.

## Programming the Retro 6k Emulator

Compile [sdk/r6k-pg.tex](sdk/r6k-pg.tex) using `pdflatex` and then consult `r6k-pg.pdf`.

I generate my cartridges by using [vasm](http://sun.hasenbraten.de/vasm/index.php?view=main) and its Oldstyle Syntax Module. [Windows vasm binaries](https://www.chibiakumas.com/z80/vasm.php) are available for download.

## Compiling the Retro 6k Emulator from Source

Install the SDL2 binaries and development libraries for your system if you don't already have them. In Debian, that's the packages `libsdl2-2.0-0`, `libsdl2-dev`, and their dependencies. You'll need GCC/G++ version 8 or higher. Run the script `buildr6k` in the source root. If you move the compiled program elsewhere, copy or move the `.rom` and `.R6kCart` files with it.

## Binary Releases

I intend to periodically provide releases with Windows 64-bit binaries. 

---

*pretend this part is right-aligned — MDPKH*
