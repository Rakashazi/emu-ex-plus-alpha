/*******************************************************************************
  Snes9x - Portable Super Nintendo Entertainment System (TM) emulator.
 
  (c) Copyright 1996 - 2002 Gary Henderson (gary.henderson@ntlworld.com) and
                            Jerremy Koot (jkoot@snes9x.com)

  (c) Copyright 2001 - 2004 John Weidman (jweidman@slip.net)

  (c) Copyright 2002 - 2004 Brad Jorsch (anomie@users.sourceforge.net),
                            funkyass (funkyass@spam.shaw.ca),
                            Joel Yliluoma (http://iki.fi/bisqwit/)
                            Kris Bleakley (codeviolation@hotmail.com),
                            Matthew Kendora,
                            Nach (n-a-c-h@users.sourceforge.net),
                            Peter Bortas (peter@bortas.org) and
                            zones (kasumitokoduck@yahoo.com)

  C4 x86 assembler and some C emulation code
  (c) Copyright 2000 - 2003 zsKnight (zsknight@zsnes.com),
                            _Demo_ (_demo_@zsnes.com), and Nach

  C4 C++ code
  (c) Copyright 2003 Brad Jorsch

  DSP-1 emulator code
  (c) Copyright 1998 - 2004 Ivar (ivar@snes9x.com), _Demo_, Gary Henderson,
                            John Weidman, neviksti (neviksti@hotmail.com),
                            Kris Bleakley, Andreas Naive

  DSP-2 emulator code
  (c) Copyright 2003 Kris Bleakley, John Weidman, neviksti, Matthew Kendora, and
                     Lord Nightmare (lord_nightmare@users.sourceforge.net

  OBC1 emulator code
  (c) Copyright 2001 - 2004 zsKnight, pagefault (pagefault@zsnes.com) and
                            Kris Bleakley
  Ported from x86 assembler to C by sanmaiwashi

  SPC7110 and RTC C++ emulator code
  (c) Copyright 2002 Matthew Kendora with research by
                     zsKnight, John Weidman, and Dark Force

  S-DD1 C emulator code
  (c) Copyright 2003 Brad Jorsch with research by
                     Andreas Naive and John Weidman
 
  S-RTC C emulator code
  (c) Copyright 2001 John Weidman
  
  ST010 C++ emulator code
  (c) Copyright 2003 Feather, Kris Bleakley, John Weidman and Matthew Kendora

  Super FX x86 assembler emulator code 
  (c) Copyright 1998 - 2003 zsKnight, _Demo_, and pagefault 

  Super FX C emulator code 
  (c) Copyright 1997 - 1999 Ivar, Gary Henderson and John Weidman


  SH assembler code partly based on x86 assembler code
  (c) Copyright 2002 - 2004 Marcus Comstedt (marcus@mc.pp.se) 

 
  Specific ports contains the works of other authors. See headers in
  individual files.
 
  Snes9x homepage: http://www.snes9x.com
 
  Permission to use, copy, modify and distribute Snes9x in both binary and
  source form, for non-commercial purposes, is hereby granted without fee,
  providing that this license information and copyright notice appear with
  all copies and any derived work.
 
  This software is provided 'as-is', without any express or implied
  warranty. In no event shall the authors be held liable for any damages
  arising from the use of this software.
 
  Snes9x is freeware for PERSONAL USE only. Commercial users should
  seek permission of the copyright holders first. Commercial use includes
  charging money for Snes9x or software derived from Snes9x.
 
  The copyright holders request that bug fixes and improvements to the code
  should be forwarded to them so everyone can benefit from the modifications
  in future versions.
 
  Super NES and Super Nintendo Entertainment System are trademarks of
  Nintendo Co., Limited and its subsidiary companies.
*******************************************************************************/

#include "snes9x.h"
#include "memmap.h"
#include "ppu.h"
#include "dsp1.h"
#include "missing.h"
#include "cpuexec.h"
#include "debug.h"
#include "apu.h"
#include "dma.h"
#include "fxemu.h"
#include "gfx.h"
#include "soundux.h"
#include "cheats.h"
#include "sa1.h"
#include "netplay.h"
#include "spc7110.h"

START_EXTERN_C
char String[513];

long OpAddress = 0;

struct Missing missing;

struct SICPU ICPU;

struct SCPUState CPU = { 0 };

struct SRegisters Registers;

struct SAPU APU;

struct SIAPU IAPU __attribute__ ((aligned(4)));

struct SAPURegisters APURegisters;

struct SSettings Settings =
{
		0, // APUEnabled
    0, // Shutdown
    (int)SNES_CYCLES_PER_SCANLINE, // H_Max
    (256 * (int)SNES_CYCLES_PER_SCANLINE) / SNES_HCOUNTER_MAX, // HBlankStart;

		#ifdef NETPLAY_SUPPORT
    0, // Paused
    0, // ForcedPause
    0, // StopEmulation
		#endif

		#ifdef DEBUGGER
    0, // TraceDMA
    0, // TraceHDMA
    0, // TraceVRAM
    0, // TraceUnknownRegisters
    0, // TraceDSP
		#endif

    0, // ForcePAL
    0, // ForceNTSC
    0, // PAL;
    Settings.FrameTimeNTSC, // FrameTime;

    0, // ForceLoROM;
    0, // ForceHiROM;
    0, // ForceHeader;
    0, // ForceNoHeader;
    0, // ForceInterleaved;
    0, // ForceInterleaved2;
    0, // ForceNotInterleaved;

    0, // ForceSuperFX;
    0, // ForceNoSuperFX;
    0, // ForceDSP1;
    0, // ForceNoDSP1;
    0, // ForceSA1;
    0, // ForceNoSA1;
    0, // ForceC4;
    0, // ForceNoC4;
    0, // ForceSDD1;
    0, // ForceNoSDD1;
    0, // MultiPlayer5;
    0, // Mouse;
    0, // SuperScope;
    0, // SRTC;
    0, // ControllerOption;

    0, // MultiPlayer5Master;
    0, // SuperScopeMaster;
    0, // MouseMaster;
    0, // SuperFX;
    0, // DSP1Master;
    0, // SA1;
    0, // C4;
    0, // SDD1;
    0, // SPC7110;
    0, // SPC7110RTC;
    0, // OBC1;

    44100, // SoundPlaybackRate;
		#ifdef DEBUGGER
    0, // TraceSoundDSP;
		#endif
    TRUE, // Stereo;
};

struct SDSP1 DSP1;

struct SSA1Registers SA1Registers;

struct SSA1 SA1;

SSoundData SoundData;

SnesModel M1SNES={1,3,2};
SnesModel M2SNES={2,4,3};
SnesModel* Model=&M1SNES;


uint8 *SRAM = NULL;
uint8 *ROM = NULL;
uint8 *RegRAM = NULL;
uint8 *C4RAM = NULL;

CMemory Memory;

struct SSNESGameFixes SNESGameFixes;

unsigned char OpenBus = 0;


END_EXTERN_C

#ifndef ZSNES_FX
struct FxInit_s SuperFX;
#else
START_EXTERN_C
uint8 *SFXPlotTable = NULL;
END_EXTERN_C
#endif

struct SPPU PPU;
struct InternalPPU IPPU;

struct SDMA DMA[8];

uint8 *HDMAMemPointers [8];
//uint8 *HDMABasePointers [8];

struct SBG BG;

struct SGFX GFX {nullptr, 1024};

uint32 odd_high[4][16];
uint32 odd_low[4][16];
uint32 even_high[4][16];
uint32 even_low[4][16];

#ifdef GFX_MULTI_FORMAT

uint32 RED_LOW_BIT_MASK = RED_LOW_BIT_MASK_RGB565;
uint32 GREEN_LOW_BIT_MASK = GREEN_LOW_BIT_MASK_RGB565;
uint32 BLUE_LOW_BIT_MASK = BLUE_LOW_BIT_MASK_RGB565;
uint32 RED_HI_BIT_MASK = RED_HI_BIT_MASK_RGB565;
uint32 GREEN_HI_BIT_MASK = GREEN_HI_BIT_MASK_RGB565;
uint32 BLUE_HI_BIT_MASK = BLUE_HI_BIT_MASK_RGB565;
uint32 MAX_RED = MAX_RED_RGB565;
uint32 MAX_GREEN = MAX_GREEN_RGB565;
uint32 MAX_BLUE = MAX_BLUE_RGB565;
uint32 SPARE_RGB_BIT_MASK = SPARE_RGB_BIT_MASK_RGB565;
uint32 GREEN_HI_BIT = (MAX_GREEN_RGB565 + 1) >> 1;
uint32 RGB_LOW_BITS_MASK = (RED_LOW_BIT_MASK_RGB565 | 
			    GREEN_LOW_BIT_MASK_RGB565 |
			    BLUE_LOW_BIT_MASK_RGB565);
uint32 RGB_HI_BITS_MASK = (RED_HI_BIT_MASK_RGB565 |
			   GREEN_HI_BIT_MASK_RGB565 |
			   BLUE_HI_BIT_MASK_RGB565);
uint32 RGB_HI_BITS_MASKx2 = (RED_HI_BIT_MASK_RGB565 |
			     GREEN_HI_BIT_MASK_RGB565 |
			     BLUE_HI_BIT_MASK_RGB565) << 1;
uint32 RGB_REMOVE_LOW_BITS_MASK = ~RGB_LOW_BITS_MASK;
uint32 FIRST_COLOR_MASK = FIRST_COLOR_MASK_RGB565;
uint32 SECOND_COLOR_MASK = SECOND_COLOR_MASK_RGB565;
uint32 THIRD_COLOR_MASK = THIRD_COLOR_MASK_RGB565;
uint32 ALPHA_BITS_MASK = ALPHA_BITS_MASK_RGB565;
uint32 FIRST_THIRD_COLOR_MASK = 0;
uint32 TWO_LOW_BITS_MASK = 0;
uint32 HIGH_BITS_SHIFTED_TWO_MASK = 0;

uint32 current_graphic_format = RGB565;
#endif

struct SCheatData Cheat;


uint16 DirectColourMaps [8][256];


START_EXTERN_C

#ifdef NETPLAY_SUPPORT
struct SNetPlay NetPlay;
#endif

// Actual data used by CPU emulation, will be scaled by APUReset routine
// to be relative to the 65c816 instruction lengths.
int32 S9xAPUCycles [256] =
{
    /*        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, a, b, c, d, e, f, */
    /* 00 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 4, 6, 8,
    /* 10 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 6, 5, 2, 2, 4, 6,
    /* 20 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 4, 5, 4,
    /* 30 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 6, 5, 2, 2, 3, 8,
    /* 40 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 4, 6, 6,
    /* 50 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 4, 5, 2, 2, 4, 3,
    /* 60 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 4, 5, 5,
    /* 70 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2, 3, 6,
    /* 80 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 2, 4, 5,
    /* 90 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2,12, 5,
    /* a0 */  3, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 2, 4, 4,
    /* b0 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2, 3, 4,
    /* c0 */  3, 8, 4, 5, 4, 5, 4, 7, 2, 5, 6, 4, 5, 2, 4, 9,
    /* d0 */  2, 8, 4, 5, 5, 6, 6, 7, 4, 5, 4, 5, 2, 2, 6, 3,
    /* e0 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 4, 5, 3, 4, 3, 4, 3,
    /* f0 */  2, 8, 4, 5, 4, 5, 5, 6, 3, 4, 5, 4, 2, 2, 4, 3
};

END_EXTERN_C

