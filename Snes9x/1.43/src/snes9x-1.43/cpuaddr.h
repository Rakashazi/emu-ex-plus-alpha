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

#ifndef _CPUADDR_H_
#define _CPUADDR_H_

EXTERN_C long OpAddress;

typedef enum {
    NONE = 0,
    READ = 1,
    WRITE = 2,
    MODIFY = 3,
    JUMP = 4
} AccessMode;

STATIC inline void Immediate8 (AccessMode a)
{
    OpAddress = ICPU.ShiftedPB + CPU.PC - CPU.PCBase;
    CPU.PC++;
}

STATIC inline void Immediate16 (AccessMode a)
{
    OpAddress = ICPU.ShiftedPB + CPU.PC - CPU.PCBase;
    CPU.PC += 2;
}

STATIC inline void Relative (AccessMode a)
{
	signed char Int8 = *CPU.PC++;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif    
    OpAddress = ((int) (CPU.PC - CPU.PCBase) + Int8) & 0xffff;
}

STATIC inline void RelativeLong (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = *(uint16 *) CPU.PC;
#else
    OpAddress = *CPU.PC + (*(CPU.PC + 1) << 8);
#endif
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2 + ONE_CYCLE;
#endif
    CPU.PC += 2;
    OpAddress += (CPU.PC - CPU.PCBase);
    OpAddress &= 0xffff;
}

STATIC inline void AbsoluteIndexedIndirect (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = (Registers.X.W + *(uint16 *) CPU.PC) & 0xffff;
#else
    OpAddress = (Registers.X.W + *CPU.PC + (*(CPU.PC + 1) << 8)) & 0xffff;
#endif
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    OpenBus = *(CPU.PC + 1);
    CPU.PC += 2;
    OpAddress = S9xGetWord (ICPU.ShiftedPB + OpAddress);
    if(a&READ) OpenBus = (uint8)(OpAddress>>8);
}

STATIC inline void AbsoluteIndirectLong (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = *(uint16 *) CPU.PC;
#else
    OpAddress = *CPU.PC + (*(CPU.PC + 1) << 8);
#endif

#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    OpenBus = *(CPU.PC + 1);
    CPU.PC += 2;
    if(a&READ) {
	OpAddress = S9xGetWord (OpAddress) | ((OpenBus=S9xGetByte (OpAddress + 2)) << 16);
    } else {
    OpAddress = S9xGetWord (OpAddress) | (S9xGetByte (OpAddress + 2) << 16);
    }
}

STATIC inline void AbsoluteIndirect (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = *(uint16 *) CPU.PC;
#else
    OpAddress = *CPU.PC + (*(CPU.PC + 1) << 8);
#endif

#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    OpenBus = *(CPU.PC + 1);
    CPU.PC += 2;
    OpAddress = S9xGetWord (OpAddress);
    if(a&READ) OpenBus = (uint8)(OpAddress>>8);
    OpAddress += ICPU.ShiftedPB;
}

STATIC inline void Absolute (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = *(uint16 *) CPU.PC + ICPU.ShiftedDB;
#else
    OpAddress = *CPU.PC + (*(CPU.PC + 1) << 8) + ICPU.ShiftedDB;
#endif
    if(a&READ) OpenBus = *(CPU.PC+1);
    CPU.PC += 2;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
}

STATIC inline void AbsoluteLong (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = (*(uint32 *) CPU.PC) & 0xffffff;
#else
    OpAddress = *CPU.PC + (*(CPU.PC + 1) << 8) + (*(CPU.PC + 2) << 16);
#endif
    if(a&READ) OpenBus = *(CPU.PC+2);
    CPU.PC += 3;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2 + CPU.MemSpeed;
#endif
}

STATIC inline void Direct(AccessMode a)
{
    if(a&READ) OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif
//    if (Registers.DL != 0) CPU.Cycles += ONE_CYCLE;
}

STATIC inline void DirectIndirectIndexed (AccessMode a)
{
    OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif

    OpAddress = S9xGetWord (OpAddress);
    if(a&READ) OpenBus = (uint8)(OpAddress>>8);
    OpAddress += ICPU.ShiftedDB + Registers.Y.W;

//    if (Registers.DL != 0) CPU.Cycles += ONE_CYCLE;
    // XXX: always add one if STA
    // XXX: else Add one cycle if crosses page boundary
}

STATIC inline void DirectIndirectIndexedLong (AccessMode a)
{
    OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif

    if(a&READ){
	OpAddress = S9xGetWord (OpAddress) + ((OpenBus = S9xGetByte (OpAddress + 2)) << 16) + Registers.Y.W;
    } else {
	OpAddress = S9xGetWord (OpAddress) + (S9xGetByte (OpAddress + 2) << 16) + Registers.Y.W;
    }
//    if (Registers.DL != 0) CPU.Cycles += ONE_CYCLE;
}

STATIC inline void DirectIndexedIndirect(AccessMode a)
{
    OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W + Registers.X.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif

    OpAddress = S9xGetWord (OpAddress);
    if(a&READ) OpenBus = (uint8)(OpAddress>>8);
    OpAddress += ICPU.ShiftedDB;

#ifndef SA1_OPCODES
//    if (Registers.DL != 0)
//	CPU.Cycles += TWO_CYCLES;
//    else
	CPU.Cycles += ONE_CYCLE;
#endif
}

STATIC inline void DirectIndexedX (AccessMode a)
{
	if(a&READ) OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W + Registers.X.W);
    OpAddress &= CheckEmulation() ? 0xff : 0xffff;

#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif

#ifndef SA1_OPCODES
//    if (Registers.DL != 0)
//	CPU.Cycles += TWO_CYCLES;
//    else
	CPU.Cycles += ONE_CYCLE;
#endif
}

STATIC inline void DirectIndexedY (AccessMode a)
{
	if(a&READ) OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W + Registers.Y.W);
    OpAddress &= CheckEmulation() ? 0xff : 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif

#ifndef SA1_OPCODES
//    if (Registers.DL != 0)
//	CPU.Cycles += TWO_CYCLES;
//    else
	CPU.Cycles += ONE_CYCLE;
#endif
}

STATIC inline void AbsoluteIndexedX (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = ICPU.ShiftedDB + *(uint16 *) CPU.PC + Registers.X.W;
#else
    OpAddress = ICPU.ShiftedDB + *CPU.PC + (*(CPU.PC + 1) << 8) +
		Registers.X.W;
#endif
    if(a&READ) OpenBus = *(CPU.PC+1);
    CPU.PC += 2;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    // XXX: always add one cycle for ROL, LSR, etc
    // XXX: else is cross page boundary add one cycle
}

STATIC inline void AbsoluteIndexedY (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = ICPU.ShiftedDB + *(uint16 *) CPU.PC + Registers.Y.W;
#else
    OpAddress = ICPU.ShiftedDB + *CPU.PC + (*(CPU.PC + 1) << 8) +
		Registers.Y.W;
#endif    
    if(a&READ) OpenBus = *(CPU.PC+1);
    CPU.PC += 2;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2;
#endif
    // XXX: always add cycle for STA
    // XXX: else is cross page boundary add one cycle
}

STATIC inline void AbsoluteLongIndexedX (AccessMode a)
{
#ifdef FAST_LSB_WORD_ACCESS
    OpAddress = (*(uint32 *) CPU.PC + Registers.X.W) & 0xffffff;
#else
    OpAddress = (*CPU.PC + (*(CPU.PC + 1) << 8) + (*(CPU.PC + 2) << 16) + Registers.X.W) & 0xffffff;
#endif
    if(a&READ) OpenBus = *(CPU.PC+2);
    CPU.PC += 3;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeedx2 + CPU.MemSpeed;
#endif
}

STATIC inline void DirectIndirect (AccessMode a)
{
    OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif
    OpAddress = S9xGetWord (OpAddress);
    if(a&READ) OpenBus = (uint8)(OpAddress>>8);
    OpAddress += ICPU.ShiftedDB;

//    if (Registers.DL != 0) CPU.Cycles += ONE_CYCLE;
}

STATIC inline void DirectIndirectLong (AccessMode a)
{
    OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.D.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
#endif
    if(a&READ){
	OpAddress = S9xGetWord (OpAddress) + ((OpenBus=S9xGetByte (OpAddress + 2)) << 16);
    } else {
	OpAddress = S9xGetWord (OpAddress) + (S9xGetByte (OpAddress + 2) << 16);
    }
//    if (Registers.DL != 0) CPU.Cycles += ONE_CYCLE;
}

STATIC inline void StackRelative (AccessMode a)
{
    if(a&READ) OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.S.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
    CPU.Cycles += ONE_CYCLE;
#endif
}

STATIC inline void StackRelativeIndirectIndexed (AccessMode a)
{
    OpenBus = *CPU.PC;
    OpAddress = (*CPU.PC++ + Registers.S.W) & 0xffff;
#ifndef SA1_OPCODES
    CPU.Cycles += CPU.MemSpeed;
    CPU.Cycles += TWO_CYCLES;
#endif
    OpAddress = S9xGetWord (OpAddress);
    if(a&READ) OpenBus = (uint8)(OpAddress>>8);
    OpAddress = (OpAddress + ICPU.ShiftedDB +
		 Registers.Y.W) & 0xffffff;
}
#endif

