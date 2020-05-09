#include <vector>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

#ifdef _MSC_VER
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

#include "custombuild.h"
#include "fake6502.h"
#include "main.h"
#include "strcpys.h"
#include "sys.h"

/*** SYSTEM GENERAL MEMORY: ***
8-bit RAM:  0x0000-0x00FF (can be accessed faster than memory locations with higher addresses)
Stack:      0x0100-0x01FF (location fixed by 6502 design)
User Input: 0x0200-0x027F (see below)
Sys Status: 0x0280-0x02FF (see below)
Device Out: 0x0300-0x037F (see below)
Sound Out:  0x0380-0x03BF (see below)
Misc Ctrl:  0x03C0-0x03FF (see below)
General:    0x0400-0x07FF (application specific)
 *** VIDEO MEMORY: ***
Characters: 0x0800-0x0A3F (32 bytes per row, 1 byte per cell: 0x800 + 0x20 * R + C)
Attributes: 0x0A40-0x0FDF (5 nonconsecutive nybbles (transpose; represents 4 5-bit attributes) per cell: 
                           0x0A40 + 0x120 * B + 0x10 * (R & 0x1E) + C + 1/2 * (R & 1))
Palette:    0x0FE0-0x0FFF (1 byte per attribute: 0x0FE0 + A)
Font:       0x1000-0x1FFF (16 bytes per glyph, 2 bytes per row, 2 bits per pixel:
                           0x1000 + 0x10 * G + 2 * Y + 1/4 * X)
 *** CARTRIDGE MEMORY: ***
RAM or ROM: 0x2000-0xBFFF (application-specific)
 *** SYSTEM ROM: ***
Bank C:     0xC000-0xCFFF (user expansion)
Bank D:     0xD000-0xDFFF (user expansion)
Bank E:     0xE000-0xEFFF (system font, user replaceable)
Bank F:     0xF000-0xFFFF (system firmware)

 *** INPUT DEVICES ***
User Input: 0x0200-0x027F
  Game Pads: 40, 41, 42, 43
  Keyboard (primary): 44
  Keyboard (low-level): 45
  Pointer X: 46
  Pointer Y: 47
Sys Status: 0x0280-0x02FF 
  Video Flags: FC
    03: Horizontal state: 00 active; 01 margin; 02 horz flyback start; 03 horz flyback in progress
	0C: Timing mode: 00 VGA; 04 PAL; 08 NTSC (set by user switch)
    30: Vertical state: 00 active scanline; 10 skipped scanline; 20 vert flyback start; 30 vert flyback in progress
	40: Scanline skip: 00 off; 40 on (set by cartridge jumper)
	80: Aspect ratio: 00 4:3; 80 16:9 (set by user switch)

  *** OUTPUT DEVICES ***
Device Out: 0x0300-0x037F 
Sound Out:  0x0380-0x03CF 
Misc Ctrl:  0x03D0-0x03FF 
  Fast Video Memory Copy Mode / Dest Page: F8
  Fast Video Memory Src Page: F9
  Video Interrupt Condition Flags: FC
*/
unsigned char* sysram;
pageflags* syspflags;
bool cartridgeinserted = false;
bool scanlinedirty[576];
bool videobusy;
bool suppresslogging;
unsigned char videostate;
unsigned char videomode = 0x00;
unsigned char keypressregister;
unsigned short soundfreqregister[4];
unsigned char soundvolregister[4];
unsigned char soundvoicetyperegister;
unsigned char soundscvmapregister;
unsigned char soundqueueregisteroffset;
unsigned short soundqueuedur[2][16];
soundqueueentry soundqueue[4][16];
int soundcndcounter[2];
unsigned char soundncounter[2];
unsigned short menubeepdur;
unsigned short menubeepfreq;
bool menuhassound = false;
constexpr int dlogbits = 18;
constexpr int dlogidxmask = (2 << dlogbits) - 1;
dlogentry debuglog[2 << dlogbits];
int debuglogstart = 0;
int debuglogend = 1;
int debuglogexpectargs = 0;
unsigned char stackbase;
dlogentry* partialinst = nullptr;
std::random_device seedgen;
std::mt19937 floatgen;
std::mt19937 noisegen;
SDL_Window* mainwindow;
SDL_Surface* cellcanvas;
SDL_Surface* framebuffer;
SDL_AudioSpec soundspec;
SDL_AudioDeviceID sounddev;
unsigned int UE_RESETCPU;

void CollapsePaths()
{
	// merge config.sessionpath and config.path vectors (sessionpath first)
	config.sessionpath.cartpath.insert(
		config.sessionpath.cartpath.end(),
		config.path.cartpath.begin(),
		config.path.cartpath.end());
	config.sessionpath.rompath.insert(
		config.sessionpath.rompath.end(),
		config.path.rompath.begin(),
		config.path.rompath.end());
	config.sessionpath.savepath.insert(
		config.sessionpath.savepath.end(),
		config.path.savepath.begin(),
		config.path.savepath.end());
	config.sessionpath.screencappath.insert(
		config.sessionpath.screencappath.end(),
		config.path.screencappath.begin(),
		config.path.screencappath.end());
	// in each combined vector, set every path to its canonical form
	// --- delete path item if it doesn't exist in filesystem
	for (auto it = config.sessionpath.cartpath.begin();
		it != config.sessionpath.cartpath.end(); )
	{
		if (std::filesystem::exists(*it))
		{
			*it = std::filesystem::canonical(*it);
			++it;
		}
		else
		{
			it = config.sessionpath.cartpath.erase(it);
		}
	}
	for (auto it = config.sessionpath.rompath.begin();
		it != config.sessionpath.rompath.end(); )
	{
		if (std::filesystem::exists(*it))
		{
			*it = std::filesystem::canonical(*it);
			++it;
		}
		else
		{
			it = config.sessionpath.rompath.erase(it);
		}
	}
	for (auto it = config.sessionpath.savepath.begin();
		it != config.sessionpath.savepath.end(); )
	{
		if (std::filesystem::exists(*it))
		{
			*it = std::filesystem::canonical(*it);
			++it;
		}
		else
		{
			it = config.sessionpath.savepath.erase(it);
		}
	}
	for (auto it = config.sessionpath.screencappath.begin();
		it != config.sessionpath.screencappath.end(); )
	{
		if (std::filesystem::exists(*it))
		{
			*it = std::filesystem::canonical(*it);
			++it;
		}
		else
		{
			it = config.sessionpath.screencappath.erase(it);
		}
	}
	// in each combined vector, remove any duplicates of previous items
	for (auto it1 = config.sessionpath.cartpath.begin();
		it1 != config.sessionpath.cartpath.end(); )
	{
		for (auto it2 = config.sessionpath.cartpath.begin();
			it2 != it1; )
		{
			if (*it1 == *it2)
			{
				it1 = config.sessionpath.cartpath.erase(it1);
				break;
			}
			else
			{
				++it2;
			}
		}
		++it1;
	}
	for (auto it1 = config.sessionpath.rompath.begin();
		it1 != config.sessionpath.rompath.end(); )
	{
		for (auto it2 = config.sessionpath.rompath.begin();
			it2 != it1; )
		{
			if (*it1 == *it2)
			{
				it1 = config.sessionpath.rompath.erase(it1);
				break;
			}
			else
			{
				++it2;
			}
		}
		++it1;
	}
	for (auto it1 = config.sessionpath.savepath.begin();
		it1 != config.sessionpath.savepath.end(); )
	{
		for (auto it2 = config.sessionpath.savepath.begin();
			it2 != it1; )
		{
			if (*it1 == *it2)
			{
				it1 = config.sessionpath.savepath.erase(it1);
				break;
			}
			else
			{
				++it2;
			}
		}
		++it1;
	}
	for (auto it1 = config.sessionpath.screencappath.begin();
		it1 != config.sessionpath.screencappath.end(); )
	{
		for (auto it2 = config.sessionpath.screencappath.begin();
			it2 != it1; )
		{
			if (*it1 == *it2)
			{
				it1 = config.sessionpath.screencappath.erase(it1);
				break;
			}
			else
			{
				++it2;
			}
		}
		++it1;
	}
	return;
}

inline void DisplayArgs(char* dest, unsigned char opc, char* args, char** end)
{
	const enum amode : char {
		AM_IMP,
		AM_ACC,
		AM_IMM,
		AM_REL,
		AM_ZP,
		AM_ZPX,
		AM_ZPY,
		AM_INDX,
		AM_INDY,
		AM_ABS,
		AM_ABSX,
		AM_ABSY,
		AM_IND
	} amodes[256] = {
		AM_IMP, AM_INDX, AM_IMP, AM_INDX,  AM_ZP,  AM_ZP,  AM_ZP,  AM_ZP, AM_IMP,  AM_IMM, AM_ACC,  AM_IMM,  AM_ABS,  AM_ABS,  AM_ABS,  AM_ABS,
		AM_REL, AM_INDY, AM_IMP, AM_INDY, AM_ZPX, AM_ZPX, AM_ZPX, AM_ZPX, AM_IMP, AM_ABSY, AM_IMP, AM_ABSY, AM_ABSX, AM_ABSX, AM_ABSX, AM_ABSX,
		AM_ABS, AM_INDX, AM_IMP, AM_INDX,  AM_ZP,  AM_ZP,  AM_ZP,  AM_ZP, AM_IMP,  AM_IMM, AM_ACC,  AM_IMM,  AM_ABS,  AM_ABS,  AM_ABS,  AM_ABS,
		AM_REL, AM_INDY, AM_IMP, AM_INDY, AM_ZPX, AM_ZPX, AM_ZPX, AM_ZPX, AM_IMP, AM_ABSY, AM_IMP, AM_ABSY, AM_ABSX, AM_ABSX, AM_ABSX, AM_ABSX,
		AM_IMP, AM_INDX, AM_IMP, AM_INDX,  AM_ZP,  AM_ZP,  AM_ZP,  AM_ZP, AM_IMP,  AM_IMM, AM_ACC,  AM_IMM,  AM_ABS,  AM_ABS,  AM_ABS,  AM_ABS,
		AM_REL, AM_INDY, AM_IMP, AM_INDY, AM_ZPX, AM_ZPX, AM_ZPX, AM_ZPX, AM_IMP, AM_ABSY, AM_IMP, AM_ABSY, AM_ABSX, AM_ABSX, AM_ABSX, AM_ABSX,
		AM_IMP, AM_INDX, AM_IMP, AM_INDX,  AM_ZP,  AM_ZP,  AM_ZP,  AM_ZP, AM_IMP,  AM_IMM, AM_ACC,  AM_IMM,  AM_IND,  AM_ABS,  AM_ABS,  AM_ABS,
		AM_REL, AM_INDY, AM_IMP, AM_INDY, AM_ZPX, AM_ZPX, AM_ZPX, AM_ZPX, AM_IMP, AM_ABSY, AM_IMP, AM_ABSY, AM_ABSX, AM_ABSX, AM_ABSX, AM_ABSX,
		AM_IMM, AM_INDX, AM_IMM, AM_INDX,  AM_ZP,  AM_ZP,  AM_ZP,  AM_ZP, AM_IMP,  AM_IMM, AM_IMP,  AM_IMM,  AM_ABS,  AM_ABS,  AM_ABS,  AM_ABS,
		AM_REL, AM_INDY, AM_IMP, AM_INDY, AM_ZPX, AM_ZPX, AM_ZPY, AM_ZPY, AM_IMP, AM_ABSY, AM_IMP, AM_ABSY, AM_ABSX, AM_ABSX, AM_ABSY, AM_ABSY,
		AM_IMM, AM_INDX, AM_IMM, AM_INDX,  AM_ZP,  AM_ZP,  AM_ZP,  AM_ZP, AM_IMP,  AM_IMM, AM_IMP,  AM_IMM,  AM_ABS,  AM_ABS,  AM_ABS,  AM_ABS,
		AM_REL, AM_INDY, AM_IMP, AM_INDY, AM_ZPX, AM_ZPX, AM_ZPY, AM_ZPY, AM_IMP, AM_ABSY, AM_IMP, AM_ABSY, AM_ABSX, AM_ABSX, AM_ABSY, AM_ABSY,
		AM_IMM, AM_INDX, AM_IMM, AM_INDX,  AM_ZP,  AM_ZP,  AM_ZP,  AM_ZP, AM_IMP,  AM_IMM, AM_IMP,  AM_IMM,  AM_ABS,  AM_ABS,  AM_ABS,  AM_ABS,
		AM_REL, AM_INDY, AM_IMP, AM_INDY, AM_ZPX, AM_ZPX, AM_ZPX, AM_ZPX, AM_IMP, AM_ABSY, AM_IMP, AM_ABSY, AM_ABSX, AM_ABSX, AM_ABSX, AM_ABSX,
		AM_IMM, AM_INDX, AM_IMM, AM_INDX,  AM_ZP,  AM_ZP,  AM_ZP,  AM_ZP, AM_IMP,  AM_IMM, AM_IMP,  AM_IMM,  AM_ABS,  AM_ABS,  AM_ABS,  AM_ABS,
		AM_REL, AM_INDY, AM_IMP, AM_INDY, AM_ZPX, AM_ZPX, AM_ZPX, AM_ZPX, AM_IMP, AM_ABSY, AM_IMP, AM_ABSY, AM_ABSX, AM_ABSX, AM_ABSX, AM_ABSX 
	};
	switch (amodes[opc])
	{
	case AM_IMP:
		*end = dest;
		break;
	case AM_ACC:
		*dest = 0x41;
		*++dest = 0x20;
		*end = dest + 1;
		break;
	case AM_IMM:
		*dest = 0x23;
		*++dest = 0x24;
		DisplayHexByte(++dest, args[0]);
		dest += 2;
		*dest = 0x20;
		*end = dest + 1;
		break;
	case AM_REL:
	case AM_ZP:
		*dest = 0x24;
		DisplayHexByte(++dest, args[0]);
		dest += 2;
		*dest = 0x20;
		*end = dest + 1;
		break;
	case AM_ZPX:
		*dest = 0x24;
		DisplayHexByte(++dest, args[0]);
		dest += 2;
		*dest = 0x2c;
		*++dest = 0x58;
		*++dest = 0x20;
		*end = dest + 1;
		break;
	case AM_ZPY:
		*dest = 0x24;
		DisplayHexByte(++dest, args[0]);
		dest += 2;
		*dest = 0x2c;
		*++dest = 0x59;
		*++dest = 0x20;
		*end = dest + 1;
		break;
	case AM_INDX:
		*dest = 0x28;
		*++dest = 0x24;
		DisplayHexByte(++dest, args[0]);
		dest += 2;
		*dest = 0x2c;
		*++dest = 0x58;
		*++dest = 0x29;
		*++dest = 0x20;
		*end = dest + 1;
		break;
	case AM_INDY:
		*dest = 0x28;
		*++dest = 0x24;
		DisplayHexByte(++dest, args[0]);
		dest += 2;
		*dest = 0x29;
		*++dest = 0x2c;
		*++dest = 0x59;
		*++dest = 0x20;
		*end = dest + 1;
		break;
	case AM_ABS:
		*dest = 0x24;
		DisplayHexByte(++dest, args[1]);
		dest += 2;
		DisplayHexByte(dest, args[0]);
		dest += 2;
		*dest = 0x20;
		*end = dest + 1;
		break;
	case AM_ABSX:
		*dest = 0x24;
		DisplayHexByte(++dest, args[1]);
		dest += 2;
		DisplayHexByte(dest, args[0]);
		dest += 2;
		*dest = 0x2c;
		*++dest = 0x58;
		*++dest = 0x20;
		*end = dest + 1;
		break;
	case AM_ABSY:
		*dest = 0x24;
		DisplayHexByte(++dest, args[1]);
		dest += 2;
		DisplayHexByte(dest, args[0]);
		dest += 2;
		*dest = 0x2c;
		*++dest = 0x59;
		*++dest = 0x20;
		*end = dest + 1;
		break;
	case AM_IND:
		*dest = 0x28;
		*++dest = 0x24;
		DisplayHexByte(++dest, args[1]);
		dest += 2;
		DisplayHexByte(dest, args[0]);
		dest += 2;
		*dest = 0x29;
		*++dest = 0x20;
		*end = dest + 1;
		break;
	}
}

inline void DisplayHexByte(char* dest, char val)
{
	const char nybbles[16] = {
		0x30, 0x31, 0x32, 0x33,
		0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x41, 0x42,
		0x43, 0x44, 0x45, 0x46,
	};
	*dest = nybbles[(val >> 4) & 0xf];
	*++dest = nybbles[val & 0xf];
}

inline void DisplayHexWord(char* dest, short val)
{
	const char nybbles[16] = {
		0x30, 0x31, 0x32, 0x33,
		0x34, 0x35, 0x36, 0x37,
		0x38, 0x39, 0x41, 0x42,
		0x43, 0x44, 0x45, 0x46,
	};
	*dest = nybbles[(val >> 12) & 0xf];
	*++dest = nybbles[(val >> 8) & 0xf];
	*++dest = nybbles[(val >> 4) & 0xf];
	*++dest = nybbles[val & 0xf];
}

inline void DisplayMemType(char* dest, pageflags pf)
{
	const char typewords[48] = {
		'N',0x7f, 'C',
		'R', 'O', 'M',
		'R', 'A', 'M',
		'N', 'V', 'M',
		'b', 'N', 'C',
		'b', 'R', 'O',
		'b', 'R', 'A',
		'b', 'N', 'V',
		'I', '/', 'O',
		'I', 'n',0x7f,
		'O', 'u', 't',
		'I', '/', 'O',
		'b', 'I', 'O',
		'b', 'I', 'n',
		'b', 'O', 'u',
		'b', 'I', 'O',
	};
	char mtypeidx = (char)pf & 0x0f;
	char* src = (char*)typewords + 3ll * mtypeidx;
	*dest = *src;
	*++dest = *++src;
	*++dest = *++src;
}

inline void DisplayOpcode(char* dest, unsigned char opc)
{
	const char mnemonics[768] = {
		0x42, 0x52, 0x4B, 0x4F, 0x52, 0x41, 0x3F, 0x3F, 0x3F, 0x53, 0x4C, 0x4F,
		0x3F, 0x3F, 0x3F, 0x4F, 0x52, 0x41, 0x41, 0x53, 0x4C, 0x53, 0x4C, 0x4F,
		0x50, 0x48, 0x50, 0x4F, 0x52, 0x41, 0x41, 0x53, 0x4C, 0x3F, 0x3F, 0x3F,
		0x3F, 0x3F, 0x3F, 0x4F, 0x52, 0x41, 0x41, 0x53, 0x4C, 0x53, 0x4C, 0x4F,
		0x42, 0x50, 0x4C, 0x4F, 0x52, 0x41, 0x3F, 0x3F, 0x3F, 0x53, 0x4C, 0x4F,
		0x3F, 0x3F, 0x3F, 0x4F, 0x52, 0x41, 0x41, 0x53, 0x4C, 0x53, 0x4C, 0x4F,
		0x43, 0x4C, 0x43, 0x4F, 0x52, 0x41, 0x3F, 0x3F, 0x3F, 0x53, 0x4C, 0x4F,
		0x3F, 0x3F, 0x3F, 0x4F, 0x52, 0x41, 0x41, 0x53, 0x4C, 0x53, 0x4C, 0x4F,
		0x4A, 0x53, 0x52, 0x41, 0x4E, 0x44, 0x3F, 0x3F, 0x3F, 0x52, 0x4C, 0x41,
		0x42, 0x49, 0x54, 0x41, 0x4E, 0x44, 0x52, 0x4F, 0x4C, 0x52, 0x4C, 0x41,
		0x50, 0x4C, 0x50, 0x41, 0x4E, 0x44, 0x52, 0x4F, 0x4C, 0x3F, 0x3F, 0x3F,
		0x42, 0x49, 0x54, 0x41, 0x4E, 0x44, 0x52, 0x4F, 0x4C, 0x52, 0x4C, 0x41,
		0x42, 0x4D, 0x49, 0x41, 0x4E, 0x44, 0x3F, 0x3F, 0x3F, 0x52, 0x4C, 0x41,
		0x3F, 0x3F, 0x3F, 0x41, 0x4E, 0x44, 0x52, 0x4F, 0x4C, 0x52, 0x4C, 0x41,
		0x53, 0x45, 0x43, 0x41, 0x4E, 0x44, 0x3F, 0x3F, 0x3F, 0x52, 0x4C, 0x41,
		0x3F, 0x3F, 0x3F, 0x41, 0x4E, 0x44, 0x52, 0x4F, 0x4C, 0x52, 0x4C, 0x41,
		0x52, 0x54, 0x49, 0x45, 0x4F, 0x52, 0x3F, 0x3F, 0x3F, 0x53, 0x52, 0x45,
		0x3F, 0x3F, 0x3F, 0x45, 0x4F, 0x52, 0x4C, 0x53, 0x52, 0x53, 0x52, 0x45,
		0x50, 0x48, 0x41, 0x45, 0x4F, 0x52, 0x4C, 0x53, 0x52, 0x3F, 0x3F, 0x3F,
		0x4A, 0x4D, 0x50, 0x45, 0x4F, 0x52, 0x4C, 0x53, 0x52, 0x53, 0x52, 0x45,
		0x42, 0x56, 0x43, 0x45, 0x4F, 0x52, 0x3F, 0x3F, 0x3F, 0x53, 0x52, 0x45,
		0x3F, 0x3F, 0x3F, 0x45, 0x4F, 0x52, 0x4C, 0x53, 0x52, 0x53, 0x52, 0x45,
		0x43, 0x4C, 0x49, 0x45, 0x4F, 0x52, 0x3F, 0x3F, 0x3F, 0x53, 0x52, 0x45,
		0x3F, 0x3F, 0x3F, 0x45, 0x4F, 0x52, 0x4C, 0x53, 0x52, 0x53, 0x52, 0x45,
		0x52, 0x54, 0x53, 0x41, 0x44, 0x43, 0x3F, 0x3F, 0x3F, 0x52, 0x52, 0x41,
		0x3F, 0x3F, 0x3F, 0x41, 0x44, 0x43, 0x52, 0x4F, 0x52, 0x52, 0x52, 0x41,
		0x50, 0x4C, 0x41, 0x41, 0x44, 0x43, 0x52, 0x4F, 0x52, 0x3F, 0x3F, 0x3F,
		0x4A, 0x4D, 0x50, 0x41, 0x44, 0x43, 0x52, 0x4F, 0x52, 0x52, 0x52, 0x41,
		0x42, 0x56, 0x53, 0x41, 0x44, 0x43, 0x3F, 0x3F, 0x3F, 0x52, 0x52, 0x41,
		0x3F, 0x3F, 0x3F, 0x41, 0x44, 0x43, 0x52, 0x4F, 0x52, 0x52, 0x52, 0x41,
		0x53, 0x45, 0x49, 0x41, 0x44, 0x43, 0x3F, 0x3F, 0x3F, 0x52, 0x52, 0x41,
		0x3F, 0x3F, 0x3F, 0x41, 0x44, 0x43, 0x52, 0x4F, 0x52, 0x52, 0x52, 0x41,
		0x3F, 0x3F, 0x3F, 0x53, 0x54, 0x41, 0x3F, 0x3F, 0x3F, 0x53, 0x41, 0x58,
		0x53, 0x54, 0x59, 0x53, 0x54, 0x41, 0x53, 0x54, 0x58, 0x53, 0x41, 0x58,
		0x44, 0x45, 0x59, 0x3F, 0x3F, 0x3F, 0x54, 0x58, 0x41, 0x3F, 0x3F, 0x3F,
		0x53, 0x54, 0x59, 0x53, 0x54, 0x41, 0x53, 0x54, 0x58, 0x53, 0x41, 0x58,
		0x42, 0x43, 0x43, 0x53, 0x54, 0x41, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
		0x53, 0x54, 0x59, 0x53, 0x54, 0x41, 0x53, 0x54, 0x58, 0x53, 0x41, 0x58,
		0x54, 0x59, 0x41, 0x53, 0x54, 0x41, 0x54, 0x58, 0x53, 0x3F, 0x3F, 0x3F,
		0x3F, 0x3F, 0x3F, 0x53, 0x54, 0x41, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F,
		0x4C, 0x44, 0x59, 0x4C, 0x44, 0x41, 0x4C, 0x44, 0x58, 0x4C, 0x41, 0x58,
		0x4C, 0x44, 0x59, 0x4C, 0x44, 0x41, 0x4C, 0x44, 0x58, 0x4C, 0x41, 0x58,
		0x54, 0x41, 0x59, 0x4C, 0x44, 0x41, 0x54, 0x41, 0x58, 0x3F, 0x3F, 0x3F,
		0x4C, 0x44, 0x59, 0x4C, 0x44, 0x41, 0x4C, 0x44, 0x58, 0x4C, 0x41, 0x58,
		0x42, 0x43, 0x53, 0x4C, 0x44, 0x41, 0x3F, 0x3F, 0x3F, 0x4C, 0x41, 0x58,
		0x4C, 0x44, 0x59, 0x4C, 0x44, 0x41, 0x4C, 0x44, 0x58, 0x4C, 0x41, 0x58,
		0x43, 0x4C, 0x56, 0x4C, 0x44, 0x41, 0x54, 0x53, 0x58, 0x4C, 0x41, 0x58,
		0x4C, 0x44, 0x59, 0x4C, 0x44, 0x41, 0x4C, 0x44, 0x58, 0x4C, 0x41, 0x58,
		0x43, 0x50, 0x59, 0x43, 0x4D, 0x50, 0x3F, 0x3F, 0x3F, 0x44, 0x43, 0x50,
		0x43, 0x50, 0x59, 0x43, 0x4D, 0x50, 0x44, 0x45, 0x43, 0x44, 0x43, 0x50,
		0x49, 0x4E, 0x59, 0x43, 0x4D, 0x50, 0x44, 0x45, 0x58, 0x3F, 0x3F, 0x3F,
		0x43, 0x50, 0x59, 0x43, 0x4D, 0x50, 0x44, 0x45, 0x43, 0x44, 0x43, 0x50,
		0x42, 0x4E, 0x45, 0x43, 0x4D, 0x50, 0x3F, 0x3F, 0x3F, 0x44, 0x43, 0x50,
		0x3F, 0x3F, 0x3F, 0x43, 0x4D, 0x50, 0x44, 0x45, 0x43, 0x44, 0x43, 0x50,
		0x43, 0x4C, 0x44, 0x43, 0x4D, 0x50, 0x3F, 0x3F, 0x3F, 0x44, 0x43, 0x50,
		0x3F, 0x3F, 0x3F, 0x43, 0x4D, 0x50, 0x44, 0x45, 0x43, 0x44, 0x43, 0x50,
		0x43, 0x50, 0x58, 0x53, 0x42, 0x43, 0x3F, 0x3F, 0x3F, 0x49, 0x53, 0x42,
		0x43, 0x50, 0x58, 0x53, 0x42, 0x43, 0x49, 0x4E, 0x43, 0x49, 0x53, 0x42,
		0x49, 0x4E, 0x58, 0x53, 0x42, 0x43, 0x4E, 0x4F, 0x50, 0x53, 0x42, 0x43,
		0x43, 0x50, 0x58, 0x53, 0x42, 0x43, 0x49, 0x4E, 0x43, 0x49, 0x53, 0x42,
		0x42, 0x45, 0x51, 0x53, 0x42, 0x43, 0x3F, 0x3F, 0x3F, 0x49, 0x53, 0x42,
		0x3F, 0x3F, 0x3F, 0x53, 0x42, 0x43, 0x49, 0x4E, 0x43, 0x49, 0x53, 0x42,
		0x53, 0x45, 0x44, 0x53, 0x42, 0x43, 0x3F, 0x3F, 0x3F, 0x49, 0x53, 0x42,
		0x3F, 0x3F, 0x3F, 0x53, 0x42, 0x43, 0x49, 0x4E, 0x43, 0x49, 0x53, 0x42
	};
	char* src = (char*)mnemonics + 3ll * opc;
	*dest = *src;
	*++dest = *++src;
	*++dest = *++src;
}

void DoAbout() {
	SDL_Surface* winbuffer = SDL_CreateRGBSurfaceWithFormat(
		0,
		377,
		144,
		8,
		SDL_PIXELFORMAT_INDEX8);
	SDL_SetSurfaceBlendMode(winbuffer, SDL_BLENDMODE_BLEND);
	SDL_SetColorKey(winbuffer, true, 0xf9);
	SDL_Surface* mwsurface = SDL_GetWindowSurface(mainwindow);
	SDL_SetSurfaceBlendMode(mwsurface, SDL_BLENDMODE_NONE);
	SDL_Surface* restorescreen = SDL_CreateRGBSurfaceWithFormat(
		0,
		winbuffer->w,
		winbuffer->h,
		mwsurface->format->BitsPerPixel,
		SDL_MasksToPixelFormatEnum(
			mwsurface->format->BitsPerPixel,
			mwsurface->format->Rmask,
			mwsurface->format->Gmask,
			mwsurface->format->Bmask,
			mwsurface->format->Amask));
	if (!winbuffer || !restorescreen)
	{
		SDL_Log("Could not create menu display buffers: %s", SDL_GetError());
		return;
	}
	SDL_SetSurfaceBlendMode(restorescreen, SDL_BLENDMODE_NONE);
	SDL_Rect winpos;
	winpos.w = winbuffer->w;
	winpos.h = winbuffer->h;
	SDL_GetWindowSize(mainwindow, &winpos.x, &winpos.y);
	winpos.x -= winpos.w;
	winpos.y -= winpos.h;
	winpos.x >>= 1;
	winpos.y *= 3;
	winpos.y >>= 2;
	SetHWPalette(winbuffer->format->palette);
	SDL_UpdateWindowSurface(mainwindow);
	SDL_BlitSurface(mwsurface, &winpos, restorescreen, NULL);
	PaintWindow(winbuffer, 0xff, 0x4d, "About Retro 6k", 0xff);
	int tx0;
	int y0 = (winpos.h + 20 - 18 * 6) >> 1;
	{
		// hey, if you're not me, don't change this #ifdef/#else/#endif block
		// and don't define OFFICIAL_BUILD. ---MDPKH
#ifdef OFFICIAL_BUILD
		char* abouttext[6] = {
			(char*)"Retro 6k designed by Maggie\220David",         //points to cstring in program data
			(char*)"P.\220K. Haynes in 2019\22620.",               //points to cstring in program data
			(char*)"",                                             //points to cstring in program data
			nullptr,                                               //will point to cstring in stack
			(char*)"Haynes. Includes public domain 6502 emulator", //points to cstring in program data
			(char*)"core written by Mike Chambers in 2011."        //points to cstring in program data
		};
		const char* vertempl = "Emulator v------- \2512019\22620 Maggie\220David P.\220K."; //program data
		char verline[46];                                                                   //stack
		strcpy_s((char*)&verline, 46, vertempl); //copy cstring from program data into stack
		extern const char* verstring; //points to cstring in program data in a separately-compiled obj file
		verline[10] = verstring[0];   //copy 7 bytes from separate program data to middle of cstring in stack
		verline[11] = verstring[1];   //verstring is assumed to point to at least 7 non-null bytes (see ver.cpp)
		verline[12] = verstring[2];
		verline[13] = verstring[3];
		verline[14] = verstring[4];
		verline[15] = verstring[5];
		verline[16] = verstring[6];
		abouttext[3] = verline;       //now the abouttext array is complete
#else
		char* abouttext[6] = {
			(char*)"Retro 6k designed by Maggie\220David",         
			(char*)"P.\220K. Haynes in 2019\22620.",               
			(char*)"",                                             
			(char*)CUSTOM_BUILD_CREDIT,                                               
			(char*)"Includes public domain 6502 emulator", 
			(char*)"core written by Mike Chambers in 2011."        
		};
#endif
		int mlw = 283;
		for (int i = 0; i < 6; ++i)
		{
			int lw = TextWidth(abouttext[i]);
			if (lw > mlw)
				mlw = lw;
		}
		tx0 = (winpos.w - mlw) >> 1;
		for (int i = 0, x = tx0, y = y0; i < 6; ++i, x = tx0, y += 18)
		{
			DrawText(abouttext[i], x, y, 0x00, 0xff, winbuffer);
		}
	}
	DrawLogo(winpos.w - tx0 - 30, y0 + 3, winbuffer);
	SDL_Rect animrect = winpos;
	SDL_Rect menurect = { 0, 0, winpos.w, winpos.h };
	for (int t = 1; t <= 8; ++t)
	{
		SDL_SetSurfaceAlphaMod(winbuffer, t * 31);
		animrect.w = winpos.w * t / 8;
		menurect.w = animrect.w;
		animrect.x = (mwsurface->w - animrect.w) >> 1;
		SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
		SDL_BlitSurface(winbuffer, &menurect, mwsurface, &animrect);
		SDL_UpdateWindowSurface(mainwindow);
		SDL_Delay(16);
	}
	while (true)
	{
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			SDL_Event newevent;
			switch (event.type)
			{
			case SDL_KEYDOWN:
				goto exitabout;
				break;
			case SDL_QUIT:
				newevent.type = SDL_QUIT;
				SDL_PushEvent(&newevent);
				goto exitabout;
			}
		}
		SDL_Delay(16);
	}
exitabout:
	for (int t = 7; t > 0; --t)
	{
		SDL_SetSurfaceAlphaMod(winbuffer, t * 31);
		animrect.h = winpos.h * t / 8;
		menurect.h = animrect.h;
		menurect.y = winpos.h - animrect.h;
		animrect.y = ((mwsurface->h - animrect.h) * 3) >> 2;
		SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
		SDL_BlitSurface(winbuffer, &menurect, mwsurface, &animrect);
		SDL_UpdateWindowSurface(mainwindow);
		SDL_Delay(16);
	}
	SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
	SDL_FreeSurface(restorescreen);
	SDL_FreeSurface(winbuffer);
}

void DoDebugger() {
	SDL_Surface* winbuffer = SDL_CreateRGBSurfaceWithFormat(
		0,
		480,
		config.screen.pixheight * 141,
		8,
		SDL_PIXELFORMAT_INDEX8);
	SDL_SetSurfaceBlendMode(winbuffer, SDL_BLENDMODE_BLEND);
	SDL_SetColorKey(winbuffer, true, 0xc5);
	SDL_Surface* mwsurface = SDL_GetWindowSurface(mainwindow);
	SDL_SetSurfaceBlendMode(mwsurface, SDL_BLENDMODE_NONE);
	SDL_Surface* restorescreen = SDL_CreateRGBSurfaceWithFormat(
		0,
		winbuffer->w,
		winbuffer->h,
		mwsurface->format->BitsPerPixel,
		SDL_MasksToPixelFormatEnum(
			mwsurface->format->BitsPerPixel,
			mwsurface->format->Rmask,
			mwsurface->format->Gmask,
			mwsurface->format->Bmask,
			mwsurface->format->Amask));
	if (!winbuffer || !restorescreen)
	{
		SDL_Log("Could not create menu display buffers: %s", SDL_GetError());
		return;
	}
	SDL_SetSurfaceBlendMode(restorescreen, SDL_BLENDMODE_NONE);
	SDL_Rect winpos;
	winpos.w = winbuffer->w;
	winpos.h = winbuffer->h;
	SDL_GetWindowSize(mainwindow, &winpos.x, &winpos.y);
	int uwinpos = 2;
	winpos.x -= winpos.w;
	winpos.y -= winpos.h;
	winpos.x >>= 1;
	winpos.y >>= 1;
	SetHWPalette(winbuffer->format->palette);
	SDL_UpdateWindowSurface(mainwindow);
	SDL_BlitSurface(mwsurface, &winpos, restorescreen, NULL);
	PaintWindow(winbuffer, 0xff, 0x0b, "Debug Log", 0xff);
	const char* keyshint = "F: Find  \x97  Left/Right: Move Window  \x97  0/Esc: Close Log";
	int tx0 = (winpos.w - TextWidth(keyshint)) >> 1;
	DrawText(keyshint, tx0, winpos.h - 20, 0x03, 0xff, winbuffer);
	int displaylines = (winpos.h - 40) / 16;
	int dy0 = (winpos.h - displaylines * 16) >> 1;
	int displaystart = debuglogend - displaylines - 1;
	SDL_Rect animrect = winpos;
	SDL_Rect menurect = { 0, 0, winpos.w, winpos.h };
	for (int t = 1; t <= 8; ++t)
	{
		SDL_SetSurfaceAlphaMod(winbuffer, t * 31);
		animrect.w = winpos.w * t / 8;
		menurect.w = animrect.w;
		animrect.x = (mwsurface->w - animrect.w) >> 1;
		SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
		SDL_BlitSurface(winbuffer, &menurect, mwsurface, &animrect);
		SDL_UpdateWindowSurface(mainwindow);
		SDL_Delay(16);
	}
	bool repaintlog = true;
	while (true)
	{
		SDL_Event event;
		int newuwinpos = uwinpos;
		bool repaintscreen = true;
		while (SDL_PollEvent(&event))
		{
			SDL_Event newevent;
			switch (event.type)
			{
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
				case SDLK_0:
				case SDLK_KP_0:
				case SDLK_ESCAPE:
					goto exitdebugger;
				case SDLK_HOME:
					displaystart = debuglogstart;
					repaintlog = true;
					break;
				case SDLK_END:
					displaystart = debuglogend - displaylines - 1;
					repaintlog = true;
					break;
				case SDLK_PAGEUP:
					if (((displaystart - debuglogstart) & dlogidxmask) < displaylines)
					{
						UIBeepUnavailable();
					}
					else
					{
						displaystart -= displaylines;
						repaintlog = true;
					}
					break;
				case SDLK_PAGEDOWN:
					if (((debuglogend - displaystart - displaylines) & dlogidxmask) < displaylines)
					{
						UIBeepUnavailable();
					}
					else
					{
						displaystart += displaylines;
						repaintlog = true;
					}
					break;
				case SDLK_UP:
					if (((displaystart - debuglogstart) & dlogidxmask) == 0)
					{
						UIBeepUnavailable();
					}
					else
					{
						--displaystart;
						repaintlog = true;
					}
					break;
				case SDLK_DOWN:
					if (((debuglogend - displaystart - displaylines) & dlogidxmask) == 0)
					{
						UIBeepUnavailable();
					}
					else
					{
						++displaystart;
						repaintlog = true;
					}
					break;
				case SDLK_LEFT:
					if (uwinpos <= 0)
					{
						UIBeepUnavailable();
					}
					else
					{
						--newuwinpos;
					}
					break;
				case SDLK_RIGHT:
					if (uwinpos >= 4)
					{
						UIBeepUnavailable();
					}
					else
					{
						++newuwinpos;
					}
					break;
				case SDLK_TAB:
					if (event.key.keysym.mod & KMOD_SHIFT)
					{
						UIBeepUnavailable();
					}
					else
					{
						UIBeepUnavailable();
					}
					break;
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					UIBeepUnavailable();
					break;
				}
				break;
			case SDL_QUIT:
				newevent.type = SDL_QUIT;
				SDL_PushEvent(&newevent);
				goto exitdebugger;
			}
		}
		if (repaintlog)
		{
			SDL_Rect boxrect = { 2, 19, winpos.w - 4, winpos.h - 39 };
			SDL_FillRect(winbuffer, &boxrect, 0xff);
			div_t dr;
			for (int i = displaystart, j = 0, y = dy0; j < displaylines; ++i, ++j, y += 16)
			{
				dlogentry* entry = debuglog + (i & dlogidxmask);
				constexpr unsigned char CPUFLAG_CARRY     = 0x01;
				constexpr unsigned char CPUFLAG_ZERO      = 0x02;
				constexpr unsigned char CPUFLAG_INTERRUPT = 0x04;
				constexpr unsigned char CPUFLAG_DECIMAL   = 0x08;
				constexpr unsigned char CPUFLAG_BREAK     = 0x10;
				constexpr unsigned char CPUFLAG_OVERFLOW  = 0x40;
				constexpr unsigned char CPUFLAG_NEGATIVE  = 0x80;
				switch (entry->entrytype)
				{
					char line[45];
				case dletype::LT_START:
					strcpy_s(line, 12, "<log start>");
					tx0 = (winpos.w - TextWidth(line)) >> 1;
					DrawText(line, tx0, y, 0x72, 0xff, winbuffer);
					break;
				case dletype::LT_READ:
					strcpy_s(line, 21, "Read --- $---- = $--");
					DisplayMemType(line + 5, entry->mementry.memtype);
					DisplayHexWord(line + 10, entry->mementry.address);
					DisplayHexByte(line + 18, entry->mementry.value);
					tx0 = (winpos.w - TextWidth(line)) >> 1;
					DrawText(line, tx0, y, 0x20, 0xff, winbuffer);
					break;
				case dletype::LT_WRITE:
					strcpy_s(line, 22, "Write --- $---- = $--");
					DisplayMemType(line + 6, entry->mementry.memtype);
					DisplayHexWord(line + 11, entry->mementry.address);
					DisplayHexByte(line + 19, entry->mementry.value);
					tx0 = (winpos.w - TextWidth(line)) >> 1;
					DrawText(line, tx0, y, 0x24, 0xff, winbuffer);
					break;
				case dletype::LT_FVMC:
					strcpy_s(line, 34, "(FVMC: --- $---- -> $-- -> $----)");
					DisplayMemType(line + 7, entry->fvmcentry.memtype);
					DisplayHexWord(line + 12, entry->fvmcentry.src);
					DisplayHexWord(line + 28, entry->fvmcentry.dest);
					DisplayHexByte(line + 21, entry->fvmcentry.value);
					tx0 = (winpos.w - TextWidth(line)) >> 1;
					DrawText(line, tx0, y, 0x68, 0xff, winbuffer);
					break;
				case dletype::LT_STATE:
					strcpy_s(line, 35, "PC=$---- ------- A=$-- X=$-- Y=$--");
					DisplayHexWord(line + 4, entry->stateentry.pc);
					if (entry->stateentry.status & CPUFLAG_NEGATIVE)
						line[9] = 'N';
					if (entry->stateentry.status & CPUFLAG_OVERFLOW)
						line[10] = 'V';
					if (entry->stateentry.status & CPUFLAG_BREAK)
						line[11] = 'B';
					if (entry->stateentry.status & CPUFLAG_DECIMAL)
						line[12] = 'D';
					if (entry->stateentry.status & CPUFLAG_INTERRUPT)
						line[13] = 'I';
					if (entry->stateentry.status & CPUFLAG_ZERO)
						line[14] = 'Z';
					if (entry->stateentry.status & CPUFLAG_CARRY)
						line[15] = 'C';
					DisplayHexByte(line + 20, entry->stateentry.a);
					DisplayHexByte(line + 26, entry->stateentry.x);
					DisplayHexByte(line + 32, entry->stateentry.y);
					tx0 = (winpos.w - TextWidth(line)) >> 1;
					DrawText(line, tx0, y, 0x03, 0xff, winbuffer);
					break;
				case dletype::LT_STACK:
					strcpy_s(line, 37, "Stack: $-- $-- $-- $-- $-- $-- $-- \205");
					DisplayHexByte(line + 8, entry->stackentry.stack[0]);
					DisplayHexByte(line + 12, entry->stackentry.stack[1]);
					DisplayHexByte(line + 16, entry->stackentry.stack[2]);
					DisplayHexByte(line + 20, entry->stackentry.stack[3]);
					DisplayHexByte(line + 24, entry->stackentry.stack[4]);
					DisplayHexByte(line + 28, entry->stackentry.stack[5]);
					DisplayHexByte(line + 32, entry->stackentry.stack[6]);
					if (entry->stackentry.nstack <= 7)
						line[6 + entry->stackentry.nstack * 4] = 0;
					tx0 = (winpos.w - TextWidth(line)) >> 1;
					DrawText(line, tx0, y, 0x21, 0xff, winbuffer);
					break;
				case dletype::LT_INST:
					strcpy_s(line, 23, "--- -------\0\0\0\0\0\0\0\0\0\0\0");
					DisplayOpcode(line, entry->instentry.opc);
					char* tpos;
					DisplayArgs(line + 4, entry->instentry.opc, (char*)&entry->instentry.arg, &tpos);
					*tpos = '(';
					*++tpos = '0' + entry->instentry.cycles;
					*++tpos = ' ';
					*++tpos = 'c';
					*++tpos = 'y';
					*++tpos = 'c';
					*++tpos = 'l';
					*++tpos = 'e';
					*++tpos = 's';
					*++tpos = ')';
					*++tpos = 0;
					tx0 = (winpos.w - TextWidth(line)) >> 1;
					DrawText(line, tx0, y, 0x11, 0xff, winbuffer);
					break;
				case dletype::LT_PARTIALINST:
					strcpy_s(line, 26, "<ready to read next inst>");
					tx0 = (winpos.w - TextWidth(line)) >> 1;
					DrawText(line, tx0, y, 0x79, 0xff, winbuffer);
					break;
				case dletype::LT_RESET:
					strcpy_s(line, 12, "<CPU reset>");
					tx0 = (winpos.w - TextWidth(line)) >> 1;
					DrawText(line, tx0, y, 0x41, 0xff, winbuffer);
					break;
				case dletype::LT_SCANLINE:
					strcpy_s(line, 21, "<begin scanline --->");
					dr = div(entry->scanline, 10);
					line[18] = '0' + dr.rem;
					dr = div(dr.quot, 10);
					line[17] = '0' + dr.rem;
					line[16] = '0' + dr.quot;
					tx0 = (winpos.w - TextWidth(line)) >> 1;
					DrawText(line, tx0, y, 0xd5, 0xff, winbuffer);
					break;
				case dletype::LT_BORING:
					if (entry->boringentry.complete)
					{
						strcpy_s(line, 45, "<selective suppression for --------- cycles>");
						{
							unsigned long clockdiff = entry->boringentry.exitclock - entry->boringentry.entryclock;
							dr = div(clockdiff, 10);
							line[35] = '0' + dr.rem;
							dr = div(dr.quot, 10);
							line[34] = '0' + dr.rem;
							dr = div(dr.quot, 10);
							line[33] = '0' + dr.rem;
							dr = div(dr.quot, 10);
							line[32] = '0' + dr.rem;
							dr = div(dr.quot, 10);
							line[31] = '0' + dr.rem;
							dr = div(dr.quot, 10);
							line[30] = '0' + dr.rem;
							dr = div(dr.quot, 10);
							line[29] = '0' + dr.rem;
							dr = div(dr.quot, 10);
							line[28] = '0' + dr.rem;
							line[27] = '0' + dr.quot;
						}
						for (int i = 27; i < 35 && line[i] == '0'; ++i)
							line[i] = 0x7f;
					}
					else
					{
						strcpy_s(line, 31, "<selective suppression active>");
					}
					tx0 = (winpos.w - TextWidth(line)) >> 1;
					DrawText(line, tx0, y, 0x07, 0xff, winbuffer);
					break;
				default:
					strcpy_s(line, 28, "<unknown log entry type -->");
					DisplayHexByte(line + 24, (char)entry->entrytype);
					tx0 = (winpos.w - TextWidth(line)) >> 1;
					DrawText(line, tx0, y, 0x07, 0xff, winbuffer);
					break;
				}
			}
			repaintscreen = true;
			repaintlog = false;
		}
		if (newuwinpos != uwinpos)
		{
			uwinpos = newuwinpos;
			SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
			SDL_GetWindowSize(mainwindow, &winpos.x, NULL);
			winpos.x -= winpos.w;
			winpos.x *= uwinpos;
			winpos.x >>= 2;
			SDL_BlitSurface(mwsurface, &winpos, restorescreen, NULL);
			repaintscreen = true;
		}
		if (repaintscreen)
		{
			SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
			SDL_BlitSurface(winbuffer, NULL, mwsurface, &winpos);
			SDL_UpdateWindowSurface(mainwindow);
		}
		SDL_Delay(16);
	}
exitdebugger:
	animrect.x = winpos.x;
	for (int t = 7; t > 0; --t)
	{
		SDL_SetSurfaceAlphaMod(winbuffer, t * 31);
		animrect.h = winpos.h * t / 8;
		menurect.h = animrect.h;
		menurect.y = winpos.h - animrect.h;
		animrect.y = (mwsurface->h - animrect.h) >> 1;
		SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
		SDL_BlitSurface(winbuffer, &menurect, mwsurface, &animrect);
		SDL_UpdateWindowSurface(mainwindow);
		SDL_Delay(16);
	}
	SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
	SDL_FreeSurface(restorescreen);
	SDL_FreeSurface(winbuffer);
}

void DoMenu(int quickoption)
{
	SDL_Surface* menubuffer = SDL_CreateRGBSurfaceWithFormat(
		0,
		240,
		224,
		8,
		SDL_PIXELFORMAT_INDEX8);
	SDL_SetSurfaceBlendMode(menubuffer, SDL_BLENDMODE_BLEND);
	SDL_SetColorKey(menubuffer, true, 0xc5);
	SDL_Surface* mwsurface = SDL_GetWindowSurface(mainwindow);
	SDL_SetSurfaceBlendMode(mwsurface, SDL_BLENDMODE_NONE);
	SDL_Surface* restorescreen = SDL_CreateRGBSurfaceWithFormat(
		0,
		menubuffer->w,
		menubuffer->h,
		mwsurface->format->BitsPerPixel,
		SDL_MasksToPixelFormatEnum(
			mwsurface->format->BitsPerPixel,
			mwsurface->format->Rmask,
			mwsurface->format->Gmask,
			mwsurface->format->Bmask,
			mwsurface->format->Amask));
	if (!menubuffer || !restorescreen)
	{
		SDL_Log("Could not create menu display buffers: %s", SDL_GetError());
		return;
	}
	SDL_SetSurfaceBlendMode(restorescreen, SDL_BLENDMODE_NONE);
	SDL_Rect winpos;
	winpos.w = menubuffer->w;
	winpos.h = menubuffer->h;
	SDL_GetWindowSize(mainwindow, &winpos.x, &winpos.y);
	winpos.x -= winpos.w;
	winpos.y -= winpos.h;
	winpos.x >>= 1;
	winpos.y >>= 2;
	SetHWPalette(menubuffer->format->palette);
	SDL_UpdateWindowSurface(mainwindow);
	SDL_BlitSurface(mwsurface, &winpos, restorescreen, NULL);
	PaintWindow(menubuffer, 0xff, 0x3d, "Emulator Menu", 0xff);
	const char* itemtext[9] = {
		"1: Options",
		"2: Screen Capture",
		"3: Debugging Tools",
		"4: Insert Cartridge",
		"6: Documentation",
		"7: Reset Computer",
		"8: About Retro 6k",
		"9: Exit Emulator",
		"0: Close Menu",
	};
	if (cartridgeinserted)
		itemtext[3] = "5: Remove Cartridge";
	unsigned char itemcolor[9] = {
		0x06,
		0x02,
		0x03,
		0x31,
		0x11,
		0x15,
		0x45,
		0x44,
		0x64,
	};
	int itemy0[9];
	int selecteditem = 8;
	if (quickoption) {
		if (quickoption < 4)
		{
			selecteditem = quickoption - 1;
		}
		else if (quickoption == 4)
		{
			if (cartridgeinserted)
			{
				selecteditem = 8;
				quickoption = 0;
			}
			else
			{
				selecteditem = 3;
			}
		}
		else if (quickoption == 5)
		{
			if (cartridgeinserted)
			{
				selecteditem = 3;
			}
			else
			{
				selecteditem = 8;
				quickoption = 0;
			}
		}
		else
		{
			selecteditem = quickoption - 2;
		}
	}
	int maxitemwidth = 0;
	for (int i = 0; i < 9; ++i)
	{
		int itemwidth = TextWidth(itemtext[i]);
		if (itemwidth > maxitemwidth)
			maxitemwidth = itemwidth;
	}
	const int semicircw[18] = { 3, 5, 6, 7, 8, 8, 9, 9, 9, 9, 9, 9, 8, 8, 7, 6, 5, 3 };
	int ix0 = (menubuffer->w - maxitemwidth) >> 1;
	for (int i = 0, y0 = 32; i < 9; ++i, y0 += 20)
	{
		int x0 = ix0;
		itemy0[i] = y0;
		if (selecteditem == i)
		{
			SDL_Rect selrect = { ix0, y0 - 1, maxitemwidth, 18 };
			SDL_FillRect(menubuffer, &selrect, itemcolor[i]);
			DrawText(itemtext[i], x0, y0, 0xff, itemcolor[i], menubuffer);
			selrect.h = 1;
			for (int y = 0; y < 18; ++y)
			{
				selrect.y = y0 + y - 1;
				selrect.w = semicircw[y];
				selrect.x = ix0 - selrect.w;
				SDL_FillRect(menubuffer, &selrect, itemcolor[i]);
				selrect.x = ix0 + maxitemwidth;
				SDL_FillRect(menubuffer, &selrect, itemcolor[i]);
			}
		}
		else
		{
			SDL_Rect selrect = { ix0 - 9, y0 - 1, maxitemwidth + 18, 18 };
			SDL_FillRect(menubuffer, &selrect, 0xff);
			DrawText(itemtext[i], x0, y0, itemcolor[i], 0xff, menubuffer);
		}
	}
	unsigned short restorefreq[4];
	restorefreq[0] = soundfreqregister[0];
	restorefreq[1] = soundfreqregister[1];
	restorefreq[2] = soundfreqregister[2];
	restorefreq[3] = soundfreqregister[3];
	SDL_Rect animrect = winpos;
	SDL_Rect menurect = { 0, 0, winpos.w, winpos.h };
	for (int t = 1; t <= 8; ++t)
	{
		SDL_SetSurfaceAlphaMod(menubuffer, t * 31);
		animrect.w = winpos.w * t / 8;
		menurect.w = animrect.w;
		animrect.x = (mwsurface->w - animrect.w) >> 1;
		SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
		SDL_BlitSurface(menubuffer, &menurect, mwsurface, &animrect);
		SDL_UpdateWindowSurface(mainwindow);
		soundfreqregister[0] = restorefreq[0] * (8 - t) / 8;
		soundfreqregister[1] = restorefreq[1] * (8 - t) / 8;
		soundfreqregister[2] = restorefreq[2] * (8 - t) / 8;
		soundfreqregister[3] = restorefreq[3] * (8 - t) / 8;
		SDL_Delay(16);
	}
	menuhassound = true;
	//SDL_UpdateWindowSurface(mainwindow);
	while (true)
	{
		int newselection = selecteditem;
		SDL_Event event;
		bool activateselection = quickoption;
		quickoption = 0;
		while (SDL_PollEvent(&event))
		{
			SDL_Event newevent;
			switch (event.type)
			{
			case SDL_KEYDOWN:
				switch (event.key.keysym.sym)
				{
				case SDLK_1:
				case SDLK_KP_1:
					newselection = 0;
					activateselection = true;
					break;
				case SDLK_2:
				case SDLK_KP_2:
					newselection = 1;
					activateselection = true;
					break;
				case SDLK_3:
				case SDLK_KP_3:
					newselection = 2;
					activateselection = true;
					break;
				case SDLK_4:
				case SDLK_KP_4:
					if (!cartridgeinserted)
					{
						newselection = 3;
						activateselection = true;
					}
					else
					{
						UIBeepUnavailable();
					}
					break;
				case SDLK_5:
				case SDLK_KP_5:
					if (cartridgeinserted)
					{
						newselection = 3;
						activateselection = true;
					}
					else
					{
						UIBeepUnavailable();
					}
					break;
				case SDLK_6:
				case SDLK_KP_6:
					newselection = 4;
					activateselection = true;
					break;
				case SDLK_7:
				case SDLK_KP_7:
					newselection = 5;
					activateselection = true;
					break;
				case SDLK_8:
				case SDLK_KP_8:
					newselection = 6;
					activateselection = true;
					break;
				case SDLK_9:
				case SDLK_KP_9:
					newselection = 7;
					activateselection = true;
					break;
				case SDLK_0:
				case SDLK_KP_0:
					newselection = 8;
					activateselection = true;
					break;
				case SDLK_ESCAPE:
					goto exitmenu;
				case SDLK_UP:
				case SDLK_LEFT:
					--newselection;
					if (newselection < 0)
						newselection = 8;
					break;
				case SDLK_DOWN:
				case SDLK_RIGHT:
					++newselection;
					if (newselection > 8)
						newselection = 0;
					break;
				case SDLK_TAB:
					if (event.key.keysym.mod & KMOD_SHIFT)
					{
					--newselection;
					if (newselection < 0)
						newselection = 8;
					}
					else
					{
					++newselection;
					if (newselection > 8)
						newselection = 0;
					}
					break;
				case SDLK_RETURN:
				case SDLK_KP_ENTER:
					activateselection = true;
					break;
				}
				break;
			case SDL_QUIT:
				newevent.type = SDL_QUIT;
				SDL_PushEvent(&newevent);
				goto exitmenu;
			}
		}
		if (newselection != selecteditem)
		{
			UIBeepMoveSel();
			int i = newselection;
			int y0 = itemy0[i];
			int x0 = ix0;
			SDL_Rect selrect = { ix0, y0 - 1, maxitemwidth, 18 };
			SDL_FillRect(menubuffer, &selrect, itemcolor[i]);
			DrawText(itemtext[i], x0, y0, 0xff, itemcolor[i], menubuffer);
			selrect.h = 1;
			for (int y = 0; y < 18; ++y)
			{
				selrect.y = y0 + y - 1;
				selrect.w = semicircw[y];
				selrect.x = ix0 - selrect.w;
				SDL_FillRect(menubuffer, &selrect, itemcolor[i]);
				selrect.x = ix0 + maxitemwidth;
				SDL_FillRect(menubuffer, &selrect, itemcolor[i]);
			}
			i = selecteditem;
			y0 = itemy0[i];
			selrect = { ix0 - 9, y0 - 1, maxitemwidth + 18, 18 };
			SDL_FillRect(menubuffer, &selrect, 0xff);
			x0 = ix0;
			DrawText(itemtext[i], x0, y0, itemcolor[i], 0xff, menubuffer);
			selecteditem = newselection;
		}
		SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
		SDL_BlitSurface(menubuffer, &menurect, mwsurface, &animrect);
		SDL_UpdateWindowSurface(mainwindow);
		if (activateselection)
		{
			switch (selecteditem)
			{
				SDL_Event newevent;
			case 0:
				// Options
				UIBeepUnavailable();
				break;
			case 1:
				// Screen Capture
				UIBeepUnavailable();
				break;
			case 2:
				// Debugging Tools
				UIBeepTakeAction();
				SDL_SetSurfaceAlphaMod(menubuffer, 0xaa);
				SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
				SDL_BlitSurface(menubuffer, &menurect, mwsurface, &animrect);
				DoDebugger();
				SDL_SetSurfaceAlphaMod(menubuffer, 0xf8);
				SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
				SDL_BlitSurface(menubuffer, &menurect, mwsurface, &animrect);
				SDL_UpdateWindowSurface(mainwindow);
				break;
			case 3:
				if (cartridgeinserted)
				{
					// Remove Cartridge
					UIBeepTakeAction();
					EjectCartridge();
					goto exitmenu;
				}
				else
				{
					// Insert Cartridge
					UIBeepTakeAction();
					SDL_SetSurfaceAlphaMod(menubuffer, 0xaa);
					SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
					SDL_BlitSurface(menubuffer, &menurect, mwsurface, &animrect);
					PickAndInsertCartridge();
					SDL_SetSurfaceAlphaMod(menubuffer, 0xf8);
					SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
					SDL_BlitSurface(menubuffer, &menurect, mwsurface, &animrect);
					SDL_UpdateWindowSurface(mainwindow);
					goto exitmenu;
				}
				break;
			case 4:
				// Documentation
				UIBeepUnavailable();
				break;
			case 5:
				// Reset
				UIBeepTakeAction();
				newevent.type = UE_RESETCPU;
				SDL_PushEvent(&newevent);
				goto exitmenu;
			case 6:
				// About
				UIBeepTakeAction();
				SDL_SetSurfaceAlphaMod(menubuffer, 0xaa);
				SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
				SDL_BlitSurface(menubuffer, &menurect, mwsurface, &animrect);
				DoAbout();
				SDL_SetSurfaceAlphaMod(menubuffer, 0xf8);
				SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
				SDL_BlitSurface(menubuffer, &menurect, mwsurface, &animrect);
				SDL_UpdateWindowSurface(mainwindow);
				break;
			case 7:
				// Exit
				newevent.type = SDL_QUIT;
				SDL_PushEvent(&newevent);
				UIBeepTakeAction();
				restorefreq[0] = 0;
				restorefreq[1] = 0;
				restorefreq[2] = 0;
				restorefreq[3] = 0;
				goto exitmenu;
			case 8:
				// Close Menu
				goto exitmenu;
			}
		}
		SDL_Delay(16);
	}
exitmenu:
	while (menubeepdur)
		SDL_Delay(1);
	menuhassound = false;
	for (int t = 7; t > 0; --t)
	{
		SDL_SetSurfaceAlphaMod(menubuffer, t * 31);
		animrect.h = winpos.h * t / 8;
		menurect.h = animrect.h;
		menurect.y = winpos.h - animrect.h;
		animrect.y = (mwsurface->h - animrect.h) >> 2;
		SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
		SDL_BlitSurface(menubuffer, &menurect, mwsurface, &animrect);
		SDL_UpdateWindowSurface(mainwindow);
		soundfreqregister[0] = restorefreq[0] * (8 - t) / 8;
		soundfreqregister[1] = restorefreq[1] * (8 - t) / 8;
		soundfreqregister[2] = restorefreq[2] * (8 - t) / 8;
		soundfreqregister[3] = restorefreq[3] * (8 - t) / 8;
		SDL_Delay(16);
	}
	SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
	SDL_FreeSurface(restorescreen);
	SDL_FreeSurface(menubuffer);
	soundfreqregister[0] = restorefreq[0];
	soundfreqregister[1] = restorefreq[1];
	soundfreqregister[2] = restorefreq[2];
	soundfreqregister[3] = restorefreq[3];
}

void DoPickFile(filetype ft, std::string &path)
{
	SDL_Surface* winbuffer = SDL_CreateRGBSurfaceWithFormat(
		0,
		480,
		config.screen.pixheight * 109,
		8,
		SDL_PIXELFORMAT_INDEX8);
	SDL_SetSurfaceBlendMode(winbuffer, SDL_BLENDMODE_BLEND);
	SDL_SetColorKey(winbuffer, true, 0xf9);
	SDL_Surface* mwsurface = SDL_GetWindowSurface(mainwindow);
	SDL_SetSurfaceBlendMode(mwsurface, SDL_BLENDMODE_NONE);
	SDL_Surface* restorescreen = SDL_CreateRGBSurfaceWithFormat(
		0,
		winbuffer->w,
		winbuffer->h,
		mwsurface->format->BitsPerPixel,
		SDL_MasksToPixelFormatEnum(
			mwsurface->format->BitsPerPixel,
			mwsurface->format->Rmask,
			mwsurface->format->Gmask,
			mwsurface->format->Bmask,
			mwsurface->format->Amask));
	if (!winbuffer || !restorescreen)
	{
		SDL_Log("Could not create menu display buffers: %s", SDL_GetError());
		return;
	}
	SDL_SetSurfaceBlendMode(restorescreen, SDL_BLENDMODE_NONE);
	int winmacrox, winmacroy;
	const char* titles[4] = {
		"Choose a File\x85",
		"Choose a ROM Chip\x85",
		"Choose a Cartridge\x85",
		"Choose a Cartridge Save File\x85",
	};
	const char* exts[4] = {
		"",
		".rom",
		".r6kcart",
		".r6ksave",
	};
	char* wintitle;
	char* filterext;
	bool ignoreext = false;
	std::filesystem::path browsedir;
	switch (ft)
	{
	case filetype::FT_ROM:
		wintitle = (char*)titles[1];
		filterext = (char*)exts[1];
		for (auto& path : config.sessionpath.rompath)
		{
			if (std::filesystem::exists(path))
			{
				browsedir = std::filesystem::canonical(path);
				break;
			}
		}
		winmacrox = 4;
		winmacroy = 4;
		break;
	case filetype::FT_CART:
		wintitle = (char*)titles[2];
		filterext = (char*)exts[2];
		for (auto& path : config.sessionpath.cartpath)
		{
			if (std::filesystem::exists(path))
			{
				browsedir = std::filesystem::canonical(path);
				break;
			}
		}
		winmacrox = 2;
		winmacroy = 3;
		break;
	case filetype::FT_CARTSAVE:
		wintitle = (char*)titles[3];
		filterext = (char*)exts[3];
		for (auto& path : config.sessionpath.savepath)
		{
			if (std::filesystem::exists(path))
			{
				browsedir = std::filesystem::canonical(path);
				break;
			}
		}
		winmacrox = 6;
		winmacroy = 5;
		break;
	default:
		wintitle = (char*)titles[0];
		filterext = (char*)exts[0];
		ignoreext = true;
		browsedir = std::filesystem::canonical(".");
		winmacrox = 4;
		winmacroy = 4;
	}
	int mww, mwh;
	SDL_GetWindowSize(mainwindow, &mww, &mwh);
	SDL_Rect winpos;
	winpos.w = winbuffer->w;
	winpos.h = winbuffer->h;
	winpos.x = (winmacrox * (mww - winpos.w)) >> 3;
	winpos.y = (winmacroy * (mwh - winpos.h)) >> 3;
	SetHWPalette(winbuffer->format->palette);
	SDL_UpdateWindowSurface(mainwindow);
	SDL_BlitSurface(mwsurface, &winpos, restorescreen, NULL);
	PaintWindow(winbuffer, 0xff, 0x39, wintitle, 0xff);
	const char* keyshint = "Ctrl+D: ChDir  \x97  Ctrl+E: ExtFilter on/off  \x97  Esc: Cancel";
	int tx0 = (winpos.w - TextWidth(keyshint)) >> 1;
	DrawText(keyshint, tx0, winpos.h - 20, 0x31, 0xff, winbuffer);
	int displaylines = (winpos.h - 60) / 18;
	int filey0;
	filey0 = (winpos.h - displaylines * 18 + 20) >> 1;
	const int filex0 = 16;
	const int prevx0 = winpos.w - 80;
	DrawTextCX("Loading directory listing\x85", filex0, filey0 + 18 * ((displaylines - 1) >> 1), 0x07, 0xff, winbuffer);
	SDL_Rect dstanimrect = winpos;
	SDL_Rect srcanimrect = { 0, 0, winpos.w, winpos.h };
	for (int t = 1; t <= 8; ++t)
	{
		SDL_SetSurfaceAlphaMod(winbuffer, t * 31);
		dstanimrect.w = winpos.w * t / 8;
		srcanimrect.w = dstanimrect.w;
		dstanimrect.x = (winmacrox * (mww - dstanimrect.w)) >> 3;
		SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
		SDL_BlitSurface(winbuffer, &srcanimrect, mwsurface, &dstanimrect);
		SDL_UpdateWindowSurface(mainwindow);
		SDL_Delay(16);
	}
	bool relistfiles = true;
	std::vector<std::string> filelist;
	std::vector<std::filesystem::path> pathlist;
	int filesel = 0;
	int displaystart = 0;
	while (true)
	{
		bool repaintfiles = false;
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			SDL_Event newevent;
			switch (event.type)
			{
			case SDL_KEYDOWN:
				if (event.key.keysym.mod & KMOD_CTRL)
				{
					switch (event.key.keysym.sym)
					{
					case SDLK_d:
						UIBeepUnavailable();
						break;
					case SDLK_e:
						UIBeepUnavailable();
						break;
					}
				}
				else
				{
					int nfiles = (int)filelist.size();
					switch (event.key.keysym.sym)
					{
					case SDLK_HOME:
						if (filesel == 0)
						{
							UIBeepUnavailable();
						}
						else
						{
							UIBeepMoveSel();
							displaystart = 0;
							filesel = 0;
							repaintfiles = true;
						}
						break;
					case SDLK_END:
						if (filesel + 1 == nfiles)
						{
							UIBeepMoveSel();
						}
						else
						{
							UIBeepMoveSel();
							displaystart = nfiles - displaylines;
							filesel = nfiles - 1;
							if (displaystart < 0)
								displaystart = 0;
							repaintfiles = true;
						}
						break;
					case SDLK_PAGEUP:
						if (displaystart == 0 || displaylines >= nfiles)
						{
							if (filesel == 0)
							{
								UIBeepUnavailable();
							}
							else
							{
								UIBeepMoveSel();
								filesel = 0;
								repaintfiles = true;
							}
						}
						else
						{
							UIBeepMoveSel();
							displaystart -= displaylines;
							if (displaystart < 0)
								displaystart = 0;
							int lastvisible = displaystart + displaylines - 1;
							if (filesel > lastvisible)
								filesel = lastvisible;
							repaintfiles = true;
						}
						break;
					case SDLK_PAGEDOWN:
						if (displaystart + displaylines == nfiles || displaylines >= nfiles)
						{
							if (filesel == nfiles - 1)
							{
								UIBeepUnavailable();
							}
							else
							{
								filesel = nfiles - 1;
								repaintfiles = true;
							}
						}
						else
						{
							UIBeepMoveSel();
							displaystart += displaylines;
							if (displaystart + displaylines > nfiles)
								displaystart = nfiles - displaylines;
							if (filesel < displaystart)
								filesel = displaystart;
							repaintfiles = true;
						}
						break;
					case SDLK_UP:
						if (filesel == 0)
						{
							UIBeepUnavailable();
						}
						else
						{
							UIBeepMoveSel();
							--filesel;
							if (filesel < displaystart)
								displaystart = filesel;
							repaintfiles = true;
						}
						break;
					case SDLK_DOWN:
						if (filesel == nfiles - 1)
						{
							UIBeepUnavailable();
						}
						else
						{
							UIBeepMoveSel();
							++filesel;
							if (filesel >= displaystart + displaylines)
								displaystart = filesel - displaylines + 1;
							repaintfiles = true;
						}
						break;
					case SDLK_RETURN:
					case SDLK_KP_ENTER:
						path = pathlist[filesel].string();
						UIBeepTakeAction();
						goto exitpickfile;
					case SDLK_ESCAPE:
						path.clear();
						goto exitpickfile;
					}
				}
				break;
			case SDL_QUIT:
				newevent.type = SDL_QUIT;
				SDL_PushEvent(&newevent);
				goto exitpickfile;
			}
		}
		if (relistfiles)
		{
			filelist.clear();
			pathlist.clear();
			for (auto& entry: std::filesystem::directory_iterator(browsedir, std::filesystem::directory_options::skip_permission_denied))
			{
				if (entry.is_regular_file() && (ignoreext || cilstreq(entry.path().extension().string().c_str(), (const char*)filterext)))
				{
					filelist.push_back(entry.path().filename().string());
					pathlist.push_back(entry.path());
				}
			}
			repaintfiles = true;
		}
		if (repaintfiles)
		{
			SDL_Rect paintbox{filex0 - 9, filey0 - 1, prevx0 - filey0 + 9, 18 * displaylines + 2};
			{
				int x = filex0;
				DrawText("Location: ", x, 23, 0x31, 0xff, winbuffer);
				std::string loc = browsedir.string();
				DrawText(loc.c_str(), x, 23, 0x00, 0xff, winbuffer);
			}
			SDL_FillRect(winbuffer, &paintbox, 0xff);
			for (int i = 0, j = displaystart, ky = filey0; i < displaylines && j < filelist.size(); ++i, ++j, ky += 18)
			{
				if (filesel == j)
				{
					SDL_Rect selrect{ paintbox.x, ky - 1, paintbox.w, 18 };
					FillRoundedRect(winbuffer, selrect, 0x31);
					DrawTextCX(filelist[j].c_str(), filex0, ky, 0xff, 0x31, winbuffer);
				}
				else
				{
					DrawTextCX(filelist[j].c_str(), filex0, ky, 0x00, 0xff, winbuffer);
				}
			}
			if (filelist.size() == 0)
			{
				DrawTextCX("No files here!", filex0, filey0 + 18 * ((displaylines - 1) >> 1), 0x07, 0xff, winbuffer);
			}
			repaintfiles = false;
			SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
			SDL_BlitSurface(winbuffer, NULL, mwsurface, &winpos);
			SDL_UpdateWindowSurface(mainwindow);
		}
		SDL_Delay(16);
	}
exitpickfile:
	for (int t = 7; t > 0; --t)
	{
		SDL_SetSurfaceAlphaMod(winbuffer, t * 31);
		dstanimrect.h = winpos.h * t / 8;
		srcanimrect.h = dstanimrect.h;
		srcanimrect.y = winpos.h - dstanimrect.h;
		dstanimrect.y = (winmacroy * (mwh - dstanimrect.h)) >> 3;
		SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
		SDL_BlitSurface(winbuffer, &srcanimrect, mwsurface, &dstanimrect);
		SDL_UpdateWindowSurface(mainwindow);
		SDL_Delay(16);
	}
	SDL_BlitSurface(restorescreen, NULL, mwsurface, &winpos);
	SDL_FreeSurface(restorescreen);
	SDL_FreeSurface(winbuffer);
}

void DrawLogo(int x0, int y0, SDL_Surface* destsurf) {
	char* sptr = (char*)aboutlogopixels;
	char* rptr = (char*)(destsurf->pixels) + y0 * (long long)destsurf->pitch + x0;
	for (int y = 0; y < 27; ++y)
	{
		char* dptr = rptr;
		for (int x = 0; x < 29; ++x)
		{
			*dptr++ = *sptr++;
		}
		rptr += destsurf->pitch;
	}
}

void DrawText(const char* text, int &x, const int y0, unsigned char fg, unsigned char bg, SDL_Surface* destsurf)
{
	if (destsurf == nullptr)
		return;
	if (destsurf->pixels == nullptr)
		return;
	if (y0 >= destsurf->h)
		return;
	unsigned char c[2] = { bg, fg };
	int mingy = -y0;
	if (mingy < 0)
		mingy = 0;
	int maxgy = destsurf->h - y0;
	if (maxgy > 16)
		maxgy = 16;
	while (*text)
	{
		int gw = hifont[(unsigned char)*text].w;
		unsigned short gbits[16];
		for (unsigned short* src = (unsigned short*)hifont[(unsigned char)*text].bmp, * dst = gbits; src < hifont[(unsigned char)*text].bmp + 16; ++src, ++dst)
			*dst = *src;
		unsigned char* rowstart = (unsigned char*)destsurf->pixels + (long long)destsurf->pitch * y0 + x;
		int mingx = -x;
		int maxgx = destsurf->w - x;
		if (mingx < gw)
		{
			for (int gy = mingy; gy < maxgy; ++gy)
			{
				for (int gx = 0; gx < gw; ++gx)
				{
					if (gx >= mingx && gx < maxgx)
						rowstart[gx] = c[gbits[gy] & 1];
					gbits[gy] >>= 1;
				}
				rowstart += destsurf->pitch;
			}
		}
		if (maxgx <= 0)
			break;
		x += gw;
		++text;
	}
}

inline void DrawTextCX(const char* text, const int x0, const int y0, unsigned char fg, unsigned char bg, SDL_Surface* destsurf)
{
	int tx0 = x0;
	DrawText(text, tx0, y0, fg, bg, destsurf);
}

void EjectCartridge()
{
	for (int p = 0x20; p < 0xc0; ++p)
		syspflags[p] = pageflags::PF_TFLOATING;
	cartridgeinserted = false;
}

dlogentry* ExtendLog(dletype etype)
{
	if (debuglogend == debuglogstart)
	{
		++debuglogstart;
		debuglogstart &= dlogidxmask;
	}
	debuglog[debuglogstart].entrytype = dletype::LT_START;
	debuglog[debuglogend].entrytype = etype;
	dlogentry* newentry = debuglog + debuglogend;
	++debuglogend;
	debuglogend &= dlogidxmask;
	return newentry;
}

void FillRoundedRect(SDL_Surface* dst, const SDL_Rect &r, unsigned c)
{
	SDL_Rect b;
	b = { r.x + 6, r.y, r.w - 12, 1 };
	SDL_FillRect(dst, &b, c);
	b.y = r.y + r.h - 1;
	SDL_FillRect(dst, &b, c);
	b = { r.x + 4, r.y + 1, r.w - 8, 1 };
	SDL_FillRect(dst, &b, c);
	b.y = r.y + r.h - 2;
	SDL_FillRect(dst, &b, c);
	b = { r.x + 3, r.y + 2, r.w - 6, 1 };
	SDL_FillRect(dst, &b, c);
	b.y = r.y + r.h - 3;
	SDL_FillRect(dst, &b, c);
	b = { r.x + 2, r.y + 3, r.w - 4, 1 };
	SDL_FillRect(dst, &b, c);
	b.y = r.y + r.h - 4;
	SDL_FillRect(dst, &b, c);
	b = { r.x + 1, r.y + 4, r.w - 2, 2 };
	SDL_FillRect(dst, &b, c);
	b.y = r.y + r.h - 6;
	SDL_FillRect(dst, &b, c);
	b = { r.x, r.y + 6, r.w, r.h - 12 };
	SDL_FillRect(dst, &b, c);
}

void GenerateStereoAudio(void* userdata, Uint8* stream, int len) //callback to fill audio buffer
{
	if (!menuhassound)
	{
		static signed char tickcounter{ 49 }; // 900 ticks per 1s at 44.1k sample rate
		static unsigned short phase[4]{ 0, 0, 0, 0 };
		static short lev[16];
		// clipped normal-ish distribution with RMS 8; when multiplied by volume and divided by 4, RMS maxes out at 30 (equal to max vol square wave RMS) while value is clipped at +/-37
		static std::discrete_distribution<short> dis{ 240127971, 49520773, 52161640, 54569628, 56671960, 58394828, 59666729, 60422379, 60606996, 60180662, 53532056, 60180662, 60606996, 60422379, 59666729, 58394828, 56671960, 54569628, 52161640, 49520773, 240127971 };
		for (int v = 0; v < 4; ++v)
		{
			switch ((voicetype)((soundvoicetyperegister >> (v << 1)) & 0x3))
			{
			case voicetype::VT_SQUARE:
				lev[(v << 2)    ] = -(soundvolregister[v] & (short)0xf0) >> 3;
				lev[(v << 2) | 1] = (soundvolregister[v] & (short)0xf0) >> 3;
				lev[(v << 2) | 2] = -(soundvolregister[v] & (short)0xf) << 1;
				lev[(v << 2) | 3] = (soundvolregister[v] & (short)0xf) << 1;
				break;
			case voicetype::VT_NOISE:
				// retain existing levels
				break;
			default:
				// undefined
				lev[(v << 2)    ] = 0;
				lev[(v << 2) | 1] = 0;
				lev[(v << 2) | 2] = 0;
				lev[(v << 2) | 3] = 0;
			}
			if (!soundfreqregister[v])
			{
				lev[v << 2    ] = 0;
				lev[v << 2 | 1] = 0;
				lev[v << 2 | 2] = 0;
				lev[v << 2 | 3] = 0;
			}
		}
		int t = soundspec.freq >> 1;
		for (; len > 0; len -= 2)
		{
			--tickcounter;
			if (tickcounter < 0)
			{
				tickcounter = 49;
				StepSoundSchedules();
			}
			phase[0] += soundfreqregister[0];
			phase[1] += soundfreqregister[1];
			phase[2] += soundfreqregister[2];
			phase[3] += soundfreqregister[3];
			if (phase[0] > soundspec.freq)
			{
				phase[0] -= soundspec.freq;
				if ((voicetype)(soundvoicetyperegister & 0x3) == voicetype::VT_NOISE)
				{ // on falling edge of square wave, sample and hold new random value
					short r = dis(noisegen) - 10;
					lev[0] = (r * ((soundvolregister[0] & (short)0xf0) >> 4) + 1) >> 2;
					lev[1] = lev[0];
					lev[2] = (r * (soundvolregister[0] & (short)0xf) + 1) >> 2;
					lev[3] = lev[2];
				}
			}
			if (phase[1] > soundspec.freq)
			{
				phase[1] -= soundspec.freq;
				if ((voicetype)((soundvoicetyperegister >> 2) & 0x3) == voicetype::VT_NOISE)
				{ // on falling edge of square wave, sample and hold new random value
					short r = dis(noisegen) - 10;
					lev[4] = (r * ((soundvolregister[1] & (short)0xf0) >> 4) + 1) >> 2;
					lev[5] = lev[0];
					lev[6] = (r * (soundvolregister[1] & (short)0xf) + 1) >> 2;
					lev[7] = lev[2];
				}
			}
			if (phase[2] > soundspec.freq)
			{
				phase[2] -= soundspec.freq;
				if ((voicetype)((soundvoicetyperegister >> 4) & 0x3) == voicetype::VT_NOISE)
				{ // on falling edge of square wave, sample and hold new random value
					short r = dis(noisegen) - 10;
					lev[8] = (r * ((soundvolregister[2] & (short)0xf0) >> 4) + 1) >> 2;
					lev[9] = lev[0];
					lev[10] = (r * (soundvolregister[2] & (short)0xf) + 1) >> 2;
					lev[11] = lev[2];
				}
			}
			if (phase[3] > soundspec.freq)
			{
				phase[3] -= soundspec.freq;
				if ((voicetype)((soundvoicetyperegister >> 6) & 0x3) == voicetype::VT_NOISE)
				{ // on falling edge of square wave, sample and hold new random value
					short r = dis(noisegen) - 10;
					lev[12] = (r * ((soundvolregister[3] & (short)0xf0) >> 4) + 1) >> 2;
					lev[13] = lev[0];
					lev[14] = (r * (soundvolregister[3] & (short)0xf) + 1) >> 2;
					lev[15] = lev[2];
				}
			}
			*stream = soundspec.silence
				+ lev[     (int)(phase[0] >= t)]
				- lev[4  | (int)(phase[1] >= t)]
				+ lev[8  | (int)(phase[2] >= t)]
				- lev[12 | (int)(phase[3] >= t)];
			++stream;
			*stream = soundspec.silence
				+ lev[2  | (int)(phase[0] >= t)]
				- lev[6  | (int)(phase[1] >= t)]
				+ lev[10 | (int)(phase[2] >= t)]
				- lev[14 | (int)(phase[3] >= t)];
			++stream;
		}
	}
	else
	{
		static unsigned short phase{ 0 };
		short lev[2]{ -13, 13 };
		int t = soundspec.freq >> 1;
		if (!menubeepfreq || !menubeepdur)
		{
			lev[0] = 0;
			lev[1] = 0;
		}
		for (; len > 0; len -= 2)
		{
			if (menubeepdur)
			{
				phase += menubeepfreq;
				--menubeepdur;
			}
			if (phase > soundspec.freq)
				phase -= soundspec.freq;
			*stream = soundspec.silence + lev[(int)(phase >= t)];
			++stream;
			*stream = soundspec.silence + lev[(int)(phase >= t)];
			++stream;
		}
	}
}

extern "C" long GetClock()
{
	extern uint32_t clockticks6502;
	return clockticks6502;
}

extern "C" unsigned char GetSP()
{
	extern uint8_t sp;
	return sp;
}

int InitEmulator()
{
	for (int y = 0; y < 18; ++y)
		for (int x = 0; x < 32; ++x)
		{
			PaintCell(x, y);
		}
	debuglog[debuglogstart].entrytype = dletype::LT_START;
	hookexternal((void*)LogStep);
	keypressregister = 0;
	{
		unsigned int uebase = SDL_RegisterEvents(1);
		UE_RESETCPU = uebase;
	}
	{
		SDL_Event newevent;
		newevent.type = UE_RESETCPU;
		SDL_PushEvent(&newevent);
	}
	soundfreqregister[0] = 0;
	soundfreqregister[1] = 0;
	soundfreqregister[2] = 0;
	soundfreqregister[3] = 0;
	soundvolregister[0] = 0x00;
	soundvolregister[1] = 0x00;
	soundvolregister[2] = 0x00;
	soundvolregister[3] = 0x00;
	return 0;
}

int InitMainWindow()
{
	SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER);
	SDL_DisplayMode desktopmode;
	SDL_GetDesktopDisplayMode(0, &desktopmode);
	config.screen.pixwidth = config.screen.specpixwidth;
	config.screen.pixheight = config.screen.specpixheight;
	switch (config.screen.specaspectratio)
	{
	case aspectratiocat::AR_WIDE:
		config.screen.reportedaspectratio = aspectratiocat::AR_WIDE;
		videomode |= 0x80;
		if (config.screen.pixwidth == -1)
		{
			if (config.screen.pixheight == -1)
			{
				int autowidth = (desktopmode.w - 64) / 256;
				int autoheight = (desktopmode.h - 64) / 144;
				if (autowidth >= autoheight)
				{
					config.screen.pixheight = autoheight;
					config.screen.pixwidth = autoheight;
				}
				else
				{
					config.screen.pixwidth = autowidth;
					config.screen.pixheight = autowidth;
				}
			}
			else
			{
				config.screen.pixwidth = config.screen.pixheight;
			}
		}
		else
		{
			if (config.screen.pixheight == -1)
			{
				config.screen.pixheight = config.screen.pixwidth;
			}
		}
		break;
	case aspectratiocat::AR_CLASSIC:
		config.screen.reportedaspectratio = aspectratiocat::AR_CLASSIC;
		videomode &= 0x7f;
		if (config.screen.pixwidth == -1)
		{
			if (config.screen.pixheight == -1)
			{
				int autowidth = (desktopmode.w - 64) / 256;
				int autoheight = (desktopmode.h - 64) / 144;
				if (autowidth * 4 >= autoheight * 3)
				{
					config.screen.pixheight = autoheight;
					config.screen.pixwidth = (autoheight * 3 + 2) / 4;
				}
				else
				{
					config.screen.pixwidth = autowidth;
					config.screen.pixheight = (autowidth * 4 + 1) / 3;
				}
			}
			else
			{
				config.screen.pixwidth = (config.screen.pixheight * 3 + 2) / 4;
			}
		}
		else
		{
			if (config.screen.pixheight == -1)
			{
				config.screen.pixheight = (config.screen.pixwidth * 4 + 1) / 3;
			}
		}
		break;
	case aspectratiocat::AR_FREE:
	default:
		if (config.screen.pixwidth == -1)
			config.screen.pixwidth = (desktopmode.w - 64) / 256;
		if (config.screen.pixheight == -1)
			config.screen.pixheight = (desktopmode.h - 64) / 144;
		if (config.screen.pixwidth * config.screen.pixwidth * 4
			> config.screen.pixheight* config.screen.pixheight * 3)
		{
			config.screen.reportedaspectratio = aspectratiocat::AR_WIDE;
			videomode |= 0x80;
		}
		else
		{
			config.screen.reportedaspectratio = aspectratiocat::AR_CLASSIC;
			videomode &= 0x7f;
		}
		break;
	}
	if (config.screen.pixwidth < 1)
		config.screen.pixwidth = 1;
	if (config.screen.pixheight < 1)
		config.screen.pixheight = 1;
	mainwindow = SDL_CreateWindow("Retro 6k Emulator",
		SDL_WINDOWPOS_UNDEFINED,
		SDL_WINDOWPOS_UNDEFINED,
		config.screen.pixwidth * 256,
		config.screen.pixheight * 144,
		0);
	if (!mainwindow)
	{
		SDL_Log("Could not create a window: %s", SDL_GetError());
		return -1;
	}
	SDL_Surface* mwsurface = SDL_GetWindowSurface(mainwindow);
	framebuffer = SDL_CreateRGBSurfaceWithFormat(
		0,
		config.screen.pixwidth * 256,
		config.screen.pixheight * 144,
		mwsurface->format->BitsPerPixel,
		SDL_MasksToPixelFormatEnum(
			mwsurface->format->BitsPerPixel,
			mwsurface->format->Rmask,
			mwsurface->format->Gmask,
			mwsurface->format->Bmask,
			mwsurface->format->Amask));
	if (!framebuffer)
	{
		SDL_Log("Could not create frame buffer: %s", SDL_GetError());
		return -1;
	}
	cellcanvas = SDL_CreateRGBSurface(0, config.screen.pixwidth * 8, config.screen.pixheight * 8, 8, 0, 0, 0, 0);
	if (!cellcanvas)
	{
		SDL_Log("Could not create cell canvas: %s", SDL_GetError());
		return -1;
	}
	SetHWPalette(cellcanvas->format->palette);
	return 0;
}

int InitMemory()
{
	sysram = new unsigned char[0x10000];
	syspflags = new pageflags[0x100];
	if (!sysram || !syspflags)
	{
		SDL_Log("Could not allocate system address space");
		return -1;
	}
	for (int p = 0x00; p < 0x20; ++p)
		syspflags[p] = pageflags::PF_TRAM;
	{ // Initialize the builtin RAM with deterministic random data
		std::mt19937 gen(1); //TODO: seed with a hash of machine identifying info
		std::uniform_int_distribution<> dis(0x00, 0xFF);
		for (int a = 0x0000; a < 0x2000; ++a)
			sysram[a] = dis(gen);
	}
	for (int p = 0x20; p < 0x100; ++p)
		syspflags[p] = pageflags::PF_TFLOATING;
	syspflags[0x02] = pageflags::PF_INONLY;
	syspflags[0x03] = pageflags::PF_OUTONLY;
	floatgen.seed(seedgen());
	//InitVideoMem(); // no cheating!
	InstallROM();
	for (int p = 0xf0; p < 0x100; ++p)
		syspflags[p] = syspflags[p] | pageflags::PF_DONTLOG;
	return 0;
}

int InitPaths()
{
	{ // find executable file location
#ifdef WINDOWS
		TCHAR rtnbuf[MAX_PATH];
		DWORD rtnlen = GetModuleFileName(NULL, rtnbuf, MAX_PATH);
		if (rtnlen < 0)
		{
			rtnbuf[0] = '.';
			rtnbuf[1] = 0;
		}
		else if (rtnlen < MAX_PATH)
		{
			rtnbuf[rtnlen] = 0;
		}
		rtnbuf[MAX_PATH - 1] = 0;
#else // *nix
		char rtnbuf[4096];
		ssize_t rtnlen = readlink("/proc/self/exe", rtnbuf, 4096);
		if (rtnlen < 0)
		{
			rtnbuf[0] = '.';
			rtnbuf[1] = 0;
		}
		else if (rtnlen < 4096)
		{
			rtnbuf[rtnlen] = 0;
		}
		rtnbuf[4095] = 0;
#endif
		config.system.exepath = std::filesystem::canonical(rtnbuf).remove_filename();
	}
	{ // find user home directory
#ifdef WINDOWS
		char homedrive[MAX_PATH], homedir[MAX_PATH];
		size_t homedrivelen;
		size_t homedirlen;
		getenv_s(&homedrivelen, homedrive, MAX_PATH, "HOMEDRIVE");
		if (homedrivelen < 0)
		{
			homedrive[0] = 'C';
			homedrive[1] = ':';
			homedrive[2] = 0;
		}
		else if (homedrivelen < MAX_PATH)
		{
			homedrive[homedrivelen] = 0;
		}
		homedrive[MAX_PATH - 1] = 0;
		getenv_s(&homedirlen, homedir, MAX_PATH, "HOMEPATH");
		if (homedirlen < 0)
		{
			homedir[0] = '\\';
			homedir[1] = 0;
		}
		else if (homedirlen < MAX_PATH)
		{
			homedir[homedirlen] = 0;
		}
		homedir[MAX_PATH - 1] = 0;
		config.system.homepath = std::filesystem::canonical(
			std::filesystem::path(homedrive)
			/ std::filesystem::path(homedir));
#else // *nix
		config.system.homepath = std::filesystem::canonical(std::getenv("HOME"));
#endif
	}
	{ // possible places to find configuration file
#ifdef WINDOWS
		config.system.configloc.push_back("Windows Registry");
#else // *nix
		config.system.configloc.push_back("/etc/.retro6k-config");
#endif
		config.system.configloc.push_back(
			config.system.homepath
			/ std::filesystem::path(".retro6k-config"));
		config.system.configloc.push_back(
			config.system.exepath / std::filesystem::path(".retro6k-config"));
	}
	// fallback paths in case of failure to load config
	config.path.rompath.push_back(config.system.exepath / std::filesystem::path("../rom"));
	config.path.rompath.push_back(std::filesystem::path("./../rom"));
	config.path.rompath.push_back(config.system.exepath);
	config.path.cartpath.push_back(config.system.exepath / std::filesystem::path("../cart"));
	config.path.cartpath.push_back(std::filesystem::path("./../cart"));
	config.path.cartpath.push_back(config.system.exepath);
	config.path.savepath.push_back(config.system.homepath / std::filesystem::path("Documents"));
	config.path.savepath.push_back(config.system.homepath);
	config.path.screencappath.push_back(config.system.homepath / std::filesystem::path("Pictures"));
	config.path.screencappath.push_back(config.system.homepath);
	return 0;
}

int InitSound()
{
	SDL_AudioSpec desiredsoundspec;
	SDL_memset(&desiredsoundspec, 0, sizeof(desiredsoundspec));
	desiredsoundspec.freq = 44100;
	desiredsoundspec.format = AUDIO_U8;
	desiredsoundspec.channels = 2;
	desiredsoundspec.samples = 512;
	desiredsoundspec.callback = GenerateStereoAudio;
	sounddev = SDL_OpenAudioDevice(NULL, false, &desiredsoundspec, &soundspec, 0);
	if (!sounddev)
	{
		SDL_Log("Could not open sound device");
	}
	SDL_PauseAudioDevice(sounddev, 0);
	return 0;
}

void InstallROM()
{
	const char* infilename[4] = {
		"bankc.rom",
		"bankd.rom",
		"banke.rom",
		"bankf.rom" 
	};
	for (int i = 0; i < 4; ++i)
	{
		std::filesystem::path romfile;
		for (auto& path : config.path.rompath)
		{
			if (std::filesystem::exists(path / std::filesystem::path(infilename[i])))
			{
				romfile = path / std::filesystem::path(infilename[i]);
				break;
			}
		}
		for (int p = 0; p < 16; ++p)
			syspflags[0xc0 | (i << 4) | p] = pageflags::PF_TROM;
		std::ifstream infile(romfile, std::ios::binary);
		if (infile.is_open())
		{
			std::streamsize readsize = 0x1000;
			char* romptr = (char*)sysram + 0xC000ll + 0x1000ll * i;
			while (readsize && infile.good())
			{
				infile.read(romptr, readsize);
				readsize -= infile.gcount();
				romptr += infile.gcount();
			}
			infile.close();
		}
	}
}

template <class T> bool LoadCartridge(T infilename)
{
	std::ifstream infile(infilename, std::ios::binary);
	if (!infile.is_open())
	{
		infile.close();
		return false;
	}
	char readbuf[0x100];
	std::streamsize readsize = 0x8;
	char* readptr = readbuf;
	// read 8-byte signature
	while (readsize && infile.good())
	{
		infile.read(readptr, readsize);
		readsize -= infile.gcount();
		readptr += infile.gcount();
	}
	if (infile.eof() || !infile.good())
	{
		infile.close();
		return false;
	}
	if ( !((readbuf[0] == (char)0x47) // magic signature for R6k cartridge format 1986c
		&& (readbuf[1] == (char)0xa9)
		&& (readbuf[2] == (char)0x02)
		&& (readbuf[3] == (char)0x6a)
		&& (readbuf[4] == (char)0xbb)
		&& (readbuf[5] == (char)0x47)
		&& (readbuf[6] == (char)0xf3)
		&& (readbuf[7] == (char)0xa7)))
	{
		infile.close();
		return false;
	}
	// read offsets to (base address space) page flags start,
	// (base address space) ROM/NonVol data start (4-byte LE)
	readsize = 0x8;
	readptr = readbuf;
	while (readsize && infile.good())
	{
		infile.read(readptr, readsize);
		readsize -= infile.gcount();
		readptr += infile.gcount();
	}
	int pfstart = (unsigned char)readbuf[0] | (unsigned char)readbuf[1] << 8 | (unsigned char)readbuf[2] << 16 | (unsigned char)readbuf[3] << 24;
	int datastart = (unsigned char)readbuf[4] | (unsigned char)readbuf[5] << 8 | (unsigned char)readbuf[6] << 16 | (unsigned char)readbuf[7] << 24;
	// TODO: read extension data
	// read BAS page flags
	infile.seekg(pfstart);
	readsize = 0xa0;
	readptr = (char*)syspflags + 0x20;
	while (readsize && infile.good())
	{
		infile.read(readptr, readsize);
		readsize -= infile.gcount();
		readptr += infile.gcount();
	}
	// read BAS ROM & factory-state non-volatile memory
	infile.seekg(datastart);
	for (unsigned char p = 0x20; p < 0xc0; ++p)
	{
		if ((char)syspflags[p] & (char)pageflags::PF_TROM) // ROM or NVM
		{
			readsize = 0x100;
			readptr = (char*)sysram + ((long long)p << 8);
			while (readsize && infile.good())
			{
				infile.read(readptr, readsize);
				readsize -= infile.gcount();
				readptr += infile.gcount();
			}
		}
	}
	infile.close();
	cartridgeinserted = true;
	return true;
}

void LoadConfig() {
	for (auto& configloc : config.system.configloc)
	{
#ifdef WINDOWS
		if (cilstreq(configloc.c_str(), L"windows registry"))
		{
			// TODO: read config from Windows Registry
		}
		else
#endif
		{
			if (LoadConfigFromFile(configloc.c_str()))
				break;
		}
	}
}

template <class T> bool LoadConfigFromFile(T infilename)
{
	std::ifstream infile(infilename, std::ios::binary);
	if (!infile.is_open())
	{
		infile.close();
		return false;
	}
	std::string line;
	configfilesection section = configfilesection::CF_UNKNOWN;
	while (std::getline(infile, line))
	{
		if (!line.size() || line[0] == '#')
			continue;
		while (line.size() > 0 && 
			(line[line.size() - 1] == '\r' || line[line.size() - 1] == '\n'))
			line.resize(line.size() - 1);
		if (line[0] == '[')
		{
			if (cilstreq(line.c_str(), "[paths]"))
			{
				section = configfilesection::CF_PATHS;
				continue;
			}
			if (cilstreq(line.c_str(), "[screen]"))
			{
				section = configfilesection::CF_SCREEN;
				continue;
			}
			section = configfilesection::CF_UNKNOWN;
			continue;
		}
		int splitpos = -1;
		for (int i = 0; i < line.size(); ++i)
		{
			if (line[i] == '=')
			{
				splitpos = i;
				break;
			}
		}
		if (splitpos < 0)
			continue;
		std::string_view key(line.c_str(), splitpos);
		++splitpos;
		std::string_view value(line.c_str() + splitpos, line.size() - splitpos);
		switch (section)
		{
		case configfilesection::CF_PATHS:
			if (key == "cart")
			{
				config.sessionpath.cartpath.push_back(std::filesystem::path(value));
				break;
			}
			if (key == "rom")
			{
				config.sessionpath.rompath.push_back(std::filesystem::path(value));
				break;
			}
			if (key == "save")
			{
				config.sessionpath.savepath.push_back(std::filesystem::path(value));
				break;
			}
			if (key == "screencap")
			{
				config.sessionpath.screencappath.push_back(std::filesystem::path(value));
				break;
			}
			break;
		case configfilesection::CF_SCREEN:
			if (key == "aspectratio")
			{
				if (value == "free")
				{
					config.screen.specaspectratio = aspectratiocat::AR_FREE;
					break;
				}
				if (value == "classic")
				{
					config.screen.specaspectratio = aspectratiocat::AR_CLASSIC;
					break;
				}
				if (value == "wide")
				{
					config.screen.specaspectratio = aspectratiocat::AR_WIDE;
					break;
				}
				break;
			}
			if (key == "pixwidth")
			{
				if (value == "auto")
				{
					config.screen.specpixwidth = -1;
					break;
				}
				config.screen.specpixwidth = atoi(value.data());
				config.screen.pixwidth = config.screen.specpixwidth;
				break;
			}
			if (key == "pixheight")
			{
				if (value == "auto")
				{
					config.screen.specpixheight = -1;
					break;
				}
				config.screen.specpixheight = atoi(value.data());
				config.screen.pixheight = config.screen.specpixheight;
				break;
			}
			break;
		}
	}
	return true;
}

void LogFVMC(uint16_t dest, uint16_t src, uint8_t value)
{
	dlogentry* newentry = ExtendLog(dletype::LT_FVMC);
	newentry->fvmcentry.dest = dest;
	newentry->fvmcentry.src = src;
	newentry->fvmcentry.value = value;
	newentry->fvmcentry.memtype = syspflags[src >> 8];
}

void LogRead(uint16_t address, uint8_t value)
{
	if (suppresslogging && ((bool)(syspflags[address >> 8] & pageflags::PF_IO)
		|| ((syspflags[address >> 8] & pageflags::PF_TMASK) == pageflags::PF_TFLOATING)))
		return;
	if (!partialinst)
		goto logmementry;
	if (debuglogexpectargs == -1)
	{
		const char nargsbyopcode[256] = {
			/*      0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
			/* 0 */ 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 2, 2, 2, 2, /* 0 */
			/* 1 */ 1, 1, 0, 1, 1, 1, 1, 1, 0, 2, 0, 2, 2, 2, 2, 2, /* 1 */
			/* 2 */ 2, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 2, 2, 2, 2, /* 2 */
			/* 3 */ 1, 1, 0, 1, 1, 1, 1, 1, 0, 2, 0, 2, 2, 2, 2, 2, /* 3 */
			/* 4 */ 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 2, 2, 2, 2, /* 4 */
			/* 5 */ 1, 1, 0, 1, 1, 1, 1, 1, 0, 2, 0, 2, 2, 2, 2, 2, /* 5 */
			/* 6 */ 0, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 2, 2, 2, 2, /* 6 */
			/* 7 */ 1, 1, 0, 1, 1, 1, 1, 1, 0, 2, 0, 2, 2, 2, 2, 2, /* 7 */
			/* 8 */ 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 2, 2, 2, 2, /* 8 */
			/* 9 */ 1, 1, 0, 1, 1, 1, 1, 1, 0, 2, 0, 2, 2, 2, 2, 2, /* 9 */
			/* A */ 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 2, 2, 2, 2, /* A */
			/* B */ 1, 1, 0, 1, 1, 1, 1, 1, 0, 2, 0, 2, 2, 2, 2, 2, /* B */
			/* C */ 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 2, 2, 2, 2, /* C */
			/* D */ 1, 1, 0, 1, 1, 1, 1, 1, 0, 2, 0, 2, 2, 2, 2, 2, /* D */
			/* E */ 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 2, 2, 2, 2, /* E */
			/* F */ 1, 1, 0, 1, 1, 1, 1, 1, 0, 2, 0, 2, 2, 2, 2, 2  /* F */
		};
		if (partialinst->entrytype != dletype::LT_PARTIALINST)
			goto logmementry;
		partialinst->instentry.opc = value;
		debuglogexpectargs = nargsbyopcode[value];
		partialinst->instentry.nargs = 0;
	}
	else if (debuglogexpectargs)
	{
		if (partialinst->entrytype != dletype::LT_PARTIALINST)
			goto logmementry;
		partialinst->instentry.arg[partialinst->instentry.nargs] = value;
		++(partialinst->instentry.nargs);
		--debuglogexpectargs;
		if (!debuglogexpectargs)
		{
			partialinst->entrytype = dletype::LT_INST;
			partialinst = nullptr;
		}
	}
	else
	{
	logmementry:
		if (suppresslogging && (bool)(syspflags[address >> 8] & pageflags::PF_DONTLOG))
			return;
		dlogentry* newentry = ExtendLog(dletype::LT_READ);
		newentry->mementry.address = address;
		newentry->mementry.value = value;
		newentry->mementry.memtype = syspflags[address >> 8];
	}
}

void LogReset()
{
	ExtendLog(dletype::LT_RESET);
	suppresslogging = false;
}

void LogScanline(int scanline)
{
	if (debuglog[(debuglogend - 1) & dlogidxmask].entrytype == dletype::LT_SCANLINE)
	{
		debuglog[(debuglogend - 1) & dlogidxmask].scanline = scanline;
		return;
	}
	ExtendLog(dletype::LT_SCANLINE)->scanline = scanline;
}

void LogStack(unsigned char sp)
{
	dlogentry* newentry = ExtendLog(dletype::LT_STACK);
	if (suppresslogging)
		return;
	for (unsigned i = 0, j = sp + 1; i < 7; ++i, ++j, j &= 0xff)
	{
		newentry->stackentry.stack[i] = sysram[j | 0x100];
	}
	newentry->stackentry.nstack = stackbase - sp;
	//DoDebugger();
}

extern "C" void LogStep()
{
	extern uint32_t clockticks6502;
	extern uint16_t pc;
	extern uint8_t sp, a, x, y, status;
	static dlogentry* laststateentry = nullptr, * lastinstentry = nullptr, * lastboringentry = nullptr;
	bool newsuppresslogging = (bool)(syspflags[pc >> 8] & pageflags::PF_DONTLOG);
	if (newsuppresslogging && !suppresslogging)
	{
		if (laststateentry && lastinstentry)
			lastinstentry->instentry.cycles = clockticks6502 - laststateentry->stateentry.ticks;
		lastboringentry = ExtendLog(dletype::LT_BORING);
		lastboringentry->boringentry.entryclock = clockticks6502;
		lastboringentry->boringentry.complete = false;
		lastinstentry = nullptr;
	}
	else if (!newsuppresslogging && suppresslogging)
	{
		lastboringentry->boringentry.exitclock = clockticks6502;
		lastboringentry->boringentry.complete = true;
	}
	suppresslogging = newsuppresslogging;
	if (suppresslogging)
		return;
	if (partialinst && partialinst->entrytype == dletype::LT_PARTIALINST)
		partialinst->entrytype = dletype::LT_INST;
	dlogentry* newentry = ExtendLog(dletype::LT_STATE);
	newentry->stateentry.ticks = clockticks6502;
	newentry->stateentry.pc = pc;
	newentry->stateentry.sp = sp;
	newentry->stateentry.a = a;
	newentry->stateentry.x = x;
	newentry->stateentry.y = y;
	newentry->stateentry.status = status;
	if (laststateentry && lastinstentry)
		lastinstentry->instentry.cycles = newentry->stateentry.ticks - laststateentry->stateentry.ticks;
	if (laststateentry && laststateentry->stateentry.sp != newentry->stateentry.sp)
		LogStack(sp);
	laststateentry = newentry;
	lastinstentry = partialinst = ExtendLog(dletype::LT_PARTIALINST);
	debuglogexpectargs = -1;
}

void LogWrite(uint16_t address, uint8_t value)
{
	dlogentry* last = debuglog + ((debuglogend - 1) & dlogidxmask);
	if (suppresslogging && ((bool)(syspflags[address >> 8] & pageflags::PF_IO)
		|| ((syspflags[address >> 8] & pageflags::PF_TMASK) == pageflags::PF_TFLOATING)))
		return;
	if (last->entrytype == dletype::LT_PARTIALINST)
		last->entrytype = dletype::LT_INST;
	dlogentry* newentry = ExtendLog(dletype::LT_WRITE);
	newentry->mementry.address = address;
	newentry->mementry.value = value;
	newentry->mementry.memtype = syspflags[address >> 8];
}

void PaintCell(unsigned char col, unsigned char row, unsigned char glyph, unsigned char* att)
{
	unsigned char* faddr = sysram;
	faddr += 0x1000 | (glyph << 4);
	unsigned char* paddr = sysram;
	paddr += 0x0FE0;
	for (int gy = 0, cy = 0, sy = row << 5; gy < 8; ++gy)
	{
		unsigned char pix[8];
		pix[0] = paddr[att[(*faddr & 0x3)]];
		pix[1] = paddr[att[(*faddr & 0xC) >> 2]];
		pix[2] = paddr[att[(*faddr & 0x30) >> 4]];
		pix[3] = paddr[att[(*faddr & 0xC0) >> 6]];
		faddr++;
		pix[4] = paddr[att[(*faddr & 0x3)]];
		pix[5] = paddr[att[(*faddr & 0xC) >> 2]];
		pix[6] = paddr[att[(*faddr & 0x30) >> 4]];
		pix[7] = paddr[att[(*faddr & 0xC0) >> 6]];
		faddr++;
		for (int ry = 0; ry < config.screen.pixheight; ++ry, ++cy)
		{
			unsigned char* dst = (unsigned char*)cellcanvas->pixels + (long long)cellcanvas->pitch * cy;
			for (int gx = 0, cx = 0; gx < 8; ++gx)
			{
				for (int rx = 0; rx < config.screen.pixwidth; ++rx, ++dst)
				{
					*dst = pix[gx];
				}
			}
		}
		scanlinedirty[sy++] = true;
		scanlinedirty[sy++] = true;
		scanlinedirty[sy++] = true;
		scanlinedirty[sy++] = true;
	}
	SDL_Rect dstrect;
	dstrect.x = col * config.screen.pixwidth * 8;
	dstrect.y = row * config.screen.pixheight * 8;
	SDL_BlitSurface(cellcanvas, nullptr, framebuffer, &dstrect);
}

inline void PaintCell(unsigned char col, unsigned char row)
{
	unsigned char glyph = sysram[0x0800 | row << 5 | col];
	unsigned char att[4];
	unsigned char tatt[5];
	tatt[0] = sysram[0x0A40 + ((row & 0x1e) << 4) + col] >> (4 * (row & 0x1));
	tatt[1] = sysram[0x0B60 + ((row & 0x1e) << 4) + col] >> (4 * (row & 0x1));
	tatt[2] = sysram[0x0C80 + ((row & 0x1e) << 4) + col] >> (4 * (row & 0x1));
	tatt[3] = sysram[0x0DA0 + ((row & 0x1e) << 4) + col] >> (4 * (row & 0x1));
	tatt[4] = sysram[0x0EC0 + ((row & 0x1e) << 4) + col] >> (4 * (row & 0x1));
	att[0] = (tatt[0] & 0x1) | (tatt[1] & 0x1) << 1 | (tatt[2] & 0x1) << 2 | (tatt[3] & 0x1) << 3 | (tatt[4] & 0x1) << 4;
	att[1] = (tatt[0] & 0x2) >> 1 | (tatt[1] & 0x2) | (tatt[2] & 0x2) << 1 | (tatt[3] & 0x2) << 2 | (tatt[4] & 0x2) << 3;
	att[2] = (tatt[0] & 0x4) >> 2 | (tatt[1] & 0x4) >> 1 | (tatt[2] & 0x4) | (tatt[3] & 0x4) << 1 | (tatt[4] & 0x4) << 2;
	att[3] = (tatt[0] & 0x8) >> 3 | (tatt[1] & 0x8) >> 2 | (tatt[2] & 0x8) >> 1 | (tatt[3] & 0x8) | (tatt[4] & 0x8) << 1;
	PaintCell(col, row, glyph, att);
}

void PaintWindow(SDL_Surface* win, unsigned int bg, unsigned int acc, const char* title, unsigned int tc)
{
	SDL_Rect boxrect;
	boxrect = { 0, 0, win->w, 1 };
	SDL_FillRect(win, &boxrect, bg);
	boxrect.y = win->h - 1;
	SDL_FillRect(win, &boxrect, bg);
	boxrect = { 0, 0, 1, win->h };
	SDL_FillRect(win, &boxrect, bg);
	boxrect.x = win->w - 1;
	SDL_FillRect(win, &boxrect, bg);
	boxrect = { 2, 19, win->w - 4, win->h - 21 };
	SDL_FillRect(win, &boxrect, bg);
	boxrect = { 1, 19, 1, win->h - 20 };
	SDL_FillRect(win, &boxrect, acc);
	boxrect.x = win->w - 2;
	SDL_FillRect(win, &boxrect, acc);
	boxrect = { 1, win->h - 2, win->w - 2, 1 };
	SDL_FillRect(win, &boxrect, acc);
	int tx0 = (win->w - TextWidth(title)) >> 1;
	boxrect = { 1, 1, win->w - 2, 18 };
	SDL_FillRect(win, &boxrect, acc);
	boxrect.h = 1;
	boxrect.y = 5;
	SDL_FillRect(win, &boxrect, darkercolor[acc]);
	boxrect.y = 6;
	SDL_FillRect(win, &boxrect, lightercolor[acc]);
	boxrect.y = 12;
	SDL_FillRect(win, &boxrect, darkercolor[acc]);
	boxrect.y = 13;
	SDL_FillRect(win, &boxrect, lightercolor[acc]);
	DrawText(title, tx0, 2, tc, acc, win);
	uint32_t k;
	SDL_GetColorKey(win, &k);
	if (k != 0xffffffff)
		RoundWinCorners(win, k, bg, acc);
}

void PickAndInsertCartridge()
{
	std::string cart;
	DoPickFile(filetype::FT_CART, cart);
	if (cart.empty())
		return;
	LoadCartridge(cart.c_str());
}

void RandomBitFlip()
{
	static std::mt19937 gen(0);
	static std::uniform_int_distribution<> dis(0x0800, 0x1FFF);
	if (dis(gen) & 0xf)
		return;
	int addr = dis(gen);
	int bit = dis(gen) & 7;
	write6502(addr, sysram[addr] ^ (1 << bit));
}

int ReadArgs(int argc, char** argv)
{
	// TODO: read command-line arguments.
	char* cartspec = nullptr;
	char* savespec = nullptr;
	for (char** thisargptr = argv + 1; thisargptr - argv < argc; ++thisargptr)
	{
		if ((*thisargptr)[0] == '-')
		{
			if ((*thisargptr)[1] == '-')
			{
				// long option
#define WARN_NOMATCH default: \
                       SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION, \
                         "Command line option %s " \
				         "ambiguous or not understood.", \
                         *thisargptr);
				switch ((*thisargptr)[2])
				{
				case 'c':
					switch ((*thisargptr)[3])
					{
					case 'a': //--ca(rtpath): specify cart path
						goto readcartpathparameter;
					case 'o': //--co(nfigfile): specify config file location
						goto readconfigfileparameter;
					WARN_NOMATCH
					}
					break;
				case 'r':
					switch ((*thisargptr)[3])
					{
					case 'o': //--ro(mpath): specify rom path
						goto readrompathparameter;
						WARN_NOMATCH
					}
					break;
				case 's':
					switch ((*thisargptr)[3])
					{
					case 'a': //--sa(vepath): specify save path
						goto readsavepathparameter;
					case 'c': //--sc(reencappath): specify save path
						goto readscreencappathparameter;
						WARN_NOMATCH
					}
					break;
					WARN_NOMATCH
				}
#undef WARN_NOMATCH
			}
			else
			{
				// short option(s) / switch(es)
				char lastshort = 0;
				for (char* thisshortptr = (*thisargptr) + 1; 
					*thisshortptr; ++thisshortptr)
				{
					lastshort = *thisshortptr;
					switch (lastshort)
					{
						// handlers for each short option / switch
					case 'c': // specify config file location
					case 'p': // specify cart path
					case 'r': // specify rom path
					case 's': // specify save path
						break;
					default:
						SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
							"Command line switch -%c not understood.",
							lastshort);
					}
				}
				switch (lastshort)
				{
				case 'c':
					goto readconfigfileparameter;
				case 'p':
					goto readcartpathparameter;
				case 'r':
					goto readrompathparameter;
				case 's':
					goto readsavepathparameter;
					// jumps for next-argument reading
				default:
					;
				}
			}
			continue;
			// labels for reading next argument as parameter to an option
		readconfigfileparameter:
			// config file spec, if given, should become
			//only item of config.system.configloc
			++thisargptr;
			config.system.configloc.clear();
			config.system.configloc.push_back(
				std::filesystem::path(*thisargptr));
			continue;
		readcartpathparameter:
			++thisargptr;
			config.sessionpath.cartpath.push_back(
				std::filesystem::path(*thisargptr));
			continue;
		readrompathparameter:
			++thisargptr;
			config.sessionpath.rompath.push_back(
				std::filesystem::path(*thisargptr));
			continue;
		readsavepathparameter:
			++thisargptr;
			config.sessionpath.savepath.push_back(
				std::filesystem::path(*thisargptr));
			continue;
		readscreencappathparameter:
			++thisargptr;
			config.sessionpath.screencappath.push_back(
				std::filesystem::path(*thisargptr));
			continue;
		}
		else
		{
			// positional argument
			if (cartspec == nullptr)
			{
				cartspec = *thisargptr;
				continue;
			}
			if (savespec == nullptr)
			{
				savespec = *thisargptr;
				continue;
			}
			SDL_LogWarn(SDL_LOG_CATEGORY_APPLICATION,
				"Command line argument %s not understood.",
				*thisargptr);
		}
	}
    // TODO: actually do something with cartspec and savespec if they are non-null
	return 0;
}

void RenderScanline(int scanline, SDL_Surface* framebuffer, SDL_Surface* winsurface)
{
	if (!scanlinedirty[scanline])
		return;
	int y0 = scanline * winsurface->h / 576;
	int y1 = (scanline + 1) * winsurface->h / 576;
	unsigned char* src = (unsigned char*)(framebuffer->pixels) + (long long)framebuffer->pitch * y0;
	unsigned char* dst = (unsigned char*)(winsurface->pixels) + (long long)winsurface->pitch * y0;
	//pitch is assumed same for both surfaces
	unsigned char* stop = (unsigned char*)(framebuffer->pixels) + (long long)framebuffer->pitch * y1;
	while (src < stop)
	{
		*dst = *src;
		++src;
		++dst;
	}
	scanlinedirty[scanline] = false;
}

void RoundWinCorners(SDL_Surface* destsurf, unsigned int key, unsigned int bg, unsigned int fg)
{
#define r0 wincornercliprect[i]
	for (int i = 0; i < NWINCORNERCLIPRECTS; ++i)
	{
		SDL_Rect r1 = wincornercliprect[i];
		SDL_Rect r2 = wincornercliprect[i];
		SDL_Rect r3 = wincornercliprect[i];
		r1.x = r3.x = destsurf->w - r0.x - r0.w;
		r2.y = r3.y = destsurf->h - r0.y - r0.h;
		SDL_FillRect(destsurf, &r0, key);
		SDL_FillRect(destsurf, &r1, key);
		SDL_FillRect(destsurf, &r2, key);
		SDL_FillRect(destsurf, &r3, key);
	}
#undef r0
#define r0 wincornerborder0rect[i]
	for (int i = 0; i < NWINCORNERBORDER0RECTS; ++i)
	{
		SDL_Rect r1 = wincornerborder0rect[i];
		SDL_Rect r2 = wincornerborder0rect[i];
		SDL_Rect r3 = wincornerborder0rect[i];
		r1.x = r3.x = destsurf->w - r0.x - r0.w;
		r2.y = r3.y = destsurf->h - r0.y - r0.h;
		SDL_FillRect(destsurf, &r0, bg);
		SDL_FillRect(destsurf, &r1, bg);
		SDL_FillRect(destsurf, &r2, bg);
		SDL_FillRect(destsurf, &r3, bg);
	}
#undef r0
#define r0 wincornerborder1rect[i]
	for (int i = 0; i < NWINCORNERBORDER1RECTS; ++i)
	{
		SDL_Rect r2 = wincornerborder1rect[i];
		SDL_Rect r3 = wincornerborder1rect[i];
		r3.x        = destsurf->w - r0.x - r0.w;
		r2.y = r3.y = destsurf->h - r0.y - r0.h;
		SDL_FillRect(destsurf, &r2, fg);
		SDL_FillRect(destsurf, &r3, fg);
	}
#undef r0
}

int SetHWPalette(SDL_Palette* dstpal)
{
	SDL_Color d[256];
	for (int j = 0; j < 256; ++j)
	{
		d[j].a = 255;
		int i = ((j & 0x8) >> 2 | (j & 0x80) >> 7) * 0x11;
		d[j].r = ((j & 0x4) >> 1 | (j & 0x40) >> 6) * 0x44 | i;
		d[j].g = ((j & 0x2) | (j & 0x20) >> 5) * 0x44 | i;
		d[j].b = ((j & 0x1) << 1 | (j & 0x10) >> 4) * 0x44 | i;
	}
	return SDL_SetPaletteColors(dstpal, d, 0, 256);
}

void StepSoundSchedules()
{
	bool shiftregister[4]{ false, false, false, false };
	bool scheduleisempty[2]{ false, false };
	for (int s = 0; s < 2; ++s)
	{
		if (!soundcndcounter[s])
		{
			for (int v = 0; v < 4; ++v)
			{
				if (soundscvmapregister & (1 << (4 * s + v)))
				{
					soundvolregister[v] = soundqueue[v][0].vol;
					soundfreqregister[v] = soundqueue[v][0].freq;
					soundcndcounter[s] = soundqueuedur[s][0];
					shiftregister[v] = true;
				}
			}
			for (int i = 0; i < 15; ++i)
				soundqueuedur[s][i] = soundqueuedur[s][i + 1];
			soundqueuedur[s][15] = 0xffff;
			++soundncounter[s];
			if (!soundncounter[s])
				scheduleisempty[s] = true;
		}
		--soundcndcounter[s];
	}
	for (int v = 0; v < 4; ++v)
	{
		for (int i = 0; i < 15; ++i)
			soundqueue[v][i] = soundqueue[v][i + 1];
		soundqueue[v][15] = { 0, 0 };
	}
	if (scheduleisempty[0] || scheduleisempty[1])
		; // TODO: trigger interrupt if appropriate flag (?) is set
}

int TextWidth(const char* text)
{
	int w = 0;
	while (*text)
	{
		w += hifont[(unsigned char)*text].w;
		++text;
	}
	return w;
}

inline void UIBeepMoveSel()
{
	menubeepdur = 2205;
	menubeepfreq = 440;
}

inline void UIBeepTakeAction()
{
	menubeepdur = 2205;
	menubeepfreq = 880;
}

inline void UIBeepUnavailable()
{
	menubeepdur = 8820;
	menubeepfreq = 55;
}

Uint32 SetBool(Uint32 interval, void* boolvar)
{
	*(bool*)boolvar = true;
	return interval;
}

inline uint8_t bare_read6502(uint16_t address)
{
	static std::uniform_int_distribution<> dis(0x00, 0xFF);
	if ((address & 0xff00) == 0x0200)
	{
		switch (address & 0xff)
		{
			unsigned char t;
		case 0x44: // keyboard
			t = keypressregister;
			keypressregister = 0;
			return t;
		case 0xfc: // video flags
			return videostate;
		default:
			return dis(floatgen);
		}
	}
	if ((syspflags[address >> 8] & pageflags::PF_TMASK) == pageflags::PF_TFLOATING
		|| (syspflags[address >> 8] & (pageflags::PF_IO | pageflags::PF_TMASK)) == pageflags::PF_OUTONLY)
	{
		return dis(floatgen); // TODO: more realistic open-bus behavior?
	}
	return sysram[address];
}

template <class T> bool cilstreq(T a, T b)
// case-insensitive c-string equality comparison,
// optimized by assuming b is already lowercase
{
	if (a == b)
		return true;
	while (true)
	{
		if (*a == 0 && *b == 0)
			return true;
		if (tolower(*a) != *b)
			return false;
		++a;
		++b;
	}
}

template <class T> bool cistreq(T a, T b)
// case-insensitive c-string equality comparison
{
	if (a == b)
		return true;
	while (true)
	{
		if (*a == 0 && *b == 0)
			return true;
		if (tolower(*a) != tolower(*b))
			return false;
		++a;
		++b;
	}
}

extern "C" uint8_t read6502(uint16_t address)
{
	uint8_t result = bare_read6502(address);
	LogRead(address, result);
	return result;
}

extern "C" void write6502(uint16_t address, uint8_t value)
{
	LogWrite(address, value);
	static unsigned short fvmcdest, fvmcsrc; // fast video memory copy address page registers (<<8 here)
	if ((address & 0xff00) == 0x0000 && fvmcdest && !videobusy)
	{
		// do fast video memory copy instead of writing value to literal address
		// note, hardware implementation does not guarantee memory at zp address won't be changed
		value = sysram[address & 0xff | fvmcsrc];
		address = address & 0xff | fvmcdest;
		LogFVMC(address, address & 0xff | fvmcsrc, value);
	}
	if (videobusy && address >= 0x0800 && address < 0x2000)
	{
		return; // writing to video ram blocked
	}
	if (!((char)syspflags[address >> 8] & (char)pageflags::PF_TRAM)) //neither RAM nor NVM
	{
		return; // this page is not writeable
	}
	sysram[address] = value;
	if (address >= 0x0800 && address < 0x2000)
	{
		if (address >= 0x1000) // change to font memory, repaint affected cells
		{
			unsigned char c = (address & 0xff0) >> 4;
			unsigned char* checkptr = sysram + 0x0800;
			for (int y = 0; y < 18; ++y)
				for (int x = 0; x < 32; ++x)
					if (*checkptr++ == c)
						PaintCell(x, y);
		}
		else if (address < 0x0A40) // change a character on screen, repaint affected cell
		{
			int x = address & 0x1f;
			int y = (address & 0x3e0) >> 5;
			PaintCell(x, y);
		}
		else if (address >= 0x0FE0) // change a palette entry, repaint affected cells
		{
			unsigned char a = address - 0x0FE0;
			unsigned char* checkptr[5] = {
				sysram + 0xA40,
				sysram + 0xB60,
				sysram + 0xC80,
				sysram + 0xDA0,
				sysram + 0xEC0 };
			for (int i = 0; i < 0x120; ++i)
			{
				int x = i & 0x1f;
				int y = (i & 0x1e0) >> 4;
				unsigned char att[4];
				att[0] = (*checkptr[0] & 0x1) | (*checkptr[1] & 0x1) << 1 | (*checkptr[2] & 0x1) << 2 | (*checkptr[3] & 0x1) << 3 | (*checkptr[4] & 0x1) << 4;
				att[1] = (*checkptr[0] & 0x2) >> 1 | (*checkptr[1] & 0x2) | (*checkptr[2] & 0x2) << 1 | (*checkptr[3] & 0x2) << 2 | (*checkptr[4] & 0x2) << 3;
				att[2] = (*checkptr[0] & 0x4) >> 2 | (*checkptr[1] & 0x4) >> 1 | (*checkptr[2] & 0x4) | (*checkptr[3] & 0x4) << 1 | (*checkptr[4] & 0x4) << 2;
				att[3] = (*checkptr[0] & 0x8) >> 3 | (*checkptr[1] & 0x8) >> 2 | (*checkptr[2] & 0x8) >> 1 | (*checkptr[3] & 0x8) | (*checkptr[4] & 0x8) << 1;
				if (att[0] == a || att[1] == a || att[2] == a || att[3] == a)
					PaintCell(x, y);
				att[0] = (*checkptr[0] & 0x10) >> 4 | (*checkptr[1] & 0x10) >> 3 | (*checkptr[2] & 0x10) >> 2 | (*checkptr[3] & 0x10) >> 1 | (*checkptr[4] & 0x10);
				att[1] = (*checkptr[0] & 0x20) >> 5 | (*checkptr[1] & 0x20) >> 4 | (*checkptr[2] & 0x20) >> 3 | (*checkptr[3] & 0x20) >> 2 | (*checkptr[4] & 0x20) >> 1;
				att[2] = (*checkptr[0] & 0x40) >> 6 | (*checkptr[1] & 0x40) >> 5 | (*checkptr[2] & 0x40) >> 4 | (*checkptr[3] & 0x40) >> 3 | (*checkptr[4] & 0x40) >> 2;
				att[3] = (*checkptr[0] & 0x80) >> 7 | (*checkptr[1] & 0x80) >> 6 | (*checkptr[2] & 0x80) >> 5 | (*checkptr[3] & 0x80) >> 4 | (*checkptr[4] & 0x80) >> 3;
				if (att[0] == a || att[1] == a || att[2] == a || att[3] == a)
					PaintCell(x, y | 0x1);
				++checkptr[0];
				++checkptr[1];
				++checkptr[2];
				++checkptr[3];
				++checkptr[4];
			}
		}
		else { // change a pair of cells' attributes, repaint affected cells
			int x = address & 0x1f;
			int y = ((address - 0x0A40 - ((address - 0x0A40) / 0x120) * 0x120) & 0x3e0) >> 4;
			PaintCell(x, y);
			PaintCell(x, y | 0x1);
		}
	}
	if ((address & 0xff00) == 0x0300)
	{
		switch (address & 0xff)
		{
		case 0x80:
			soundfreqregister[0] = soundfreqregister[0] & 0xff00 | value;
			break;
		case 0x81:
			soundfreqregister[0] = soundfreqregister[0] & 0xff | (value << 8);
			break;
		case 0x82:
			soundfreqregister[1] = soundfreqregister[1] & 0xff00 | value;
			break;
		case 0x83:
			soundfreqregister[1] = soundfreqregister[1] & 0xff | (value << 8);
			break;
		case 0x84:
			soundfreqregister[2] = soundfreqregister[2] & 0xff00 | value;
			break;
		case 0x85:
			soundfreqregister[2] = soundfreqregister[2] & 0xff | (value << 8);
			break;
		case 0x86:
			soundfreqregister[3] = soundfreqregister[3] & 0xff00 | value;
			break;
		case 0x87:
			soundfreqregister[3] = soundfreqregister[3] & 0xff | (value << 8);
			break;
		case 0x88:
			soundvolregister[0] = value;
			break;
		case 0x89:
			soundvolregister[1] = value;
			break;
		case 0x8a:
			soundvolregister[2] = value;
			break;
		case 0x8b:
			soundvolregister[3] = value;
			break;
		case 0x8c:
			soundvoicetyperegister = value;
			break;
		case 0x8d:
			soundscvmapregister = value;
			break;
		case 0x8e:
			if (value & 0x80)
				for (int i = 0; i < 16; ++i)
					soundqueue[3][i] = { 0, 0 };
			if (value & 0x40)
				for (int i = 0; i < 16; ++i)
					soundqueue[2][i] = { 0, 0 };
			if (value & 0x20)
				for (int i = 0; i < 16; ++i)
					soundqueue[1][i] = { 0, 0 };
			if (value & 0x10)
				for (int i = 0; i < 16; ++i)
					soundqueue[0][i] = { 0, 0 };
			if (value & 0x08)
				for (int i = 0; i < 16; ++i)
				{
					soundqueuedur[1][i] = 0xffff;
					soundcndcounter[1] = 0xffff;
				}
			if (value & 0x04)
				for (int i = 0; i < 16; ++i)
				{
					soundqueuedur[0][i] = 0xffff;
					soundcndcounter[0] = 0xffff;
				}
			if (value & 0x02)
				soundcndcounter[1] = 0;
			if (value & 0x01)
				soundcndcounter[0] = 0;
			break;
		case 0x8f:
			soundqueueregisteroffset = (value & 0x3) << 2;
			break;
		case 0x90:
			soundqueuedur[0][soundqueueregisteroffset] = soundqueuedur[0][soundqueueregisteroffset] & 0xff00 | value;
			soundncounter[0] = ~soundqueueregisteroffset;
			break;
		case 0x91:
			soundqueuedur[0][soundqueueregisteroffset] = soundqueuedur[0][soundqueueregisteroffset] & 0xff | (value << 8);
			soundncounter[0] = ~soundqueueregisteroffset;
			break;
		case 0x92:
			soundqueuedur[0][soundqueueregisteroffset | 1] = soundqueuedur[0][soundqueueregisteroffset | 1] & 0xff00 | value;
			soundncounter[0] = ~(soundqueueregisteroffset | 1);
			break;
		case 0x93:
			soundqueuedur[0][soundqueueregisteroffset | 1] = soundqueuedur[0][soundqueueregisteroffset | 1] & 0xff | (value << 8);
			soundncounter[0] = ~(soundqueueregisteroffset | 1);
			break;
		case 0x94:
			soundqueuedur[0][soundqueueregisteroffset | 2] = soundqueuedur[0][soundqueueregisteroffset | 2] & 0xff00 | value;
			soundncounter[0] = ~(soundqueueregisteroffset | 2);
			break;
		case 0x95:
			soundqueuedur[0][soundqueueregisteroffset | 2] = soundqueuedur[0][soundqueueregisteroffset | 2] & 0xff | (value << 8);
			soundncounter[0] = ~(soundqueueregisteroffset | 2);
			break;
		case 0x96:
			soundqueuedur[0][soundqueueregisteroffset | 3] = soundqueuedur[0][soundqueueregisteroffset | 3] & 0xff00 | value;
			soundncounter[0] = ~(soundqueueregisteroffset | 3);
			break;
		case 0x97:
			soundqueuedur[0][soundqueueregisteroffset | 3] = soundqueuedur[0][soundqueueregisteroffset | 3] & 0xff | (value << 8);
			soundncounter[0] = ~(soundqueueregisteroffset | 3);
			break;
		case 0x98:
			soundqueuedur[1][soundqueueregisteroffset] = soundqueuedur[1][soundqueueregisteroffset] & 0xff00 | value;
			soundncounter[1] = ~soundqueueregisteroffset;
			break;
		case 0x99:
			soundqueuedur[1][soundqueueregisteroffset] = soundqueuedur[1][soundqueueregisteroffset] & 0xff | (value << 8);
			soundncounter[1] = ~soundqueueregisteroffset;
			break;
		case 0x9a:
			soundqueuedur[1][soundqueueregisteroffset | 1] = soundqueuedur[1][soundqueueregisteroffset | 1] & 0xff00 | value;
			soundncounter[1] = ~(soundqueueregisteroffset | 1);
			break;
		case 0x9b:
			soundqueuedur[1][soundqueueregisteroffset | 1] = soundqueuedur[1][soundqueueregisteroffset | 1] & 0xff | (value << 8);
			soundncounter[1] = ~(soundqueueregisteroffset | 1);
			break;
		case 0x9c:
			soundqueuedur[1][soundqueueregisteroffset | 2] = soundqueuedur[1][soundqueueregisteroffset | 2] & 0xff00 | value;
			soundncounter[1] = ~(soundqueueregisteroffset | 2);
			break;
		case 0x9d:
			soundqueuedur[1][soundqueueregisteroffset | 2] = soundqueuedur[1][soundqueueregisteroffset | 2] & 0xff | (value << 8);
			soundncounter[1] = ~(soundqueueregisteroffset | 2);
			break;
		case 0x9e:
			soundqueuedur[1][soundqueueregisteroffset | 3] = soundqueuedur[1][soundqueueregisteroffset | 3] & 0xff00 | value;
			soundncounter[1] = ~(soundqueueregisteroffset | 3);
			break;
		case 0x9f:
			soundqueuedur[1][soundqueueregisteroffset | 3] = soundqueuedur[1][soundqueueregisteroffset | 3] & 0xff | (value << 8);
			soundncounter[1] = ~(soundqueueregisteroffset | 3);
			break;
		case 0xa0:
			soundqueue[0][soundqueueregisteroffset].freq = soundqueue[0][soundqueueregisteroffset].freq & 0xff00 | value;
			break;
		case 0xa1:
			soundqueue[0][soundqueueregisteroffset].freq = soundqueue[0][soundqueueregisteroffset].freq & 0xff | (value << 8);
			break;
		case 0xa2:
			soundqueue[0][soundqueueregisteroffset | 1].freq = soundqueue[0][soundqueueregisteroffset | 1].freq & 0xff00 | value;
			break;
		case 0xa3:
			soundqueue[0][soundqueueregisteroffset | 1].freq = soundqueue[0][soundqueueregisteroffset | 1].freq & 0xff | (value << 8);
			break;
		case 0xa4:
			soundqueue[0][soundqueueregisteroffset | 2].freq = soundqueue[0][soundqueueregisteroffset | 2].freq & 0xff00 | value;
			break;
		case 0xa5:
			soundqueue[0][soundqueueregisteroffset | 2].freq = soundqueue[0][soundqueueregisteroffset | 2].freq & 0xff | (value << 8);
			break;
		case 0xa6:
			soundqueue[0][soundqueueregisteroffset | 3].freq = soundqueue[0][soundqueueregisteroffset | 3].freq & 0xff00 | value;
			break;
		case 0xa7:
			soundqueue[0][soundqueueregisteroffset | 3].freq = soundqueue[0][soundqueueregisteroffset | 3].freq & 0xff | (value << 8);
			break;
		case 0xa8:
			soundqueue[1][soundqueueregisteroffset].freq = soundqueue[1][soundqueueregisteroffset].freq & 0xff00 | value;
			break;
		case 0xa9:
			soundqueue[1][soundqueueregisteroffset].freq = soundqueue[1][soundqueueregisteroffset].freq & 0xff | (value << 8);
			break;
		case 0xaa:
			soundqueue[1][soundqueueregisteroffset | 1].freq = soundqueue[1][soundqueueregisteroffset | 1].freq & 0xff00 | value;
			break;
		case 0xab:
			soundqueue[1][soundqueueregisteroffset | 1].freq = soundqueue[1][soundqueueregisteroffset | 1].freq & 0xff | (value << 8);
			break;
		case 0xac:
			soundqueue[1][soundqueueregisteroffset | 2].freq = soundqueue[1][soundqueueregisteroffset | 2].freq & 0xff00 | value;
			break;
		case 0xad:
			soundqueue[1][soundqueueregisteroffset | 2].freq = soundqueue[1][soundqueueregisteroffset | 2].freq & 0xff | (value << 8);
			break;
		case 0xae:
			soundqueue[1][soundqueueregisteroffset | 3].freq = soundqueue[1][soundqueueregisteroffset | 3].freq & 0xff00 | value;
			break;
		case 0xaf:
			soundqueue[1][soundqueueregisteroffset | 3].freq = soundqueue[1][soundqueueregisteroffset | 3].freq & 0xff | (value << 8);
			break;
		case 0xb0:
			soundqueue[2][soundqueueregisteroffset].freq = soundqueue[2][soundqueueregisteroffset].freq & 0xff00 | value;
			break;
		case 0xb1:
			soundqueue[2][soundqueueregisteroffset].freq = soundqueue[2][soundqueueregisteroffset].freq & 0xff | (value << 8);
			break;
		case 0xb2:
			soundqueue[2][soundqueueregisteroffset | 1].freq = soundqueue[2][soundqueueregisteroffset | 1].freq & 0xff00 | value;
			break;
		case 0xb3:
			soundqueue[2][soundqueueregisteroffset | 1].freq = soundqueue[2][soundqueueregisteroffset | 1].freq & 0xff | (value << 8);
			break;
		case 0xb4:
			soundqueue[2][soundqueueregisteroffset | 2].freq = soundqueue[2][soundqueueregisteroffset | 2].freq & 0xff00 | value;
			break;
		case 0xb5:
			soundqueue[2][soundqueueregisteroffset | 2].freq = soundqueue[2][soundqueueregisteroffset | 2].freq & 0xff | (value << 8);
			break;
		case 0xb6:
			soundqueue[2][soundqueueregisteroffset | 3].freq = soundqueue[2][soundqueueregisteroffset | 3].freq & 0xff00 | value;
			break;
		case 0xb7:
			soundqueue[2][soundqueueregisteroffset | 3].freq = soundqueue[2][soundqueueregisteroffset | 3].freq & 0xff | (value << 8);
			break;
		case 0xb8:
			soundqueue[3][soundqueueregisteroffset].freq = soundqueue[3][soundqueueregisteroffset].freq & 0xff00 | value;
			break;
		case 0xb9:
			soundqueue[3][soundqueueregisteroffset].freq = soundqueue[3][soundqueueregisteroffset].freq & 0xff | (value << 8);
			break;
		case 0xba:
			soundqueue[3][soundqueueregisteroffset | 1].freq = soundqueue[3][soundqueueregisteroffset | 1].freq & 0xff00 | value;
			break;
		case 0xbb:
			soundqueue[3][soundqueueregisteroffset | 1].freq = soundqueue[3][soundqueueregisteroffset | 1].freq & 0xff | (value << 8);
			break;
		case 0xbc:
			soundqueue[3][soundqueueregisteroffset | 2].freq = soundqueue[3][soundqueueregisteroffset | 2].freq & 0xff00 | value;
			break;
		case 0xbd:
			soundqueue[3][soundqueueregisteroffset | 2].freq = soundqueue[3][soundqueueregisteroffset | 2].freq & 0xff | (value << 8);
			break;
		case 0xbe:
			soundqueue[3][soundqueueregisteroffset | 3].freq = soundqueue[3][soundqueueregisteroffset | 3].freq & 0xff00 | value;
			break;
		case 0xbf:
			soundqueue[3][soundqueueregisteroffset | 3].freq = soundqueue[3][soundqueueregisteroffset | 3].freq & 0xff | (value << 8);
			break;
		case 0xc0:
			soundqueue[0][soundqueueregisteroffset].vol = value;
			break;
		case 0xc1:
			soundqueue[0][soundqueueregisteroffset | 1].vol = value;
			break;
		case 0xc2:
			soundqueue[0][soundqueueregisteroffset | 2].vol = value;
			break;
		case 0xc3:
			soundqueue[0][soundqueueregisteroffset | 3].vol = value;
			break;
		case 0xc4:
			soundqueue[1][soundqueueregisteroffset].vol = value;
			break;
		case 0xc5:
			soundqueue[1][soundqueueregisteroffset | 1].vol = value;
			break;
		case 0xc6:
			soundqueue[1][soundqueueregisteroffset | 2].vol = value;
			break;
		case 0xc7:
			soundqueue[1][soundqueueregisteroffset | 3].vol = value;
			break;
		case 0xc8:
			soundqueue[2][soundqueueregisteroffset].vol = value;
			break;
		case 0xc9:
			soundqueue[2][soundqueueregisteroffset | 1].vol = value;
			break;
		case 0xca:
			soundqueue[2][soundqueueregisteroffset | 2].vol = value;
			break;
		case 0xcb:
			soundqueue[2][soundqueueregisteroffset | 3].vol = value;
			break;
		case 0xcc:
			soundqueue[3][soundqueueregisteroffset].vol = value;
			break;
		case 0xcd:
			soundqueue[3][soundqueueregisteroffset | 1].vol = value;
			break;
		case 0xce:
			soundqueue[3][soundqueueregisteroffset | 2].vol = value;
			break;
		case 0xcf:
			soundqueue[3][soundqueueregisteroffset | 3].vol = value;
			break;
		case 0xf8:
			fvmcdest = value << 8;
			break;
		case 0xf9:
			fvmcsrc = value << 8;
			break;
		case 0xfa:
			// TODO: set video flags
			break;
		case 0xfc:
			syspflags[value] = syspflags[value] & ~pageflags::PF_DONTLOG;
			break;
		case 0xfd:
			syspflags[value] = syspflags[value] | pageflags::PF_DONTLOG;
			break;
		case 0xfe:
			stackbase = GetSP() + value;
			break;
		case 0xff:
			if (value)
				DoMenu(3);
			break;
		}
	}
}

int main(int argc, char** argv)
{
	if (int rv = InitPaths())
		return rv;
	if (int rv = ReadArgs(argc, argv))
		return rv;
	LoadConfig();
	CollapsePaths();
	if (int rv = InitMainWindow())
		return rv;
	if (int rv = InitMemory())
		return rv;
	if (int rv = InitSound())
		return rv;
	menuhassound = true;
	PickAndInsertCartridge();
	menuhassound = false;
	if (int rv = InitEmulator())
		return rv;
	int resetcounter = 0;
	int scanline = 0;
	bool nextframe = false;
	SDL_TimerID frame_timer_id = SDL_AddTimer(16, SetBool, &nextframe);
	SDL_Surface* mwsurface = SDL_GetWindowSurface(mainwindow);
	while (true)
	{
		sysram[0x02fe] = scanline;
		sysram[0x02ff] = scanline >> 8;
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			unsigned char inkey;
			switch (event.type)
			{
			case SDL_KEYDOWN:
				if (inkey = keytrans[(event.key.keysym.sym | (event.key.keysym.sym >> 22)) & 0x1ff])
				{
					keypressregister = inkey;
					if (event.key.keysym.mod & KMOD_SHIFT)
						keypressregister ^= 0x20;
					if (event.key.keysym.mod & KMOD_CTRL)
						keypressregister ^= 0x60;
					if (event.key.keysym.mod & KMOD_ALT)
						keypressregister ^= 0x80;
					break;
				}
				switch (event.key.keysym.sym)
				{
				case SDLK_KP_ENTER:
				case SDLK_KP_0:
					DoMenu(0);
					break;
				case SDLK_KP_1:
					DoMenu(1);
					break;
				case SDLK_KP_2:
					DoMenu(2);
					break;
				case SDLK_KP_3:
					DoMenu(3);
					break;
				case SDLK_KP_4:
					DoMenu(4);
					break;
				case SDLK_KP_5:
					DoMenu(5);
					break;
				case SDLK_KP_6:
					DoMenu(6);
					break;
				case SDLK_KP_7:
					DoMenu(7);
					break;
				case SDLK_KP_8:
					DoMenu(8);
					break;
				case SDLK_KP_9:
					DoMenu(9);
					break;
				}
				break;
			case SDL_QUIT:
				goto cleanup;
			default:
				if (event.type == UE_RESETCPU)
				{
					LogReset();
					reset6502();
					stackbase = GetSP();
					resetcounter = 8955;
					LogStep();
				}
			}
		}
		// 6502 clocked at 4MHz, pixel clock at 34.96MHz)
		if (scanline < 576)
		{
			if (!resetcounter)
			{
				LogScanline(scanline);
				videobusy = true;
				videostate = 0x00 | videomode;
				// run 6502 for 21.967963386728 microseconds (768 pixel ticks, 88 cpu cycles)
				exec6502(88);
				RenderScanline(scanline, framebuffer, mwsurface);
				videobusy = false;
				videostate = 0x02 | videomode;
				// run 6502 for 5.949656750572 microseconds (208 pixel ticks, 24 cpu cycles)
				exec6502(12);
				videostate = 0x03 | videomode;
				exec6502(12);
			}
			else
				--resetcounter;
			++scanline;
		}
		else if (scanline < 597)
		{
			// TODO: issue IRQ if video interrupt condition is met
			if (!resetcounter)
			{
				// run 6502 for 27.9176201373 microseconds (976 pixel ticks, 112 cpu cycles)
				LogScanline(scanline);
				if (scanline == 576)
				{
					videostate = 0x20 | videomode;
					exec6502(32);
					videostate = 0x30 | videomode;
					exec6502(56);
					videostate = 0x32 | videomode;
					exec6502(12);
					videostate = 0x33 | videomode;
					exec6502(12);
				}
				else {
					videostate = 0x30 | videomode;
					exec6502(88);
					videostate = 0x32 | videomode;
					exec6502(12);
					videostate = 0x33 | videomode;
					exec6502(12);
				}
			}
			else
				--resetcounter;
			++scanline;
		}
		else if (nextframe)
		{
			scanline = 0;
			nextframe = false;
			SDL_UpdateWindowSurface(mainwindow);
			//RandomBitFlip();
		}
		else
		{
			SDL_Delay(1);
		}
	}
	SDL_RemoveTimer(frame_timer_id);
cleanup:
	SDL_CloseAudioDevice(sounddev);
	SDL_DestroyWindow(mainwindow);
	SDL_FreeSurface(cellcanvas);
	SDL_Quit();
	delete[] sysram;
	return 0;
}


