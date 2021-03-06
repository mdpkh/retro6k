# Retro 6k Fantasy Computer Entertainment System Emulator

## About the Retro 6k FCES

The Retro 6k is a “fantasy computer entertainment system”, a. k. a. “fantasy
console”, with capabilities in the vicinity of those of the Nintendo
Entertainment System and Commodore 64. It is designed by Maggie David P. K.
Haynes, who is considering the possibility of actually building a physical
Retro 6k machine. The “6k” in the name comes from the fact that Mrs. Haynes
first designed the video adapter, which has only 6144 bytes of video RAM, and
then started designing a computer around that. Retro 6k games and software are
typically packaged on cartridges, but may instead be loaded by other means such
as using disk or tape peripherals. Users can also install their own custom
content ROM chips containing fonts or music to be used by supporting software.

## Using the Retro 6k Emulator

Open the emulator menu with numpad 0, then use arrow keys / enter to select an
option. If you know the number of the main menu option you want, you can
activate it without first opening the menu, by pressing the corresponding digit
key on the numpad. Note, several menu options are not yet implemented.

## Programming the Retro 6k Emulator

Compile [docs/pg-src/r6k-pg.tex](docs/pg-src/r6k-pg.tex) using `pdflatex` (may
need to run a few times to get TOC and table-layout stuff right) and then
consult `r6k-pg.pdf` (to make it easy to find again, move `r6k-pg.pdf` to
`docs`).

I generate my cartridges by using
[vasm](http://sun.hasenbraten.de/vasm/index.php?view=main) and its Oldstyle
Syntax Module. Windows users:
[Windows vasm binaries](https://www.chibiakumas.com/z80/vasm.php) are available
for download so you don't have to try to build vasm from source.

## Compiling the Retro 6k Emulator from Source

### Linux / GCC

Install the SDL2 binaries and development libraries for your system if you
don't already have them.  You'll need GCC/G++ version 8 or higher, because
C++17 features such as `std::filesystem` are used. On Debian based distros,
run `sudo apt install libsdl2-dev g++-8` (which will automatically install
GCC8 and the normal libSDL2 libraries as well).

Once all dependencies are installed, simply run `make` to generate a binary. To
use an alternate compiler, such as Clang, simply override the CXX and CC
variables(`make CXX=clang`). You can also override the compilation flags by
tweaking CFLAGS and CXXFLAGS (*e. g.*
`make CXXFLAGS="-march=native -O2" CFLAGS="-march=native -O2"` for a
significant speed boost and a binary that will only work on your machine).

The binary will by default by generated in ./compiled/bin/ as r6k. You can
override the name by tweaking the BINARY variable, and the location by
tweaking OUTPUT_FOLDER. To run the newly produced binary, you can either run
the produced file by hand or use `make run`.

### MS Visual Studio

Go to the [SDL2 download page](http://www.libsdl.org/download-2.0.php) and
download the latest Visual C++ Development Libraries. Create a folder for your
SDL2 libraries in a convenient location, *e. g.*
`C:\Program Files\SDL2-`*(version number)* and extract the `include` and `lib`
folders there from the ZIP file you downloaded.

Extract the Retro 6k source tree somewhere. Open `retro6k.vcxproj` in Visual
Studio Code. Open the retro6k Project Properties. Select All Configurations,
and the `Win32` platform. Under Linker / General, ensure that Additional 
Library Directories contains the full path to the `lib\x86` subfolder you
extracted earlier. Click Apply, then select the `x64` platform. ensure that
Additional Library Directories contains the full path to the `lib\x64`
subfolder you extracted earlier. Click Apply, then select All Platforms.

Under VC++ Directories, ensure that Include Directories includes the full path
to the `include` folder you extracted earlier. Then under C/C++ / Preprocessor,
remove `;OFFICIAL_BUILD` from the Preprocessor Definitions list.

Under Build Events / Pre-Build, clear the pre-build command line entry.

## Binary Releases

I intend to periodically provide releases with Windows 64-bit binaries, 
documentation compiled to PDF, and possibly Debian packages.

## Custom Builds

If you intend to modifiy the Retro 6k Emulator's functionality, please edit
`custombuild.h` to indicate what is customized, version info if applicable, and
your name.

---

*pretend this part is right-aligned — MDPKH*
