# Retro 6k Fantasy Computer Entertainment System Emulator

## About the Retro 6k FCES

The Retro 6k is a “fantasy computer entertainment system”, a. k. a. “fantasy console”, with capabilities in the vicinity of those of the Nintendo Entertainment System and Commodore 64. It is designed by Maggie David P. K. Haynes, who is considering the possibility of actually building a physical Retro 6k machine. The “6k” in the name comes from the fact that Mrs. Haynes first designed the video adapter, which has only 6144 bytes of video RAM, and then started designing a computer around that. Retro 6k games and software are typically packaged on cartridges, but may instead be loaded by other means such as using disk or tape peripherals. Users can also install their own custom content ROM chips containing fonts or music to be used by supporting software.

## Using the Retro 6k Emulator

Open the emulator menu with numpad 0, then use arrow keys / enter to select an option. If you know the number of the main menu option you want, you can activate it without first opening the menu, by pressing the corresponding digit key on the numpad. Note, several menu options are not yet imple!ented.

## Programming the Retro 6k Emulator

Compile [sdk/r6k-pg.tex](sdk/r6k-pg.tex) using `pdflatex` and then consult `r6k-pg.pdf`.

I generate my cartridges by using [vasm](http://sun.hasenbraten.de/vasm/index.php?view=main) and its Oldstyle Syntax Module. [Windows vasm binaries](https://www.chibiakumas.com/z80/vasm.php) are available for download.

## Compiling the Retro 6k Emulator from Source

### Linux / GCC

Install the SDL2 binaries and development libraries for your system if you don't already have them. In Debian, that's the packages `libsdl2-2.0-0`, `libsdl2-dev`, and their dependencies. You'll need GCC/G++ version 8 or higher, because C++17 features such as `std::filesystem` are used. Run the script `buildr6k` in the source root. If you move the compiled program elsewhere, copy or move the `.rom` and `.R6kCart` files with it.

### MS Visual Studio

Go to the [SDL2 download page](http://www.libsdl.org/download-2.0.php) and download the latest Visual C++ Development Libraries. Create a folder for your SDL2 libraries in a convenient location, *e. g.* `C:\Program Files\SDL2-`*(version number)* and extract the `include` and `lib` folders there from the ZIP file you downloaded.  

Create an empty Visual C++ project. In the project Property Pages, make sure you have All Configurations and All Platforms selected, then go to the VC++ Directories page. Add the full path to the extracted `include` folder to the list in the property Include Directories. The list is semicolon-delimited.

Go to the Linker / General page. Select the Win32 platform, and add the full path to the `x86` subfolder of the `lib` folder that was extracted earlier, to the Additional Library Directories property. Again, that's a semicolon-delimited list. Select the x64 platform, and do the same thing, except this time add the path to the `x64` subfolder instead of the `x86` subfolder. Then select All Platforms again.

Then on the Linker / Input, replace the value of Additional Dependencies with `SDL2.lib;SDL2main.lib`. Finally, go to the Linker / System page, and for SubSystem, select Windows.

Extract the Retro 6k source code into the same folder as the project `.vcxproj` file.

Now add to your new project the following header files:
* `custombuild.h`
* `fake6502.h`
* `main.h`
* `strcpys.h`

And the following source files:
* `fake6502.c`
* `main.cpp`

If you intend to introduce your own modifications to the Retro 6k emulator, edit `custombuild.h` and place in the string literal whatever version / author information you want to appear in the emulator's About box.

Take whatever steps suit your workflow to ensure that when run, the compiled emulator's working directory contains `banke.rom` and `bankf.rom`. Note, when launched from Visual Studio, the working directory is where the project file is, which should be where the `.rom` files wound up. However, when `.exe` files built by Visual Studio won't be in that directory. Instead they'll be in `..\`*(platform)*`\`*(configuration)* relative to the project directory. Relative to the `.exe` files, the `.rom` files are in `..\..\retro6k` *(or whatever the project got named)*. I personally chose to create shortcuts in the project directory pointing to the Debug and Release configuration builds of my computer's architecture (x64) and made sure the working directory set in those shortcuts' properties was the project directory where the `.rom` files are.

## Binary Releases

I intend to periodically provide releases with Windows 64-bit binaries, documentation compiled to PDF, and possibly Debian packages. 

---

*pretend this part is right-aligned — MDPKH*
