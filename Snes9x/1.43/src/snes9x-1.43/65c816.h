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

#ifndef _65c816_h_
#define _65c816_h_

#define AL A.B.l
#define AH A.B.h
#define XL X.B.l
#define XH X.B.h
#define YL Y.B.l
#define YH Y.B.h
#define SL S.B.l
#define SH S.B.h
#define DL D.B.l
#define DH D.B.h
#define PL P.B.l
#define PH P.B.h

#define Carry       1
#define Zero        2
#define IRQ         4
#define Decimal     8
#define IndexFlag  16
#define MemoryFlag 32
#define Overflow   64
#define Negative  128
#define Emulation 256

#define ClearCarry() (ICPU._Carry = 0)
#define SetCarry() (ICPU._Carry = 1)
#define SetZero() (ICPU._Zero = 0)
#define ClearZero() (ICPU._Zero = 1)
#define SetIRQ() (Registers.PL |= IRQ)
#define ClearIRQ() (Registers.PL &= ~IRQ)
#define SetDecimal() (Registers.PL |= Decimal)
#define ClearDecimal() (Registers.PL &= ~Decimal)
#define SetIndex() (Registers.PL |= IndexFlag)
#define ClearIndex() (Registers.PL &= ~IndexFlag)
#define SetMemory() (Registers.PL |= MemoryFlag)
#define ClearMemory() (Registers.PL &= ~MemoryFlag)
#define SetOverflow() (ICPU._Overflow = 1)
#define ClearOverflow() (ICPU._Overflow = 0)
#define SetNegative() (ICPU._Negative = 0x80)
#define ClearNegative() (ICPU._Negative = 0)

#define CheckZero() (ICPU._Zero == 0)
#define CheckCarry() (ICPU._Carry)
#define CheckIRQ() (Registers.PL & IRQ)
#define CheckDecimal() (Registers.PL & Decimal)
#define CheckIndex() (Registers.PL & IndexFlag)
#define CheckMemory() (Registers.PL & MemoryFlag)
#define CheckOverflow() (ICPU._Overflow)
#define CheckNegative() (ICPU._Negative & 0x80)
#define CheckEmulation() (Registers.P.W & Emulation)

#define ClearFlags(f) (Registers.P.W &= ~(f))
#define SetFlags(f)   (Registers.P.W |=  (f))
#define CheckFlag(f)  (Registers.PL & (f))

typedef union
{
#ifdef LSB_FIRST
    struct { uint8 l,h; } B;
#else
    struct { uint8 h,l; } B;
#endif
    uint16 W;
} pair;

struct SRegisters{
    uint8  PB;
    uint8  DB;
    pair   P;
    pair   A;
    pair   D;
    pair   S;
    pair   X;
    pair   Y;
    uint16 PC;
};

EXTERN_C struct SRegisters Registers;

#endif

