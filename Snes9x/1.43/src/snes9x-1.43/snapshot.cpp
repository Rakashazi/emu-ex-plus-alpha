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
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <ctype.h>
#include <stdlib.h>

#if defined(__unix) || defined(__linux) || defined(__sun) || defined(__DJGPP)
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "snapshot.h"
#include "snaporig.h"
#include "memmap.h"
#include "snes9x.h"
#include "65c816.h"
#include "ppu.h"
#include "cpuexec.h"
#include "display.h"
#include "apu.h"
#include "soundux.h"
#include "sa1.h"
#include "srtc.h"
#include "sdd1.h"
#include "spc7110.h"
#include "movie.h"

extern uint8 *SRAM;

#ifdef ZSNES_FX
START_EXTERN_C
void S9xSuperFXPreSaveState ();
void S9xSuperFXPostSaveState ();
void S9xSuperFXPostLoadState ();
END_EXTERN_C
#endif

bool8 S9xUnfreezeZSNES (const char *filename);

typedef struct {
    int offset;
    int size;
    int type;
} FreezeData;

enum {
    INT_V, uint8_ARRAY_V, uint16_ARRAY_V, uint32_ARRAY_V
};

#define Offset(field,structure) \
((int) (((char *) (&(((structure)NULL)->field))) - ((char *) NULL)))

#define COUNT(ARRAY) (sizeof (ARRAY) / sizeof (ARRAY[0]))

struct SnapshotMovieInfo
{
	uint32	MovieInputDataSize;
};

#undef OFFSET
#define OFFSET(f) Offset(f,struct SnapshotMovieInfo *)

static FreezeData SnapMovie [] = {
    {OFFSET (MovieInputDataSize), 4, INT_V},
};

#undef OFFSET
#define OFFSET(f) Offset(f,struct SCPUState *)

static FreezeData SnapCPU [] = {
    {OFFSET (Flags), 4, INT_V},
    {OFFSET (BranchSkip), 1, INT_V},
    {OFFSET (NMIActive), 1, INT_V},
    {OFFSET (IRQActive), 1, INT_V},
    {OFFSET (WaitingForInterrupt), 1, INT_V},
    {OFFSET (WhichEvent), 1, INT_V},
    {OFFSET (Cycles), 4, INT_V},
    {OFFSET (NextEvent), 4, INT_V},
    {OFFSET (V_Counter), 4, INT_V},
    {OFFSET (MemSpeed), 4, INT_V},
    {OFFSET (MemSpeedx2), 4, INT_V},
    {OFFSET (FastROMSpeed), 4, INT_V}
};

#undef OFFSET
#define OFFSET(f) Offset(f,struct SRegisters *)

static FreezeData SnapRegisters [] = {
    {OFFSET (PB),  1, INT_V},
    {OFFSET (DB),  1, INT_V},
    {OFFSET (P.W), 2, INT_V},
    {OFFSET (A.W), 2, INT_V},
    {OFFSET (D.W), 2, INT_V},
    {OFFSET (S.W), 2, INT_V},
    {OFFSET (X.W), 2, INT_V},
    {OFFSET (Y.W), 2, INT_V},
    {OFFSET (PC),  2, INT_V}
};

#undef OFFSET
#define OFFSET(f) Offset(f,struct SPPU *)

static FreezeData SnapPPU [] = {
    {OFFSET (BGMode), 1, INT_V},
    {OFFSET (BG3Priority), 1, INT_V},
    {OFFSET (Brightness), 1, INT_V},
    {OFFSET (VMA.High), 1, INT_V},
    {OFFSET (VMA.Increment), 1, INT_V},
    {OFFSET (VMA.Address), 2, INT_V},
    {OFFSET (VMA.Mask1), 2, INT_V},
    {OFFSET (VMA.FullGraphicCount), 2, INT_V},
    {OFFSET (VMA.Shift), 2, INT_V},
    {OFFSET (BG[0].SCBase), 2, INT_V},
    {OFFSET (BG[0].VOffset), 2, INT_V},
    {OFFSET (BG[0].HOffset), 2, INT_V},
    {OFFSET (BG[0].BGSize), 1, INT_V},
    {OFFSET (BG[0].NameBase), 2, INT_V},
    {OFFSET (BG[0].SCSize), 2, INT_V},
	
    {OFFSET (BG[1].SCBase), 2, INT_V},
    {OFFSET (BG[1].VOffset), 2, INT_V},
    {OFFSET (BG[1].HOffset), 2, INT_V},
    {OFFSET (BG[1].BGSize), 1, INT_V},
    {OFFSET (BG[1].NameBase), 2, INT_V},
    {OFFSET (BG[1].SCSize), 2, INT_V},
	
    {OFFSET (BG[2].SCBase), 2, INT_V},
    {OFFSET (BG[2].VOffset), 2, INT_V},
    {OFFSET (BG[2].HOffset), 2, INT_V},
    {OFFSET (BG[2].BGSize), 1, INT_V},
    {OFFSET (BG[2].NameBase), 2, INT_V},
    {OFFSET (BG[2].SCSize), 2, INT_V},
	
    {OFFSET (BG[3].SCBase), 2, INT_V},
    {OFFSET (BG[3].VOffset), 2, INT_V},
    {OFFSET (BG[3].HOffset), 2, INT_V},
    {OFFSET (BG[3].BGSize), 1, INT_V},
    {OFFSET (BG[3].NameBase), 2, INT_V},
    {OFFSET (BG[3].SCSize), 2, INT_V},
	
    {OFFSET (CGFLIP), 1, INT_V},
    {OFFSET (CGDATA), 256, uint16_ARRAY_V},
    {OFFSET (FirstSprite), 1, INT_V},
#define O(N) \
    {OFFSET (OBJ[N].HPos), 2, INT_V}, \
    {OFFSET (OBJ[N].VPos), 2, INT_V}, \
    {OFFSET (OBJ[N].Name), 2, INT_V}, \
    {OFFSET (OBJ[N].VFlip), 1, INT_V}, \
    {OFFSET (OBJ[N].HFlip), 1, INT_V}, \
    {OFFSET (OBJ[N].Priority), 1, INT_V}, \
    {OFFSET (OBJ[N].Palette), 1, INT_V}, \
    {OFFSET (OBJ[N].Size), 1, INT_V}
	
    O(  0), O(  1), O(  2), O(  3), O(  4), O(  5), O(  6), O(  7),
    O(  8), O(  9), O( 10), O( 11), O( 12), O( 13), O( 14), O( 15),
    O( 16), O( 17), O( 18), O( 19), O( 20), O( 21), O( 22), O( 23),
    O( 24), O( 25), O( 26), O( 27), O( 28), O( 29), O( 30), O( 31),
    O( 32), O( 33), O( 34), O( 35), O( 36), O( 37), O( 38), O( 39),
    O( 40), O( 41), O( 42), O( 43), O( 44), O( 45), O( 46), O( 47),
    O( 48), O( 49), O( 50), O( 51), O( 52), O( 53), O( 54), O( 55),
    O( 56), O( 57), O( 58), O( 59), O( 60), O( 61), O( 62), O( 63),
    O( 64), O( 65), O( 66), O( 67), O( 68), O( 69), O( 70), O( 71),
    O( 72), O( 73), O( 74), O( 75), O( 76), O( 77), O( 78), O( 79),
    O( 80), O( 81), O( 82), O( 83), O( 84), O( 85), O( 86), O( 87),
    O( 88), O( 89), O( 90), O( 91), O( 92), O( 93), O( 94), O( 95),
    O( 96), O( 97), O( 98), O( 99), O(100), O(101), O(102), O(103),
    O(104), O(105), O(106), O(107), O(108), O(109), O(110), O(111),
    O(112), O(113), O(114), O(115), O(116), O(117), O(118), O(119),
    O(120), O(121), O(122), O(123), O(124), O(125), O(126), O(127),
#undef O
    {OFFSET (OAMPriorityRotation), 1, INT_V},
    {OFFSET (OAMAddr), 2, INT_V},
    {OFFSET (OAMFlip), 1, INT_V},
    {OFFSET (OAMTileAddress), 2, INT_V},
    {OFFSET (IRQVBeamPos), 2, INT_V},
    {OFFSET (IRQHBeamPos), 2, INT_V},
    {OFFSET (VBeamPosLatched), 2, INT_V},
    {OFFSET (HBeamPosLatched), 2, INT_V},
    {OFFSET (HBeamFlip), 1, INT_V},
    {OFFSET (VBeamFlip), 1, INT_V},
    {OFFSET (HVBeamCounterLatched), 1, INT_V},
    {OFFSET (MatrixA), 2, INT_V},
    {OFFSET (MatrixB), 2, INT_V},
    {OFFSET (MatrixC), 2, INT_V},
    {OFFSET (MatrixD), 2, INT_V},
    {OFFSET (CentreX), 2, INT_V},
    {OFFSET (CentreY), 2, INT_V},
    {OFFSET (Joypad1ButtonReadPos), 1, INT_V},
    {OFFSET (Joypad2ButtonReadPos), 1, INT_V},
    {OFFSET (Joypad3ButtonReadPos), 1, INT_V},
    {OFFSET (CGADD), 1, INT_V},
    {OFFSET (FixedColourRed), 1, INT_V},
    {OFFSET (FixedColourGreen), 1, INT_V},
    {OFFSET (FixedColourBlue), 1, INT_V},
    {OFFSET (SavedOAMAddr), 2, INT_V},
    {OFFSET (ScreenHeight), 2, INT_V},
    {OFFSET (WRAM), 4, INT_V},
    {OFFSET (ForcedBlanking), 1, INT_V},
    {OFFSET (OBJNameSelect), 2, INT_V},
    {OFFSET (OBJSizeSelect), 1, INT_V},
    {OFFSET (OBJNameBase), 2, INT_V},
    {OFFSET (OAMReadFlip), 1, INT_V},
    {OFFSET (VTimerEnabled), 1, INT_V},
    {OFFSET (HTimerEnabled), 1, INT_V},
    {OFFSET (HTimerPosition), 2, INT_V},
    {OFFSET (Mosaic), 1, INT_V},
    {OFFSET (Mode7HFlip), 1, INT_V},
    {OFFSET (Mode7VFlip), 1, INT_V},
    {OFFSET (Mode7Repeat), 1, INT_V},
    {OFFSET (Window1Left), 1, INT_V},
    {OFFSET (Window1Right), 1, INT_V},
    {OFFSET (Window2Left), 1, INT_V},
    {OFFSET (Window2Right), 1, INT_V},
#define O(N) \
    {OFFSET (ClipWindowOverlapLogic[N]), 1, INT_V}, \
    {OFFSET (ClipWindow1Enable[N]), 1, INT_V}, \
    {OFFSET (ClipWindow2Enable[N]), 1, INT_V}, \
    {OFFSET (ClipWindow1Inside[N]), 1, INT_V}, \
    {OFFSET (ClipWindow2Inside[N]), 1, INT_V}
	
    O(0), O(1), O(2), O(3), O(4), O(5),
	
#undef O
	
    {OFFSET (CGFLIPRead), 1, INT_V},
    {OFFSET (Need16x8Mulitply), 1, INT_V},
    {OFFSET (BGMosaic), 4, uint8_ARRAY_V},
    {OFFSET (OAMData), 512 + 32, uint8_ARRAY_V},
    {OFFSET (Need16x8Mulitply), 1, INT_V},
    {OFFSET (MouseSpeed), 2, uint8_ARRAY_V}
};

#undef OFFSET
#define OFFSET(f) Offset(f,struct SDMA *)

static FreezeData SnapDMA [] = {
#define O(N) \
    {OFFSET (TransferDirection) + N * (int)sizeof (struct SDMA), 1, INT_V}, \
    {OFFSET (AAddressFixed) + N * (int)sizeof (struct SDMA), 1, INT_V}, \
    {OFFSET (AAddressDecrement) + N * (int)sizeof (struct SDMA), 1, INT_V}, \
    {OFFSET (TransferMode) + N * (int)sizeof (struct SDMA), 1, INT_V}, \
    {OFFSET (ABank) + N * (int)sizeof (struct SDMA), 1, INT_V}, \
    {OFFSET (AAddress) + N * (int)sizeof (struct SDMA), 2, INT_V}, \
    {OFFSET (Address) + N * (int)sizeof (struct SDMA), 2, INT_V}, \
    {OFFSET (BAddress) + N * (int)sizeof (struct SDMA), 1, INT_V}, \
    {OFFSET (TransferBytes) + N * (int)sizeof (struct SDMA), 2, INT_V}, \
    {OFFSET (HDMAIndirectAddressing) + N * (int)sizeof (struct SDMA), 1, INT_V}, \
    {OFFSET (IndirectAddress) + N * (int)sizeof (struct SDMA), 2, INT_V}, \
    {OFFSET (IndirectBank) + N * (int)sizeof (struct SDMA), 1, INT_V}, \
    {OFFSET (Repeat) + N * (int)sizeof (struct SDMA), 1, INT_V}, \
    {OFFSET (LineCount) + N * (int)sizeof (struct SDMA), 1, INT_V}, \
    {OFFSET (FirstLine) + N * (int)sizeof (struct SDMA), 1, INT_V}
	
    O(0), O(1), O(2), O(3), O(4), O(5), O(6), O(7)
#undef O
};

#undef OFFSET
#define OFFSET(f) Offset(f,struct SAPU *)

static FreezeData SnapAPU [] = {
    {OFFSET (Cycles), 4, INT_V},
    {OFFSET (ShowROM), 1, INT_V},
    {OFFSET (Flags), 1, INT_V},
    {OFFSET (KeyedChannels), 1, INT_V},
    {OFFSET (OutPorts), 4, uint8_ARRAY_V},
    {OFFSET (DSP), 0x80, uint8_ARRAY_V},
    {OFFSET (ExtraRAM), 64, uint8_ARRAY_V},
    {OFFSET (Timer), 3, uint16_ARRAY_V},
    {OFFSET (TimerTarget), 3, uint16_ARRAY_V},
    {OFFSET (TimerEnabled), 3, uint8_ARRAY_V},
    {OFFSET (TimerValueWritten), 3, uint8_ARRAY_V}
};

#undef OFFSET
#define OFFSET(f) Offset(f,struct SAPURegisters *)

static FreezeData SnapAPURegisters [] = {
    {OFFSET (P), 1, INT_V},
    {OFFSET (YA.W), 2, INT_V},
    {OFFSET (X), 1, INT_V},
    {OFFSET (S), 1, INT_V},
    {OFFSET (PC), 2, INT_V},
};

#undef OFFSET
#define OFFSET(f) Offset(f,SSoundData *)

static FreezeData SnapSoundData [] = {
    {OFFSET (master_volume_left), 2, INT_V},
    {OFFSET (master_volume_right), 2, INT_V},
    {OFFSET (echo_volume_left), 2, INT_V},
    {OFFSET (echo_volume_right), 2, INT_V},
    {OFFSET (echo_enable), 4, INT_V},
    {OFFSET (echo_feedback), 4, INT_V},
    {OFFSET (echo_ptr), 4, INT_V},
    {OFFSET (echo_buffer_size), 4, INT_V},
    {OFFSET (echo_write_enabled), 4, INT_V},
    {OFFSET (echo_channel_enable), 4, INT_V},
    {OFFSET (pitch_mod), 4, INT_V},
    {OFFSET (dummy), 3, uint32_ARRAY_V},
#define O(N) \
    {OFFSET (channels [N].state), 4, INT_V}, \
    {OFFSET (channels [N].type), 4, INT_V}, \
    {OFFSET (channels [N].volume_left), 2, INT_V}, \
    {OFFSET (channels [N].volume_right), 2, INT_V}, \
    {OFFSET (channels [N].hertz), 4, INT_V}, \
    {OFFSET (channels [N].count), 4, INT_V}, \
    {OFFSET (channels [N].loop), 1, INT_V}, \
    {OFFSET (channels [N].envx), 4, INT_V}, \
    {OFFSET (channels [N].left_vol_level), 2, INT_V}, \
    {OFFSET (channels [N].right_vol_level), 2, INT_V}, \
    {OFFSET (channels [N].envx_target), 2, INT_V}, \
    {OFFSET (channels [N].env_error), 4, INT_V}, \
    {OFFSET (channels [N].erate), 4, INT_V}, \
    {OFFSET (channels [N].direction), 4, INT_V}, \
    {OFFSET (channels [N].attack_rate), 4, INT_V}, \
    {OFFSET (channels [N].decay_rate), 4, INT_V}, \
    {OFFSET (channels [N].sustain_rate), 4, INT_V}, \
    {OFFSET (channels [N].release_rate), 4, INT_V}, \
    {OFFSET (channels [N].sustain_level), 4, INT_V}, \
    {OFFSET (channels [N].sample), 2, INT_V}, \
    {OFFSET (channels [N].decoded), 16, uint16_ARRAY_V}, \
    {OFFSET (channels [N].previous16), 2, uint16_ARRAY_V}, \
    {OFFSET (channels [N].sample_number), 2, INT_V}, \
    {OFFSET (channels [N].last_block), 1, INT_V}, \
    {OFFSET (channels [N].needs_decode), 1, INT_V}, \
    {OFFSET (channels [N].block_pointer), 4, INT_V}, \
    {OFFSET (channels [N].sample_pointer), 4, INT_V}, \
    {OFFSET (channels [N].mode), 4, INT_V}
	
    O(0), O(1), O(2), O(3), O(4), O(5), O(6), O(7)
#undef O
};

#undef OFFSET
#define OFFSET(f) Offset(f,struct SSA1Registers *)

static FreezeData SnapSA1Registers [] = {
    {OFFSET (PB),  1, INT_V},
    {OFFSET (DB),  1, INT_V},
    {OFFSET (P.W), 2, INT_V},
    {OFFSET (A.W), 2, INT_V},
    {OFFSET (D.W), 2, INT_V},
    {OFFSET (S.W), 2, INT_V},
    {OFFSET (X.W), 2, INT_V},
    {OFFSET (Y.W), 2, INT_V},
    {OFFSET (PC),  2, INT_V}
};

#undef OFFSET
#define OFFSET(f) Offset(f,struct SSA1 *)

static FreezeData SnapSA1 [] = {
    {OFFSET (Flags), 4, INT_V},
    {OFFSET (NMIActive), 1, INT_V},
    {OFFSET (IRQActive), 1, INT_V},
    {OFFSET (WaitingForInterrupt), 1, INT_V},
    {OFFSET (op1), 2, INT_V},
    {OFFSET (op2), 2, INT_V},
    {OFFSET (arithmetic_op), 4, INT_V},
    {OFFSET (sum), 8, INT_V},
    {OFFSET (overflow), 1, INT_V}
};

#undef OFFSET
#define OFFSET(f) Offset(f,struct SPC7110EmuVars *)

static FreezeData SnapSPC7110 [] = {
    {OFFSET (reg4800), 1, INT_V},
    {OFFSET (reg4801), 1, INT_V},
    {OFFSET (reg4802), 1, INT_V},
    {OFFSET (reg4803), 1, INT_V},
    {OFFSET (reg4804), 1, INT_V},
    {OFFSET (reg4805), 1, INT_V},
    {OFFSET (reg4806), 1, INT_V},
    {OFFSET (reg4807), 1, INT_V},
    {OFFSET (reg4808), 1, INT_V},
    {OFFSET (reg4809), 1, INT_V},
    {OFFSET (reg480A), 1, INT_V},
    {OFFSET (reg480B), 1, INT_V},
    {OFFSET (reg480C), 1, INT_V},
    {OFFSET (reg4811), 1, INT_V},
    {OFFSET (reg4812), 1, INT_V},
    {OFFSET (reg4813), 1, INT_V},
    {OFFSET (reg4814), 1, INT_V},
    {OFFSET (reg4815), 1, INT_V},
    {OFFSET (reg4816), 1, INT_V},
    {OFFSET (reg4817), 1, INT_V},
    {OFFSET (reg4818), 1, INT_V},
    {OFFSET (reg4820), 1, INT_V},
    {OFFSET (reg4821), 1, INT_V},
    {OFFSET (reg4822), 1, INT_V},
    {OFFSET (reg4823), 1, INT_V},
    {OFFSET (reg4824), 1, INT_V},
    {OFFSET (reg4825), 1, INT_V},
    {OFFSET (reg4826), 1, INT_V},
    {OFFSET (reg4827), 1, INT_V},
    {OFFSET (reg4828), 1, INT_V},
    {OFFSET (reg4829), 1, INT_V},
    {OFFSET (reg482A), 1, INT_V},
    {OFFSET (reg482B), 1, INT_V},
    {OFFSET (reg482C), 1, INT_V},
    {OFFSET (reg482D), 1, INT_V},
    {OFFSET (reg482E), 1, INT_V},
    {OFFSET (reg482F), 1, INT_V},
    {OFFSET (reg4830), 1, INT_V},
    {OFFSET (reg4831), 1, INT_V},
    {OFFSET (reg4832), 1, INT_V},
    {OFFSET (reg4833), 1, INT_V},
    {OFFSET (reg4834), 1, INT_V},
    {OFFSET (reg4840), 1, INT_V},
    {OFFSET (reg4841), 1, INT_V},
    {OFFSET (reg4842), 1, INT_V},
    {OFFSET (AlignBy), 1, INT_V},
    {OFFSET (written), 1, INT_V},
    {OFFSET (offset_add), 1, INT_V},
    {OFFSET (DataRomOffset), 4, INT_V},
    {OFFSET (DataRomSize), 4, INT_V},
    {OFFSET (bank50Internal), 4, INT_V},
	{OFFSET (bank50), 0x10000, uint8_ARRAY_V}
};

#undef OFFSET
#define OFFSET(f) Offset(f,struct SPC7110RTC *)

static FreezeData SnapS7RTC [] = {
    {OFFSET (reg), 16, uint8_ARRAY_V},
    {OFFSET (index), 2, INT_V},
    {OFFSET (control), 1, INT_V},
    {OFFSET (init), 1, INT_V},
	{OFFSET (last_used),4,INT_V}
};

static char ROMFilename [_MAX_PATH];
//static char SnapshotFilename [_MAX_PATH];

static void FreezeStruct (STREAM stream, const char *name, void *base, FreezeData *fields,
				   int num_fields);
static void FreezeBlock (STREAM stream, const char *name, uint8 *block, int size);

static int UnfreezeStruct (STREAM stream, const char *name, void *base, FreezeData *fields,
					int num_fields);
static int UnfreezeBlock (STREAM stream, const char *name, uint8 *block, int size);

static int UnfreezeStructCopy (STREAM stream, const char *name, uint8** block, FreezeData *fields, int num_fields);

static void UnfreezeStructFromCopy (void *base, FreezeData *fields, int num_fields, uint8* block);

static int UnfreezeBlockCopy (STREAM stream, const char *name, uint8** block, int size);

bool8 Snapshot (const char *filename)
{
    return (S9xFreezeGame (filename));
}

bool8 S9xFreezeGame (const char *filename)
{
    STREAM stream = NULL;
	
    if (S9xOpenSnapshotFile (filename, FALSE, &stream))
    {
		S9xFreezeToStream (stream);
		S9xCloseSnapshotFile (stream);

		if(S9xMovieActive())
		{
			sprintf(String, "Movie snapshot %s", S9xBasename (filename));
			S9xMessage (S9X_INFO, S9X_FREEZE_FILE_INFO, String);
		}
		else
		{
			sprintf(String, "Saved %s", S9xBasename (filename));
			S9xMessage (S9X_INFO, S9X_FREEZE_FILE_INFO, String);
		}

		return (TRUE);
    }
    return (FALSE);
}

bool8 S9xLoadSnapshot (const char *filename)
{
    return (S9xUnfreezeGame (filename));
}

bool8 S9xUnfreezeGame (const char *filename)
{
    /*if (S9xLoadOrigSnapshot (filename))
		return (TRUE);*/
	
    if (S9xUnfreezeZSNES (filename))
		return (TRUE);
	
    STREAM snapshot = NULL;
    if (S9xOpenSnapshotFile (filename, TRUE, &snapshot))
    {
		int result;
		if ((result = S9xUnfreezeFromStream (snapshot)) != SUCCESS)
		{
			switch (result)
			{
			case WRONG_FORMAT:
				S9xMessage (S9X_ERROR, S9X_WRONG_FORMAT, 
					"File not in Snes9x freeze format");
				break;
			case WRONG_VERSION:
				S9xMessage (S9X_ERROR, S9X_WRONG_VERSION,
					"Incompatable Snes9x freeze file format version");
				break;
			case WRONG_MOVIE_SNAPSHOT:
				S9xMessage (S9X_ERROR, S9X_WRONG_MOVIE_SNAPSHOT, 0/*MOVIE_ERR_SNAPSHOT_WRONG_MOVIE*/);
				break;
			case NOT_A_MOVIE_SNAPSHOT:
				S9xMessage (S9X_ERROR, S9X_NOT_A_MOVIE_SNAPSHOT, 0/*MOVIE_ERR_SNAPSHOT_NOT_MOVIE*/);
				break;
			default:
			case FILE_NOT_FOUND:
				sprintf (String, "ROM image \"%s\" for freeze file not found",
					ROMFilename);
				S9xMessage (S9X_ERROR, S9X_ROM_NOT_FOUND, String);
				break;
			}
			S9xCloseSnapshotFile (snapshot);
			return (FALSE);
		}

		if(!S9xMovieActive())
		{
			sprintf(String, "Loaded %s", S9xBasename (filename));
			S9xMessage (S9X_INFO, S9X_FREEZE_FILE_INFO, String);
		}

		S9xCloseSnapshotFile (snapshot);
		return (TRUE);
    }
    return (FALSE);
}

void S9xFreezeToStream (STREAM stream)
{
    char buffer [1024];
    int i;
	
    S9xSetSoundMute (TRUE);
#ifdef ZSNES_FX
    if (Settings.SuperFX)
		S9xSuperFXPreSaveState ();
#endif
	
	S9xUpdateRTC();
    S9xSRTCPreSaveState ();
	
    for (i = 0; i < 8; i++)
    {
		SoundData.channels [i].previous16 [0] = (int16) SoundData.channels [i].previous [0];
		SoundData.channels [i].previous16 [1] = (int16) SoundData.channels [i].previous [1];
    }
    sprintf (buffer, "%s:%04d\n", SNAPSHOT_MAGIC, SNAPSHOT_VERSION);
    WRITE_STREAM (buffer, strlen (buffer), stream);
    sprintf (buffer, "NAM:%06d:%s%c", (int)Memory.ROMFilename.size() + 1,
		Memory.ROMFilename.c_str(), 0);
    WRITE_STREAM (buffer, strlen (buffer) + 1, stream);
    FreezeStruct (stream, "CPU", &CPU, SnapCPU, COUNT (SnapCPU));
    FreezeStruct (stream, "REG", &Registers, SnapRegisters, COUNT (SnapRegisters));
    FreezeStruct (stream, "PPU", &PPU, SnapPPU, COUNT (SnapPPU));
    FreezeStruct (stream, "DMA", DMA, SnapDMA, COUNT (SnapDMA));

	// RAM and VRAM
    FreezeBlock (stream, "VRA", Memory.VRAM, 0x10000);
    FreezeBlock (stream, "RAM", Memory.RAM, 0x20000);
    FreezeBlock (stream, "SRA", ::SRAM, 0x20000);
    FreezeBlock (stream, "FIL", Memory.FillRAM, 0x8000);
    if (Settings.APUEnabled)
    {
		// APU
		FreezeStruct (stream, "APU", &APU, SnapAPU, COUNT (SnapAPU));
		FreezeStruct (stream, "ARE", &APURegisters, SnapAPURegisters,
			COUNT (SnapAPURegisters));
		FreezeBlock (stream, "ARA", IAPU.RAM, 0x10000);
		FreezeStruct (stream, "SOU", &SoundData, SnapSoundData,
			COUNT (SnapSoundData));
    }
    if (Settings.SA1)
    {
		SA1Registers.PC = SA1.PC - SA1.PCBase;
		S9xSA1PackStatus ();
		FreezeStruct (stream, "SA1", &SA1, SnapSA1, COUNT (SnapSA1));
		FreezeStruct (stream, "SAR", &SA1Registers, SnapSA1Registers, 
			COUNT (SnapSA1Registers));
    }
	
	if (Settings.SPC7110)
    {
		FreezeStruct (stream, "SP7", &s7r, SnapSPC7110, COUNT (SnapSPC7110));
    }
	if(Settings.SPC7110RTC)
	{
		FreezeStruct (stream, "RTC", &rtc_f9, SnapS7RTC, COUNT (SnapS7RTC));
	}

	if (S9xMovieActive ())
	{
		uint8* movie_freeze_buf;
		uint32 movie_freeze_size;

		S9xMovieFreeze(&movie_freeze_buf, &movie_freeze_size);
		if(movie_freeze_buf)
		{
			struct SnapshotMovieInfo mi;
			mi.MovieInputDataSize = movie_freeze_size;
			FreezeStruct (stream, "MOV", &mi, SnapMovie, COUNT (SnapMovie));
		    FreezeBlock (stream, "MID", movie_freeze_buf, movie_freeze_size);
			delete [] movie_freeze_buf;
		}
	}

	S9xSetSoundMute (FALSE);
#ifdef ZSNES_FX
	if (Settings.SuperFX)
		S9xSuperFXPostSaveState ();
#endif
}

int S9xUnfreezeFromStream (STREAM stream)
{
    char buffer [_MAX_PATH + 1];
    char rom_filename [_MAX_PATH + 1];
    int result;
	
    int version;
    int len = strlen (SNAPSHOT_MAGIC) + 1 + 4 + 1;
    if (READ_STREAM (buffer, len, stream) != len)
		return (WRONG_FORMAT);
    if (strncmp (buffer, SNAPSHOT_MAGIC, strlen (SNAPSHOT_MAGIC)) != 0)
		return (WRONG_FORMAT);
    if ((version = atoi (&buffer [strlen (SNAPSHOT_MAGIC) + 1])) > SNAPSHOT_VERSION)
		return (WRONG_VERSION);
	
    if ((result = UnfreezeBlock (stream, "NAM", (uint8 *) rom_filename, _MAX_PATH)) != SUCCESS)
		return (result);
	
    if (strcasecmp (rom_filename, Memory.ROMFilename.c_str()) != 0 &&
		strcasecmp (S9xBasename (rom_filename), S9xBasename (Memory.ROMFilename.c_str())) != 0)
    {
		S9xMessage (S9X_WARNING, S9X_FREEZE_ROM_NAME,
			"Current loaded ROM image doesn't match that required by freeze-game file.");
    }
	
// ## begin load ##
	uint8* local_cpu = NULL;
	uint8* local_registers = NULL;
	uint8* local_ppu = NULL;
	uint8* local_dma = NULL;
	uint8* local_vram = NULL;
	uint8* local_ram = NULL;
	uint8* local_sram = NULL;
	uint8* local_fillram = NULL;
	uint8* local_apu = NULL;
	uint8* local_apu_registers = NULL;
	uint8* local_apu_ram = NULL;
	uint8* local_apu_sounddata = NULL;
	uint8* local_sa1 = NULL;
	uint8* local_sa1_registers = NULL;
	uint8* local_spc = NULL;
	uint8* local_spc_rtc = NULL;
	uint8* local_movie_data = NULL;

	do
	{
		if ((result = UnfreezeStructCopy (stream, "CPU", &local_cpu, SnapCPU, COUNT (SnapCPU))) != SUCCESS)
			break;
		if ((result = UnfreezeStructCopy (stream, "REG", &local_registers, SnapRegisters, COUNT (SnapRegisters))) != SUCCESS)
			break;
		if ((result = UnfreezeStructCopy (stream, "PPU", &local_ppu, SnapPPU, COUNT (SnapPPU))) != SUCCESS)
			break;
		if ((result = UnfreezeStructCopy (stream, "DMA", &local_dma, SnapDMA, COUNT (SnapDMA))) != SUCCESS)
			break;
		if ((result = UnfreezeBlockCopy (stream, "VRA", &local_vram, 0x10000)) != SUCCESS)
			break;
		if ((result = UnfreezeBlockCopy (stream, "RAM", &local_ram, 0x20000)) != SUCCESS)
			break;
		if ((result = UnfreezeBlockCopy (stream, "SRA", &local_sram, 0x20000)) != SUCCESS)
			break;
		if ((result = UnfreezeBlockCopy (stream, "FIL", &local_fillram, 0x8000)) != SUCCESS)
			break;
		if (UnfreezeStructCopy (stream, "APU", &local_apu, SnapAPU, COUNT (SnapAPU)) == SUCCESS)
		{
			if ((result = UnfreezeStructCopy (stream, "ARE", &local_apu_registers, SnapAPURegisters, COUNT (SnapAPURegisters))) != SUCCESS)
				break;
			if ((result = UnfreezeBlockCopy (stream, "ARA", &local_apu_ram, 0x10000)) != SUCCESS)
				break;
			if ((result = UnfreezeStructCopy (stream, "SOU", &local_apu_sounddata, SnapSoundData, COUNT (SnapSoundData))) != SUCCESS)
				break;
		}
		if ((result = UnfreezeStructCopy (stream, "SA1", &local_sa1, SnapSA1, COUNT(SnapSA1))) == SUCCESS)
		{
			if ((result = UnfreezeStructCopy (stream, "SAR", &local_sa1_registers, SnapSA1Registers, COUNT (SnapSA1Registers))) != SUCCESS)
				break;
		}
		
		if ((result = UnfreezeStructCopy (stream, "SP7", &local_spc, SnapSPC7110, COUNT(SnapSPC7110))) != SUCCESS)
		{
			if(Settings.SPC7110)
				break;
		}
		if ((result = UnfreezeStructCopy (stream, "RTC", &local_spc_rtc, SnapS7RTC, COUNT (SnapS7RTC))) != SUCCESS)
		{
			if(Settings.SPC7110RTC)
				break;
		}

		if (S9xMovieActive ())
		{
			SnapshotMovieInfo mi;
			if ((result = UnfreezeStruct (stream, "MOV", &mi, SnapMovie, COUNT(SnapMovie))) != SUCCESS)
			{
				result = NOT_A_MOVIE_SNAPSHOT;
				break;
			}

			if ((result = UnfreezeBlockCopy (stream, "MID", &local_movie_data, mi.MovieInputDataSize)) != SUCCESS)
			{
				result = NOT_A_MOVIE_SNAPSHOT;
				break;
			}

			if (!S9xMovieUnfreeze(local_movie_data, mi.MovieInputDataSize))
			{
				result = WRONG_MOVIE_SNAPSHOT;
				break;
			}
		}

		result=SUCCESS;

	} while(false);
// ## end load ##

	if (result == SUCCESS)
	{
		uint32 old_flags = CPU.Flags;
		uint32 sa1_old_flags = SA1.Flags;
		S9xReset ();
		S9xSetSoundMute (TRUE);

		UnfreezeStructFromCopy (&CPU, SnapCPU, COUNT (SnapCPU), local_cpu);
		UnfreezeStructFromCopy (&Registers, SnapRegisters, COUNT (SnapRegisters), local_registers);
		UnfreezeStructFromCopy (&PPU, SnapPPU, COUNT (SnapPPU), local_ppu);
		UnfreezeStructFromCopy (DMA, SnapDMA, COUNT (SnapDMA), local_dma);
		memcpy (Memory.VRAM, local_vram, 0x10000);
		memcpy (Memory.RAM, local_ram, 0x20000);
		memcpy (::SRAM, local_sram, 0x20000);
		memcpy (Memory.FillRAM, local_fillram, 0x8000);
		if(local_apu)
		{
			UnfreezeStructFromCopy (&APU, SnapAPU, COUNT (SnapAPU), local_apu);
			UnfreezeStructFromCopy (&APURegisters, SnapAPURegisters, COUNT (SnapAPURegisters), local_apu_registers);
			memcpy (IAPU.RAM, local_apu_ram, 0x10000);
			UnfreezeStructFromCopy (&SoundData, SnapSoundData, COUNT (SnapSoundData), local_apu_sounddata);
		}
		if(local_sa1)
		{
			UnfreezeStructFromCopy (&SA1, SnapSA1, COUNT (SnapSA1), local_sa1);
			UnfreezeStructFromCopy (&SA1Registers, SnapSA1Registers, COUNT (SnapSA1Registers), local_sa1_registers);
		}
		if(local_spc)
		{
			UnfreezeStructFromCopy (&s7r, SnapSPC7110, COUNT (SnapSPC7110), local_spc);
		}
		if(local_spc_rtc)
		{
			UnfreezeStructFromCopy (&rtc_f9, SnapS7RTC, COUNT (SnapS7RTC), local_spc_rtc);
		}

		Memory.FixROMSpeed ();
		CPU.Flags |= old_flags & (DEBUG_MODE_FLAG | TRACE_FLAG |
			SINGLE_STEP_FLAG | FRAME_ADVANCE_FLAG);

		#ifndef NO_COLOR_CHANGE_TRACKING
	    IPPU.ColorsChanged = TRUE;
		#endif
		IPPU.OBJChanged = TRUE;
		CPU.InDMA = FALSE;
		S9xFixColourBrightness ();
		IPPU.RenderThisFrame = FALSE;

		if (local_apu)
		{
			S9xSetSoundMute (FALSE);
			IAPU.PC = IAPU.RAM + APURegisters.PC;
			S9xAPUUnpackStatus ();
			if (APUCheckDirectPage ())
				IAPU.DirectPage = IAPU.RAM + 0x100;
			else
				IAPU.DirectPage = IAPU.RAM;
			Settings.APUEnabled = TRUE;
			IAPU.APUExecuting = TRUE;
		}
		else
		{
			Settings.APUEnabled = FALSE;
			IAPU.APUExecuting = FALSE;
			S9xSetSoundMute (TRUE);
		}

		if (local_sa1)
		{
			S9xFixSA1AfterSnapshotLoad ();
			SA1.Flags |= sa1_old_flags & (TRACE_FLAG);
		}

		if (local_spc_rtc)
		{
			S9xUpdateRTC();
		}

		S9xFixSoundAfterSnapshotLoad ();

		uint8 hdma_byte = Memory.FillRAM[0x420c];
		S9xSetCPU(hdma_byte, 0x420c);

		if(!Memory.FillRAM[0x4213]){
			// most likely an old savestate
			Memory.FillRAM[0x4213]=Memory.FillRAM[0x4201];
			if(!Memory.FillRAM[0x4213])
				Memory.FillRAM[0x4213]=Memory.FillRAM[0x4201]=0xFF;
		}

		ICPU.ShiftedPB = Registers.PB << 16;
		ICPU.ShiftedDB = Registers.DB << 16;
		S9xSetPCBase (ICPU.ShiftedPB + Registers.PC);
		S9xUnpackStatus ();
		S9xFixCycles ();
//		S9xReschedule ();				// <-- this causes desync when recording or playing movies

#ifdef ZSNES_FX
		if (Settings.SuperFX)
			S9xSuperFXPostLoadState ();
#endif
		
		S9xSRTCPostLoadState ();
		if (Settings.SDD1)
			S9xSDD1PostLoadState ();
			
		IAPU.NextAPUTimerPos = CPU.Cycles * 10000L;
		IAPU.APUTimerCounter = 0; 
	}

	if (local_cpu)           delete [] local_cpu;
	if (local_registers)     delete [] local_registers;
	if (local_ppu)           delete [] local_ppu;
	if (local_dma)           delete [] local_dma;
	if (local_vram)          delete [] local_vram;
	if (local_ram)           delete [] local_ram;
	if (local_sram)          delete [] local_sram;
	if (local_fillram)       delete [] local_fillram;
	if (local_apu)           delete [] local_apu;
	if (local_apu_registers) delete [] local_apu_registers;
	if (local_apu_ram)       delete [] local_apu_ram;
	if (local_apu_sounddata) delete [] local_apu_sounddata;
	if (local_sa1)           delete [] local_sa1;
	if (local_sa1_registers) delete [] local_sa1_registers;
	if (local_spc)           delete [] local_spc;
	if (local_spc_rtc)       delete [] local_spc_rtc;
	if (local_movie_data)    delete [] local_movie_data;

	return (result);
}

int FreezeSize (int size, int type)
{
    switch (type)
    {
    case uint16_ARRAY_V:
		return (size * 2);
    case uint32_ARRAY_V:
		return (size * 4);
    default:
		return (size);
    }
}

static void FreezeStruct (STREAM stream, const char *name, void *base, FreezeData *fields,
				   int num_fields)
{
    // Work out the size of the required block
    int len = 0;
    int i;
    int j;
	
    for (i = 0; i < num_fields; i++)
    {
		if (fields [i].offset + FreezeSize (fields [i].size, 
			fields [i].type) > len)
			len = fields [i].offset + FreezeSize (fields [i].size, 
			fields [i].type);
    }
	
    uint8 *block = new uint8 [len];
    uint8 *ptr = block;
    uint16 word;
    uint32 dword;
    int64  qword;
	
    // Build the block ready to be streamed out
    for (i = 0; i < num_fields; i++)
    {
		switch (fields [i].type)
		{
		case INT_V:
			switch (fields [i].size)
			{
			case 1:
				*ptr++ = *((uint8 *) base + fields [i].offset);
				break;
			case 2:
				word = *((uint16 *) ((uint8 *) base + fields [i].offset));
				*ptr++ = (uint8) (word >> 8);
				*ptr++ = (uint8) word;
				break;
			case 4:
				dword = *((uint32 *) ((uint8 *) base + fields [i].offset));
				*ptr++ = (uint8) (dword >> 24);
				*ptr++ = (uint8) (dword >> 16);
				*ptr++ = (uint8) (dword >> 8);
				*ptr++ = (uint8) dword;
				break;
			case 8:
				qword = *((int64 *) ((uint8 *) base + fields [i].offset));
				*ptr++ = (uint8) (qword >> 56);
				*ptr++ = (uint8) (qword >> 48);
				*ptr++ = (uint8) (qword >> 40);
				*ptr++ = (uint8) (qword >> 32);
				*ptr++ = (uint8) (qword >> 24);
				*ptr++ = (uint8) (qword >> 16);
				*ptr++ = (uint8) (qword >> 8);
				*ptr++ = (uint8) qword;
				break;
			}
			break;
			case uint8_ARRAY_V:
				memmove (ptr, (uint8 *) base + fields [i].offset, fields [i].size);
				ptr += fields [i].size;
				break;
			case uint16_ARRAY_V:
				for (j = 0; j < fields [i].size; j++)
				{
					word = *((uint16 *) ((uint8 *) base + fields [i].offset + j * 2));
					*ptr++ = (uint8) (word >> 8);
					*ptr++ = (uint8) word;
				}
				break;
			case uint32_ARRAY_V:
				for (j = 0; j < fields [i].size; j++)
				{
					dword = *((uint32 *) ((uint8 *) base + fields [i].offset + j * 4));
					*ptr++ = (uint8) (dword >> 24);
					*ptr++ = (uint8) (dword >> 16);
					*ptr++ = (uint8) (dword >> 8);
					*ptr++ = (uint8) dword;
				}
				break;
		}
    }
	
    FreezeBlock (stream, name, block, len);
    delete[] block;
}

static void FreezeBlock (STREAM stream, const char *name, uint8 *block, int size)
{
    char buffer [512];
    sprintf (buffer, "%s:%06d:", name, size);
    WRITE_STREAM (buffer, strlen (buffer), stream);
    WRITE_STREAM (block, size, stream);
    
}

static int UnfreezeStruct (STREAM stream, const char *name, void *base, FreezeData *fields,
					int num_fields)
{
    // Work out the size of the required block
    int len = 0;
    int i;
    int j;
	
    for (i = 0; i < num_fields; i++)
    {
		if (fields [i].offset + FreezeSize (fields [i].size, 
			fields [i].type) > len)
			len = fields [i].offset + FreezeSize (fields [i].size, 
			fields [i].type);
    }
	
    uint8 *block = new uint8 [len];
    uint8 *ptr = block;
    uint16 word;
    uint32 dword;
    int64  qword;
    int result;
	
    if ((result = UnfreezeBlock (stream, name, block, len)) != SUCCESS)
    {
		delete[] block;
		return (result);
    }
	
    // Unpack the block of data into a C structure
    for (i = 0; i < num_fields; i++)
    {
		switch (fields [i].type)
		{
		case INT_V:
			switch (fields [i].size)
			{
			case 1:
				*((uint8 *) base + fields [i].offset) = *ptr++;
				break;
			case 2:
				word  = *ptr++ << 8;
				word |= *ptr++;
				*((uint16 *) ((uint8 *) base + fields [i].offset)) = word;
				break;
			case 4:
				dword  = *ptr++ << 24;
				dword |= *ptr++ << 16;
				dword |= *ptr++ << 8;
				dword |= *ptr++;
				*((uint32 *) ((uint8 *) base + fields [i].offset)) = dword;
				break;
			case 8:
				qword  = (int64) *ptr++ << 56;
				qword |= (int64) *ptr++ << 48;
				qword |= (int64) *ptr++ << 40;
				qword |= (int64) *ptr++ << 32;
				qword |= (int64) *ptr++ << 24;
				qword |= (int64) *ptr++ << 16;
				qword |= (int64) *ptr++ << 8;
				qword |= (int64) *ptr++;
				*((int64 *) ((uint8 *) base + fields [i].offset)) = qword;
				break;
			}
			break;
			case uint8_ARRAY_V:
				memmove ((uint8 *) base + fields [i].offset, ptr, fields [i].size);
				ptr += fields [i].size;
				break;
			case uint16_ARRAY_V:
				for (j = 0; j < fields [i].size; j++)
				{
					word  = *ptr++ << 8;
					word |= *ptr++;
					*((uint16 *) ((uint8 *) base + fields [i].offset + j * 2)) = word;
				}
				break;
			case uint32_ARRAY_V:
				for (j = 0; j < fields [i].size; j++)
				{
					dword  = *ptr++ << 24;
					dword |= *ptr++ << 16;
					dword |= *ptr++ << 8;
					dword |= *ptr++;
					*((uint32 *) ((uint8 *) base + fields [i].offset + j * 4)) = dword;
				}
				break;
		}
    }
	
    delete [] block;
    return (result);
}

static int UnfreezeBlock (STREAM stream, const char *name, uint8 *block, int size)
{
    char buffer [20];
    int len = 0;
    int rem = 0;
    int rew_len;
    if (READ_STREAM (buffer, 11, stream) != 11 ||
		strncmp (buffer, name, 3) != 0 || buffer [3] != ':' ||
		(len = atoi (&buffer [4])) == 0)
    {
		REVERT_STREAM(stream, FIND_STREAM(stream)-11, 0);
		return (WRONG_FORMAT);
    }

    if (len > size)
    {
		rem = len - size;
		len = size;
    }
    if ((rew_len=READ_STREAM (block, len, stream)) != len)
	{
		REVERT_STREAM(stream, FIND_STREAM(stream)-11-rew_len, 0);
		return (WRONG_FORMAT);
	}
    if (rem)
    {
		char *junk = new char [rem];
		READ_STREAM (junk, rem, stream);
		delete [] junk;
    }
	
    return (SUCCESS);
}

static int UnfreezeStructCopy (STREAM stream, const char *name, uint8** block, FreezeData *fields, int num_fields)
{
    // Work out the size of the required block
    int len = 0;
    int i;
	
    for (i = 0; i < num_fields; i++)
    {
		if (fields [i].offset + FreezeSize (fields [i].size, 
			fields [i].type) > len)
			len = fields [i].offset + FreezeSize (fields [i].size, 
			fields [i].type);
    }
	
    return (UnfreezeBlockCopy (stream, name, block, len));
}

static void UnfreezeStructFromCopy (void *base, FreezeData *fields, int num_fields, uint8* block)
{
	int i;
	int j;
    uint8 *ptr = block;
    uint16 word;
    uint32 dword;
    int64  qword;
	
    // Unpack the block of data into a C structure
    for (i = 0; i < num_fields; i++)
    {
		switch (fields [i].type)
		{
		case INT_V:
			switch (fields [i].size)
			{
			case 1:
				*((uint8 *) base + fields [i].offset) = *ptr++;
				break;
			case 2:
				word  = *ptr++ << 8;
				word |= *ptr++;
				*((uint16 *) ((uint8 *) base + fields [i].offset)) = word;
				break;
			case 4:
				dword  = *ptr++ << 24;
				dword |= *ptr++ << 16;
				dword |= *ptr++ << 8;
				dword |= *ptr++;
				*((uint32 *) ((uint8 *) base + fields [i].offset)) = dword;
				break;
			case 8:
				qword  = (int64) *ptr++ << 56;
				qword |= (int64) *ptr++ << 48;
				qword |= (int64) *ptr++ << 40;
				qword |= (int64) *ptr++ << 32;
				qword |= (int64) *ptr++ << 24;
				qword |= (int64) *ptr++ << 16;
				qword |= (int64) *ptr++ << 8;
				qword |= (int64) *ptr++;
				*((int64 *) ((uint8 *) base + fields [i].offset)) = qword;
				break;
			}
			break;
			case uint8_ARRAY_V:
				memmove ((uint8 *) base + fields [i].offset, ptr, fields [i].size);
				ptr += fields [i].size;
				break;
			case uint16_ARRAY_V:
				for (j = 0; j < fields [i].size; j++)
				{
					word  = *ptr++ << 8;
					word |= *ptr++;
					*((uint16 *) ((uint8 *) base + fields [i].offset + j * 2)) = word;
				}
				break;
			case uint32_ARRAY_V:
				for (j = 0; j < fields [i].size; j++)
				{
					dword  = *ptr++ << 24;
					dword |= *ptr++ << 16;
					dword |= *ptr++ << 8;
					dword |= *ptr++;
					*((uint32 *) ((uint8 *) base + fields [i].offset + j * 4)) = dword;
				}
				break;
		}
    }
}

static int UnfreezeBlockCopy (STREAM stream, const char *name, uint8** block, int size)
{
    *block = new uint8 [size];
    int result;
	
    if ((result = UnfreezeBlock (stream, name, *block, size)) != SUCCESS)
    {
		delete [] (*block);
		*block = NULL;
		return (result);
    }
	
    return (result);
}

extern uint8 spc_dump_dsp[0x100];

bool8 S9xSPCDump (const char *filename)
{
    static uint8 header [] = {
		'S', 'N', 'E', 'S', '-', 'S', 'P', 'C', '7', '0', '0', ' ',
			'S', 'o', 'u', 'n', 'd', ' ', 'F', 'i', 'l', 'e', ' ',
			'D', 'a', 't', 'a', ' ', 'v', '0', '.', '3', '0', 26, 26, 26
    };
    static uint8 version = {
		0x1e
    };
	
    FILE *fs;
	
    S9xSetSoundMute (TRUE);
	
    if (!(fs = fopen (filename, "wb")))
		return (FALSE);
	
    // The SPC file format:
    // 0000: header:	'SNES-SPC700 Sound File Data v0.30',26,26,26
    // 0036: version:	$1e
    // 0037: SPC700 PC:
    // 0039: SPC700 A:
    // 0040: SPC700 X:
    // 0041: SPC700 Y:
    // 0042: SPC700 P:
    // 0043: SPC700 S:
    // 0044: Reserved: 0, 0, 0, 0
    // 0048: Title of game: 32 bytes
    // 0000: Song name: 32 bytes
    // 0000: Name of dumper: 32 bytes
    // 0000: Comments: 32 bytes
    // 0000: Date of SPC dump: 4 bytes
    // 0000: Fade out time in milliseconds: 4 bytes
    // 0000: Fade out length in milliseconds: 2 bytes
    // 0000: Default channel enables: 1 bytes
    // 0000: Emulator used to dump .SPC files: 1 byte, 1 == ZSNES
    // 0000: Reserved: 36 bytes
    // 0256: SPC700 RAM: 64K
    // ----: DSP Registers: 256 bytes
	
    if (fwrite (header, sizeof (header), 1, fs) != 1 ||
		fputc (version, fs) == EOF ||
		fseek (fs, 37, SEEK_SET) == EOF ||
		fputc (APURegisters.PC & 0xff, fs) == EOF ||
		fputc (APURegisters.PC >> 8, fs) == EOF ||
		fputc (APURegisters.YA.B.A, fs) == EOF ||
		fputc (APURegisters.X, fs) == EOF ||
		fputc (APURegisters.YA.B.Y, fs) == EOF ||
		fputc (APURegisters.P, fs) == EOF ||
		fputc (APURegisters.S, fs) == EOF ||
		fseek (fs, 256, SEEK_SET) == EOF ||
		fwrite (IAPU.RAM, 0x10000, 1, fs) != 1 ||
		fwrite (spc_dump_dsp, 1, 256, fs) != 256 ||
		fwrite (APU.ExtraRAM, 64, 1, fs) != 1 ||
		fclose (fs) < 0)
    {
		S9xSetSoundMute (FALSE);
		return (FALSE);
    }
    S9xSetSoundMute (FALSE);
    return (TRUE);
}

bool8 S9xUnfreezeZSNES (const char *filename)
{
    FILE *fs;
    uint8 t [4000];
	
    if (!(fs = fopen (filename, "rb")))
		return (FALSE);
	
    if (fread (t, 64, 1, fs) == 1 &&
		strncmp ((char *) t, "ZSNES Save State File V0.6", 26) == 0)
    {
		S9xReset ();
		S9xSetSoundMute (TRUE);
		
		// 28 Curr cycle
		CPU.V_Counter = READ_WORD (&t [29]);
		// 33 instrset
		Settings.APUEnabled = t [36];
		
		// 34 bcycpl cycles per scanline
		// 35 cycphb cyclers per hblank
		
		Registers.A.W   = READ_WORD (&t [41]);
		Registers.DB    = t [43];
		Registers.PB    = t [44];
		Registers.S.W   = READ_WORD (&t [45]);
		Registers.D.W   = READ_WORD (&t [47]);
		Registers.X.W   = READ_WORD (&t [49]);
		Registers.Y.W   = READ_WORD (&t [51]);
		Registers.P.W   = READ_WORD (&t [53]);
		Registers.PC    = READ_WORD (&t [55]);
		
		fread (t, 1, 8, fs);
		fread (t, 1, 3019, fs);
		S9xSetCPU (t [2], 0x4200);
		Memory.FillRAM [0x4210] = t [3];
		PPU.IRQVBeamPos = READ_WORD (&t [4]);
		PPU.IRQHBeamPos = READ_WORD (&t [2527]);
		PPU.Brightness = t [6];
		PPU.ForcedBlanking = t [8] >> 7;
		
		int i;
		for (i = 0; i < 544; i++)
			S9xSetPPU (t [0464 + i], 0x2104);
		
		PPU.OBJNameBase = READ_WORD (&t [9]);
		PPU.OBJNameSelect = READ_WORD (&t [13]) - PPU.OBJNameBase;
		switch (t [18])
		{
		case 4:
			if (t [17] == 1)
				PPU.OBJSizeSelect = 0;
			else
				PPU.OBJSizeSelect = 6;
			break;
		case 16:
			if (t [17] == 1)
				PPU.OBJSizeSelect = 1;
			else
				PPU.OBJSizeSelect = 3;
			break;
		default:
		case 64:
			if (t [17] == 1)
				PPU.OBJSizeSelect = 2;
			else
				if (t [17] == 4)
					PPU.OBJSizeSelect = 4;
				else
					PPU.OBJSizeSelect = 5;
				break;
		}
		PPU.OAMAddr = READ_WORD (&t [25]);
		PPU.SavedOAMAddr =  READ_WORD (&t [27]);
		PPU.FirstSprite = t [29];
		PPU.BGMode = t [30];
		PPU.BG3Priority = t [31];
		PPU.BG[0].BGSize = (t [32] >> 0) & 1;
		PPU.BG[1].BGSize = (t [32] >> 1) & 1;
		PPU.BG[2].BGSize = (t [32] >> 2) & 1;
		PPU.BG[3].BGSize = (t [32] >> 3) & 1;
		PPU.Mosaic = t [33] + 1;
		PPU.BGMosaic [0] = (t [34] & 1) != 0;
		PPU.BGMosaic [1] = (t [34] & 2) != 0;
		PPU.BGMosaic [2] = (t [34] & 4) != 0;
		PPU.BGMosaic [3] = (t [34] & 8) != 0;
		PPU.BG [0].SCBase = READ_WORD (&t [35]) >> 1;
		PPU.BG [1].SCBase = READ_WORD (&t [37]) >> 1;
		PPU.BG [2].SCBase = READ_WORD (&t [39]) >> 1;
		PPU.BG [3].SCBase = READ_WORD (&t [41]) >> 1;
		PPU.BG [0].SCSize = t [67];
		PPU.BG [1].SCSize = t [68];
		PPU.BG [2].SCSize = t [69];
		PPU.BG [3].SCSize = t [70];
		PPU.BG[0].NameBase = READ_WORD (&t [71]) >> 1;
		PPU.BG[1].NameBase = READ_WORD (&t [73]) >> 1;
		PPU.BG[2].NameBase = READ_WORD (&t [75]) >> 1;
		PPU.BG[3].NameBase = READ_WORD (&t [77]) >> 1;
		PPU.BG[0].HOffset = READ_WORD (&t [79]);
		PPU.BG[1].HOffset = READ_WORD (&t [81]);
		PPU.BG[2].HOffset = READ_WORD (&t [83]);
		PPU.BG[3].HOffset = READ_WORD (&t [85]);
		PPU.BG[0].VOffset = READ_WORD (&t [89]);
		PPU.BG[1].VOffset = READ_WORD (&t [91]);
		PPU.BG[2].VOffset = READ_WORD (&t [93]);
		PPU.BG[3].VOffset = READ_WORD (&t [95]);
		PPU.VMA.Increment = READ_WORD (&t [97]) >> 1;
		PPU.VMA.High = t [99];
#ifndef CORRECT_VRAM_READS
                IPPU.FirstVRAMRead = t [100];
#endif
		S9xSetPPU (t [2512], 0x2115);
		PPU.VMA.Address = READ_DWORD (&t [101]);
		for (i = 0; i < 512; i++)
			S9xSetPPU (t [1488 + i], 0x2122);
		
		PPU.CGADD = (uint8) READ_WORD (&t [105]);
		Memory.FillRAM [0x212c] = t [108];
		Memory.FillRAM [0x212d] = t [109];
		PPU.ScreenHeight = READ_WORD (&t [111]);
		Memory.FillRAM [0x2133] = t [2526];
		Memory.FillRAM [0x4202] = t [113];
		Memory.FillRAM [0x4204] = t [114];
		Memory.FillRAM [0x4205] = t [115];
		Memory.FillRAM [0x4214] = t [116];
		Memory.FillRAM [0x4215] = t [117];
		Memory.FillRAM [0x4216] = t [118];
		Memory.FillRAM [0x4217] = t [119];
		PPU.VBeamPosLatched = READ_WORD (&t [122]);
		PPU.HBeamPosLatched = READ_WORD (&t [120]);
		PPU.Window1Left = t [127];
		PPU.Window1Right = t [128];
		PPU.Window2Left = t [129];
		PPU.Window2Right = t [130];
		S9xSetPPU (t [131] | (t [132] << 4), 0x2123);
		S9xSetPPU (t [133] | (t [134] << 4), 0x2124);
		S9xSetPPU (t [135] | (t [136] << 4), 0x2125);
		S9xSetPPU (t [137], 0x212a);
		S9xSetPPU (t [138], 0x212b);
		S9xSetPPU (t [139], 0x212e);
		S9xSetPPU (t [140], 0x212f);
		S9xSetPPU (t [141], 0x211a);
		PPU.MatrixA = READ_WORD (&t [142]);
		PPU.MatrixB = READ_WORD (&t [144]);
		PPU.MatrixC = READ_WORD (&t [146]);
		PPU.MatrixD = READ_WORD (&t [148]);
		PPU.CentreX = READ_WORD (&t [150]);
		PPU.CentreY = READ_WORD (&t [152]);
		// JoyAPos t[154]
		// JoyBPos t[155]
		Memory.FillRAM [0x2134] = t [156]; // Matrix mult
		Memory.FillRAM [0x2135] = t [157]; // Matrix mult
		Memory.FillRAM [0x2136] = t [158]; // Matrix mult
		PPU.WRAM = READ_DWORD (&t [161]);
		
		for (i = 0; i < 128; i++)
			S9xSetCPU (t [165 + i], 0x4300 + i);
		
		if (t [294])
			CPU.IRQActive |= PPU_V_BEAM_IRQ_SOURCE | PPU_H_BEAM_IRQ_SOURCE;
		
		S9xSetCPU (t [296], 0x420c);
		// hdmadata t[297] + 8 * 19
		PPU.FixedColourRed = t [450];
		PPU.FixedColourGreen = t [451];
		PPU.FixedColourBlue = t [452];
		S9xSetPPU (t [454], 0x2130);
		S9xSetPPU (t [455], 0x2131);
		// vraminctype ...
		
		fread (Memory.RAM, 1, 128 * 1024, fs);
		fread (Memory.VRAM, 1, 64 * 1024, fs);
		
		if (Settings.APUEnabled)
		{
			// SNES SPC700 RAM (64K)
			fread (IAPU.RAM, 1, 64 * 1024, fs);
			
			// Junk 16 bytes
			fread (t, 1, 16, fs);
			
			// SNES SPC700 state and internal ZSNES SPC700 emulation state
			fread (t, 1, 304, fs);
			
			APURegisters.PC   = READ_DWORD (&t [0]);
			APURegisters.YA.B.A = t [4];
			APURegisters.X    = t [8];
			APURegisters.YA.B.Y = t [12];
			APURegisters.P    = t [16];
			APURegisters.S    = t [24];
			
			APU.Cycles = READ_DWORD (&t [32]);
			APU.ShowROM = (IAPU.RAM [0xf1] & 0x80) != 0;
			APU.OutPorts [0] = t [36];
			APU.OutPorts [1] = t [37];
			APU.OutPorts [2] = t [38];
			APU.OutPorts [3] = t [39];
			
			APU.TimerEnabled [0] = (t [40] & 1) != 0;
			APU.TimerEnabled [1] = (t [40] & 2) != 0;
			APU.TimerEnabled [2] = (t [40] & 4) != 0;
			S9xSetAPUTimer (0xfa, t [41]);
			S9xSetAPUTimer (0xfb, t [42]);
			S9xSetAPUTimer (0xfc, t [43]);
			APU.Timer [0] = t [44];
			APU.Timer [1] = t [45];
			APU.Timer [2] = t [46];
			
			memmove (APU.ExtraRAM, &t [48], 64);
			
			// Internal ZSNES sound DSP state
			fread (t, 1, 1068, fs);
			
			// SNES sound DSP register values
			fread (t, 1, 256, fs);
			
			uint8 saved = IAPU.RAM [0xf2];
			
			for (i = 0; i < 128; i++)
			{
				switch (i)
				{
				case APU_KON:
				case APU_KOFF:
					break;
				case APU_FLG:
					t [i] &= ~APU_SOFT_RESET;
				default:
					IAPU.RAM [0xf2] = i;
					S9xSetAPUDSP (t [i]);
					break;
				}
			}
			IAPU.RAM [0xf2] = APU_KON;
			S9xSetAPUDSP (t [APU_KON]);
			IAPU.RAM [0xf2] = saved;
			
			S9xSetSoundMute (FALSE);
			IAPU.PC = IAPU.RAM + APURegisters.PC;
			S9xAPUUnpackStatus ();
			if (APUCheckDirectPage ())
				IAPU.DirectPage = IAPU.RAM + 0x100;
			else
				IAPU.DirectPage = IAPU.RAM;
			Settings.APUEnabled = TRUE;
			IAPU.APUExecuting = TRUE;
		}
		else
		{
			Settings.APUEnabled = FALSE;
			IAPU.APUExecuting = FALSE;
			S9xSetSoundMute (TRUE);
		}
		
		if (Settings.SuperFX)
		{
			fread (::SRAM, 1, 64 * 1024, fs);
			fseek (fs, 64 * 1024, SEEK_CUR);
			fread (Memory.FillRAM + 0x7000, 1, 692, fs);
		}
		if (Settings.SA1)
		{
			fread (t, 1, 2741, fs);
			S9xSetSA1 (t [4], 0x2200);  // Control
			S9xSetSA1 (t [12], 0x2203);	// ResetV low
			S9xSetSA1 (t [13], 0x2204); // ResetV hi
			S9xSetSA1 (t [14], 0x2205); // NMI low
			S9xSetSA1 (t [15], 0x2206); // NMI hi
			S9xSetSA1 (t [16], 0x2207); // IRQ low
			S9xSetSA1 (t [17], 0x2208); // IRQ hi
			S9xSetSA1 (((READ_DWORD (&t [28]) - (4096*1024-0x6000))) >> 13, 0x2224);
			S9xSetSA1 (t [36], 0x2201);
			S9xSetSA1 (t [41], 0x2209);
			
			SA1Registers.A.W = READ_DWORD (&t [592]);
			SA1Registers.X.W = READ_DWORD (&t [596]);
			SA1Registers.Y.W = READ_DWORD (&t [600]);
			SA1Registers.D.W = READ_DWORD (&t [604]);
			SA1Registers.DB  = t [608];
			SA1Registers.PB  = t [612];
			SA1Registers.S.W = READ_DWORD (&t [616]);
			SA1Registers.PC  = READ_DWORD (&t [636]);
			SA1Registers.P.W = t [620] | (t [624] << 8);
			
			memmove (&Memory.FillRAM [0x3000], t + 692, 2 * 1024);
			
			fread (::SRAM, 1, 64 * 1024, fs);
			fseek (fs, 64 * 1024, SEEK_CUR);
			S9xFixSA1AfterSnapshotLoad ();
		}
		if(Settings.SPC7110)
		{
			uint32 temp;
			fread(&s7r.bank50, 1,0x10000, fs);
			
			//NEWSYM SPCMultA, dd 0  4820-23
			fread(&temp, 1, 4, fs);
			
			s7r.reg4820=temp&(0x0FF);
			s7r.reg4821=(temp>>8)&(0x0FF);
			s7r.reg4822=(temp>>16)&(0x0FF);
			s7r.reg4823=(temp>>24)&(0x0FF);

			//NEWSYM SPCMultB, dd 0				4824-5
			fread(&temp, 1,4,fs);
			s7r.reg4824=temp&(0x0FF);
			s7r.reg4825=(temp>>8)&(0x0FF);


			//NEWSYM SPCDivEnd, dd 0				4826-7
			fread(&temp, 1,4,fs);
			s7r.reg4826=temp&(0x0FF);
			s7r.reg4827=(temp>>8)&(0x0FF);

			//NEWSYM SPCMulRes, dd 0				4828-B
			fread(&temp, 1, 4, fs);
			
			s7r.reg4828=temp&(0x0FF);
			s7r.reg4829=(temp>>8)&(0x0FF);
			s7r.reg482A=(temp>>16)&(0x0FF);
			s7r.reg482B=(temp>>24)&(0x0FF);
			
			//NEWSYM SPCDivRes, dd 0				482C-D
			fread(&temp, 1,4,fs);
			s7r.reg482C=temp&(0x0FF);
			s7r.reg482D=(temp>>8)&(0x0FF);

			//NEWSYM SPC7110BankA, dd 020100h		4831-3
			fread(&temp, 1, 4, fs);
			
			s7r.reg4831=temp&(0x0FF);
			s7r.reg4832=(temp>>8)&(0x0FF);
			s7r.reg4833=(temp>>16)&(0x0FF);
			
			//NEWSYM SPC7110RTCStat, dd 0			4840,init,command, index
			fread(&temp, 1, 4, fs);
			
			s7r.reg4840=temp&(0x0FF);

//NEWSYM SPC7110RTC, db 00,00,00,00,00,00,01,00,01,00,00,00,00,00,0Fh,00
fread(&temp, 1, 4, fs);
if(Settings.SPC7110RTC)
{
	rtc_f9.reg[0]=temp&(0x0FF);
	rtc_f9.reg[1]=(temp>>8)&(0x0FF);
	rtc_f9.reg[2]=(temp>>16)&(0x0FF);
	rtc_f9.reg[3]=(temp>>24)&(0x0FF);
}
fread(&temp, 1, 4, fs);
if(Settings.SPC7110RTC)
{
	rtc_f9.reg[4]=temp&(0x0FF);
	rtc_f9.reg[5]=(temp>>8)&(0x0FF);
	rtc_f9.reg[6]=(temp>>16)&(0x0FF);
	rtc_f9.reg[7]=(temp>>24)&(0x0FF);
}
fread(&temp, 1, 4, fs);
if(Settings.SPC7110RTC)
{
	rtc_f9.reg[8]=temp&(0x0FF);
	rtc_f9.reg[9]=(temp>>8)&(0x0FF);
	rtc_f9.reg[10]=(temp>>16)&(0x0FF);
	rtc_f9.reg[11]=(temp>>24)&(0x0FF);
}
fread(&temp, 1, 4, fs);
if(Settings.SPC7110RTC)
{
	rtc_f9.reg[12]=temp&(0x0FF);
	rtc_f9.reg[13]=(temp>>8)&(0x0FF);
	rtc_f9.reg[14]=(temp>>16)&(0x0FF);
	rtc_f9.reg[15]=(temp>>24)&(0x0FF);
}
//NEWSYM SPC7110RTCB, db 00,00,00,00,00,00,01,00,01,00,00,00,00,01,0Fh,06
fread(&temp, 1, 4, fs);
fread(&temp, 1, 4, fs);
fread(&temp, 1, 4, fs);
fread(&temp, 1, 4, fs);

//NEWSYM SPCROMPtr, dd 0		4811-4813
			fread(&temp, 1, 4, fs);
			
			s7r.reg4811=temp&(0x0FF);
			s7r.reg4812=(temp>>8)&(0x0FF);
			s7r.reg4813=(temp>>16)&(0x0FF);
//NEWSYM SPCROMtoI, dd SPCROMPtr
			fread(&temp, 1, 4, fs);
//NEWSYM SPCROMAdj, dd 0      4814-5
			fread(&temp, 1, 4, fs);
			s7r.reg4814=temp&(0x0FF);
			s7r.reg4815=(temp>>8)&(0x0FF);
//NEWSYM SPCROMInc, dd 0		4816-7
			fread(&temp, 1, 4, fs);
			s7r.reg4816=temp&(0x0FF);
			s7r.reg4817=(temp>>8)&(0x0FF);
//NEWSYM SPCROMCom, dd 0		4818
fread(&temp, 1, 4, fs);
			
			s7r.reg4818=temp&(0x0FF);
//NEWSYM SPCCompPtr, dd 0  4801-4804 (+b50i) if"manual"
			fread(&temp, 1, 4, fs);

			//do table check
			
			s7r.reg4801=temp&(0x0FF);
			s7r.reg4802=(temp>>8)&(0x0FF);
			s7r.reg4803=(temp>>16)&(0x0FF);
			s7r.reg4804=(temp>>24)&(0x0FF);
///NEWSYM SPCDecmPtr, dd 0  4805-6   +b50i
			fread(&temp, 1, 4, fs);
			s7r.reg4805=temp&(0x0FF);
			s7r.reg4806=(temp>>8)&(0x0FF);
//NEWSYM SPCCompCounter, dd 0  4809-A
			fread(&temp, 1, 4, fs);
			s7r.reg4809=temp&(0x0FF);
			s7r.reg480A=(temp>>8)&(0x0FF);
//NEWSYM SPCCompCommand, dd 0  480B
fread(&temp, 1, 4, fs);
			
			s7r.reg480B=temp&(0x0FF);
//NEWSYM SPCCheckFix, dd 0		written(if 1, then set writtne to max value!)
fread(&temp, 1, 4, fs);
(temp&(0x0FF))?s7r.written=0x1F:s7r.written=0x00;
//NEWSYM SPCSignedVal, dd 0	482E
fread(&temp, 1, 4, fs);
			
			s7r.reg482E=temp&(0x0FF);
			
		}
		fclose (fs);
		
		Memory.FixROMSpeed ();
		#ifndef NO_COLOR_CHANGE_TRACKING
		IPPU.ColorsChanged = TRUE;
		#endif
		IPPU.OBJChanged = TRUE;
		CPU.InDMA = FALSE;
		S9xFixColourBrightness ();
		IPPU.RenderThisFrame = FALSE;
		
		S9xFixSoundAfterSnapshotLoad ();
		ICPU.ShiftedPB = Registers.PB << 16;
		ICPU.ShiftedDB = Registers.DB << 16;
		S9xSetPCBase (ICPU.ShiftedPB + Registers.PC);
		S9xUnpackStatus ();
		S9xFixCycles ();
		S9xReschedule ();
#ifdef ZSNES_FX
		if (Settings.SuperFX)
			S9xSuperFXPostLoadState ();
#endif
		return (TRUE);
    }
    fclose (fs);
    return (FALSE);
}

