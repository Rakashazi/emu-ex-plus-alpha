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

#ifndef _apumemory_h_
#define _apumemory_h_

START_EXTERN_C
extern uint8 W4;
extern uint8 APUROM[64];
END_EXTERN_C

INLINE uint8 S9xAPUGetByteZ (uint8 Address)
{
    if (Address >= 0xf0 && IAPU.DirectPage == IAPU.RAM)
    {
	if (Address >= 0xf4 && Address <= 0xf7)
	{
#ifdef SPC700_SHUTDOWN
	    IAPU.WaitAddress2 = IAPU.WaitAddress1;
	    IAPU.WaitAddress1 = IAPU.PC;
#endif	    
	    return (IAPU.RAM [Address]);
	}
	if (Address >= 0xfd)
	{
#ifdef SPC700_SHUTDOWN
	    IAPU.WaitAddress2 = IAPU.WaitAddress1;
	    IAPU.WaitAddress1 = IAPU.PC;
#endif	    
	    uint8 t = IAPU.RAM [Address];
	    IAPU.RAM [Address] = 0;
	    return (t);
	}
	else
	if (Address == 0xf3)
	    return (S9xGetAPUDSP ());

	return (IAPU.RAM [Address]);
    }
    else
	return (IAPU.DirectPage [Address]);
}

INLINE void S9xAPUSetByteZ (uint8 byte, uint8 Address)
{
    if (Address >= 0xf0 && IAPU.DirectPage == IAPU.RAM)
    {
	if (Address == 0xf3)
	    S9xSetAPUDSP (byte);
	else
	if (Address >= 0xf4 && Address <= 0xf7)
	    APU.OutPorts [Address - 0xf4] = byte;
	else
	if (Address == 0xf1)
	    S9xSetAPUControl (byte);
	else
	if (Address < 0xfd)
	{
	    IAPU.RAM [Address] = byte;
	    if (Address >= 0xfa)
	    {
		if (byte == 0)
		    APU.TimerTarget [Address - 0xfa] = 0x100;
		else
		    APU.TimerTarget [Address - 0xfa] = byte;
	    }
	}
    }
    else
	IAPU.DirectPage [Address] = byte;
}

INLINE uint8 S9xAPUGetByte (uint32 Address)
{
    Address &= 0xffff;
    
    if (Address <= 0xff && Address >= 0xf0)
    {
	if (Address >= 0xf4 && Address <= 0xf7)
	{
#ifdef SPC700_SHUTDOWN
	    IAPU.WaitAddress2 = IAPU.WaitAddress1;
	    IAPU.WaitAddress1 = IAPU.PC;
#endif	    
	    return (IAPU.RAM [Address]);
	}
	else
	if (Address == 0xf3)
	    return (S9xGetAPUDSP ());
	if (Address >= 0xfd)
	{
#ifdef SPC700_SHUTDOWN
	    IAPU.WaitAddress2 = IAPU.WaitAddress1;
	    IAPU.WaitAddress1 = IAPU.PC;
#endif
	    uint8 t = IAPU.RAM [Address];
	    IAPU.RAM [Address] = 0;
	    return (t);
	}
	return (IAPU.RAM [Address]);
    }
    else
	return (IAPU.RAM [Address]);
}

INLINE void S9xAPUSetByte (uint8 byte, uint32 Address)
{
    Address &= 0xffff;
    
    if (Address <= 0xff && Address >= 0xf0)
    {
	if (Address == 0xf3)
	    S9xSetAPUDSP (byte);
	else
	if (Address >= 0xf4 && Address <= 0xf7)
	    APU.OutPorts [Address - 0xf4] = byte;
	else
	if (Address == 0xf1)
	    S9xSetAPUControl (byte);
	else
	if (Address < 0xfd)
	{
	    IAPU.RAM [Address] = byte;
	    if (Address >= 0xfa)
	    {
		if (byte == 0)
		    APU.TimerTarget [Address - 0xfa] = 0x100;
		else
		    APU.TimerTarget [Address - 0xfa] = byte;
	    }
	}
    }
    else
    {
#if 0
if (Address >= 0x2500 && Address <= 0x2504)
printf ("%06d %04x <- %02x\n", ICPU.Scanline, Address, byte);
if (Address == 0x26c6)
{
    extern FILE *apu_trace;
    extern FILE *trace;
    APU.Flags |= TRACE_FLAG;
    CPU.Flags |= TRACE_FLAG;
    if (apu_trace == NULL)
	apu_trace = fopen ("aputrace.log", "wb");
    if (trace == NULL)
	trace = fopen ("trace.log", "wb");
    printf ("TRACING SWITCHED ON\n");
}
#endif
	if (Address < 0xffc0)
	    IAPU.RAM [Address] = byte;
	else
	{
	    APU.ExtraRAM [Address - 0xffc0] = byte;
	    if (!APU.ShowROM)
		IAPU.RAM [Address] = byte;
	}
    }
}
#endif

