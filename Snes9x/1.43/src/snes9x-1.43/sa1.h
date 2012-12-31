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

#ifndef _sa1_h_
#define _sa1_h_

#include "memmap.h"

struct SSA1Registers {
    uint8   PB;
    uint8   DB;
    pair    P;
    pair    A;
    pair    D;
    pair    S;
    pair    X;
    pair    Y;
    uint16  PC;
};

struct SSA1 {
    const struct  SOpcodes *S9xOpcodes;
    uint8   _Carry;
    uint8   _Zero;
    uint8   _Negative;
    uint8   _Overflow;
    bool8   CPUExecuting;
    uint32  ShiftedPB;
    uint32  ShiftedDB;
    uint32  Flags;
    bool8   Executing;
    bool8   NMIActive;
    bool8   IRQActive;
    bool8   WaitingForInterrupt;
    bool8   Waiting;
//    uint8   WhichEvent;
    uint8   *PC;
    uint8   *PCBase;
    uint8   *BWRAM;
    uint8   *PCAtOpcodeStart;
    uint8   *WaitAddress;
    uint32  WaitCounter;
    uint8   *WaitByteAddress1;
    uint8   *WaitByteAddress2;
//    long    Cycles;
//    long    NextEvent;
//    long    V_Counter;
    uint8   *Map [MEMMAP_NUM_BLOCKS];
    uint8   *WriteMap [MEMMAP_NUM_BLOCKS];
    int16   op1;
    int16   op2;
    int     arithmetic_op;
    int64   sum;
    bool8   overflow;
    uint8   VirtualBitmapFormat;
    bool8   in_char_dma;
    uint8   variable_bit_pos;
};

#define SA1CheckZero() (SA1._Zero == 0)
#define SA1CheckCarry() (SA1._Carry)
#define SA1CheckIRQ() (SA1Registers.PL & IRQ)
#define SA1CheckDecimal() (SA1Registers.PL & Decimal)
#define SA1CheckIndex() (SA1Registers.PL & IndexFlag)
#define SA1CheckMemory() (SA1Registers.PL & MemoryFlag)
#define SA1CheckOverflow() (SA1._Overflow)
#define SA1CheckNegative() (SA1._Negative & 0x80)
#define SA1CheckEmulation() (SA1Registers.P.W & Emulation)

#define SA1ClearFlags(f) (SA1Registers.P.W &= ~(f))
#define SA1SetFlags(f)   (SA1Registers.P.W |=  (f))
#define SA1CheckFlag(f)  (SA1Registers.PL & (f))


START_EXTERN_C
uint8 S9xSA1GetByte (uint32);
uint16 S9xSA1GetWord (uint32);
void S9xSA1SetByte (uint8, uint32);
void S9xSA1SetWord (uint16, uint32);
void S9xSA1SetPCBase (uint32);
uint8 S9xGetSA1 (uint32);
void S9xSetSA1 (uint8, uint32);

extern const struct SOpcodes S9xSA1OpcodesM1X1 [256];
extern const struct SOpcodes S9xSA1OpcodesM1X0 [256];
extern const struct SOpcodes S9xSA1OpcodesM0X1 [256];
extern const struct SOpcodes S9xSA1OpcodesM0X0 [256];
extern struct SSA1Registers SA1Registers;
extern struct SSA1 SA1;

void S9xSA1MainLoop ();
void S9xSA1Init ();
void S9xFixSA1AfterSnapshotLoad ();
void S9xSA1ExecuteDuringSleep ();
END_EXTERN_C

#define SNES_IRQ_SOURCE	    (1 << 7)
#define TIMER_IRQ_SOURCE    (1 << 6)
#define DMA_IRQ_SOURCE	    (1 << 5)

STATIC inline void S9xSA1UnpackStatus()
{
    SA1._Zero = (SA1Registers.PL & Zero) == 0;
    SA1._Negative = (SA1Registers.PL & Negative);
    SA1._Carry = (SA1Registers.PL & Carry);
    SA1._Overflow = (SA1Registers.PL & Overflow) >> 6;
}

STATIC inline void S9xSA1PackStatus()
{
    SA1Registers.PL &= ~(Zero | Negative | Carry | Overflow);
    SA1Registers.PL |= SA1._Carry | ((SA1._Zero == 0) << 1) |
		       (SA1._Negative & 0x80) | (SA1._Overflow << 6);
}

STATIC inline void S9xSA1FixCycles ()
{
    if (SA1CheckEmulation ())
	SA1.S9xOpcodes = S9xSA1OpcodesM1X1;
    else
    if (SA1CheckMemory ())
    {
	if (SA1CheckIndex ())
	    SA1.S9xOpcodes = S9xSA1OpcodesM1X1;
	else
	    SA1.S9xOpcodes = S9xSA1OpcodesM1X0;
    }
    else
    {
	if (SA1CheckIndex ())
	    SA1.S9xOpcodes = S9xSA1OpcodesM0X1;
	else
	    SA1.S9xOpcodes = S9xSA1OpcodesM0X0;
    }
}
#endif

