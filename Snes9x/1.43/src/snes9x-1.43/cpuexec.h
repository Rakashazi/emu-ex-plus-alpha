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

#ifndef _CPUEXEC_H_
#define _CPUEXEC_H_
#include "ppu.h"
#include "memmap.h"
#include "65c816.h"

#define DO_HBLANK_CHECK() \
    if (CPU.Cycles >= CPU.NextEvent) \
	S9xDoHBlankProcessing ();

struct SOpcodes {
#ifdef __WIN32__
	void (__cdecl *S9xOpcode)( void);
#else
	void (*S9xOpcode)( void);
#endif
};

struct SICPU
{
    uint8  *Speed;
    const struct SOpcodes *S9xOpcodes;
    uint8  _Carry;
    uint8  _Zero;
    uint8  _Negative;
    uint8  _Overflow;
    bool8  CPUExecuting;
    uint32 ShiftedPB;
    uint32 ShiftedDB;
    uint32 Frame;
    uint32 Scanline;
    uint32 FrameAdvanceCount;
};

START_EXTERN_C
void S9xMainLoop (void);
void S9xReset (void);
void S9xSoftReset (void);
void S9xDoHBlankProcessing ();
void S9xClearIRQ (uint32);
void S9xSetIRQ (uint32);

extern const struct SOpcodes S9xOpcodesE1 [256];
extern const struct SOpcodes S9xOpcodesM1X1 [256];
extern const struct SOpcodes S9xOpcodesM1X0 [256];
extern const struct SOpcodes S9xOpcodesM0X1 [256];
extern const struct SOpcodes S9xOpcodesM0X0 [256];

extern struct SICPU ICPU;
END_EXTERN_C

STATIC inline void S9xUnpackStatus()
{
    ICPU._Zero = (Registers.PL & Zero) == 0;
    ICPU._Negative = (Registers.PL & Negative);
    ICPU._Carry = (Registers.PL & Carry);
    ICPU._Overflow = (Registers.PL & Overflow) >> 6;
}

STATIC inline void S9xPackStatus()
{
    Registers.PL &= ~(Zero | Negative | Carry | Overflow);
    Registers.PL |= ICPU._Carry | ((ICPU._Zero == 0) << 1) |
		    (ICPU._Negative & 0x80) | (ICPU._Overflow << 6);
}

STATIC inline void CLEAR_IRQ_SOURCE (uint32 M)
{
    CPU.IRQActive &= ~M;
    if (!CPU.IRQActive)
	CPU.Flags &= ~IRQ_PENDING_FLAG;
}
	
STATIC inline void S9xFixCycles ()
{
    if (CheckEmulation ())
    {
	ICPU.S9xOpcodes = S9xOpcodesE1;
    }
    else
    if (CheckMemory ())
    {
	if (CheckIndex ())
	{
	    ICPU.S9xOpcodes = S9xOpcodesM1X1;
	}
	else
	{
	    ICPU.S9xOpcodes = S9xOpcodesM1X0;
	}
    }
    else
    {
	if (CheckIndex ())
	{
	    ICPU.S9xOpcodes = S9xOpcodesM0X1;
	}
	else
	{
	    ICPU.S9xOpcodes = S9xOpcodesM0X0;
	}
    }
}

STATIC inline void S9xReschedule ()
{
    uint8 which;
    long max;
    
    if (CPU.WhichEvent == HBLANK_START_EVENT ||
	CPU.WhichEvent == HTIMER_AFTER_EVENT)
    {
	which = HBLANK_END_EVENT;
	max = Settings.H_Max;
    }
    else
    {
	which = HBLANK_START_EVENT;
	max = Settings.HBlankStart;
    }

    if (PPU.HTimerEnabled &&
        (long) PPU.HTimerPosition < max &&
	(long) PPU.HTimerPosition > CPU.NextEvent &&
	(!PPU.VTimerEnabled ||
	 (PPU.VTimerEnabled && CPU.V_Counter == PPU.IRQVBeamPos)))
    {
	which = (long) PPU.HTimerPosition < Settings.HBlankStart ?
			HTIMER_BEFORE_EVENT : HTIMER_AFTER_EVENT;
	max = PPU.HTimerPosition;
    }
    CPU.NextEvent = max;
    CPU.WhichEvent = which;
}

#endif

