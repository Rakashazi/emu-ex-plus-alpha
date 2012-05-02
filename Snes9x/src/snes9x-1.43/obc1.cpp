/****************************************************
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
#include "memmap.h"
#include "obc1.h"

static uint8 *OBC1_RAM = NULL;

int OBC1_Address;
int OBC1_BasePtr;
int OBC1_Shift;

extern "C"
{
uint8 GetOBC1 (uint16 Address)
{
	switch(Address) {
		case 0x7ff0:
			return OBC1_RAM[OBC1_BasePtr + (OBC1_Address << 2)];
		
		case 0x7ff1:
			return OBC1_RAM[OBC1_BasePtr + (OBC1_Address << 2) + 1];
		
		case 0x7ff2:
			return OBC1_RAM[OBC1_BasePtr + (OBC1_Address << 2) + 2];
		
		case 0x7ff3:
			return OBC1_RAM[OBC1_BasePtr + (OBC1_Address << 2) + 3];
		
		case 0x7ff4:
			return OBC1_RAM[OBC1_BasePtr + (OBC1_Address >> 2) + 0x200];	
	}

	return OBC1_RAM[Address & 0x1fff];
}

void SetOBC1 (uint8 Byte, uint16 Address)
{
	switch(Address) {
		case 0x7ff0:
		{
			OBC1_RAM[OBC1_BasePtr + (OBC1_Address << 2)] = Byte;
			break;
		}
		
		case 0x7ff1:
		{
			OBC1_RAM[OBC1_BasePtr + (OBC1_Address << 2) + 1] = Byte;
			break;
		}
		
		case 0x7ff2:
		{
			OBC1_RAM[OBC1_BasePtr + (OBC1_Address << 2) + 2] = Byte;
			break;
		}
		
		case 0x7ff3:
		{
			OBC1_RAM[OBC1_BasePtr + (OBC1_Address << 2) + 3] = Byte;
			break;
		}
		
		case 0x7ff4:
		{
			unsigned char Temp;

			Temp = OBC1_RAM[OBC1_BasePtr + (OBC1_Address >> 2) + 0x200];
			Temp = (Temp & ~(3 << OBC1_Shift)) | ((Byte & 3) << OBC1_Shift);	
			OBC1_RAM[OBC1_BasePtr + (OBC1_Address >> 2) + 0x200] = Temp;
			break;
		}
		
		case 0x7ff5:
		{
			if (Byte & 1) 
				OBC1_BasePtr = 0x1800;
			else
				OBC1_BasePtr = 0x1c00;

			break;
		}
		
		case 0x7ff6:
		{
			OBC1_Address = Byte & 0x7f;	
			OBC1_Shift = (Byte & 3) << 1;	
			break;
		}	
	}

	OBC1_RAM[Address & 0x1fff] = Byte;
}

uint8 *GetBasePointerOBC1(uint32 Address)
{
	return Memory.FillRAM;
}

uint8 *GetMemPointerOBC1(uint32 Address)
{
	return (Memory.FillRAM + (Address & 0xffff));
}

void ResetOBC1()
{
	OBC1_RAM = &Memory.FillRAM[0x6000];

	if (OBC1_RAM[0x1ff5] & 1) 
		OBC1_BasePtr = 0x1800;
	else
		OBC1_BasePtr = 0x1c00;

	OBC1_Address = OBC1_RAM[0x1ff6] & 0x7f;	
	OBC1_Shift = (OBC1_RAM[0x1ff6] & 3) << 1;
}

}
