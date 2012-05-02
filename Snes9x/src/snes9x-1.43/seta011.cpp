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
#include <stdio.h>
#include "seta.h"
#include "memmap.h"

ST011_Regs ST011;

// shougi playboard
uint8 board[9][9];

// debug
static int line = 0;

uint8 S9xGetST011(uint32 Address)
{
	uint8 t;
	uint16 address = (uint16) Address & 0xFFFF;

	// line counter
	line++;

	// status check
	if (address == 0x01)
	{
		t = 0xFF;
	}
	// read directly from s-ram
	else
	{
		t = Memory.SRAM[address];
	}
	
	// debug
//	if(address<0x150)
//		printf( "ST011 R: %06X %02X\n", Address, t);

	return t;
}

void S9xSetST011(uint32 Address, uint8 Byte)
{
	uint16 address = (uint16) Address & 0xFFFF;
	static bool reset = false;

	// debug
	line++;

	if(!reset)
	{
		// bootup values
		ST011.waiting4command = true;
		reset = true;
	}

	// debug
//	if(address<0x150)
//		printf( "ST011 W: %06X %02X\n", Address, Byte );

	Memory.SRAM[address]=Byte;

	// op commands/data goes through this address
	if(address==0x00)
	{
		// check for new commands
		if (ST011.waiting4command)
		{
			ST011.waiting4command = false;
			ST011.command = Byte;
			ST011.in_index = 0;
			ST011.out_index = 0;
			switch(ST011.command)
			{
			case 0x01: ST011.in_count = 12*10+8; break;
			case 0x02: ST011.in_count = 4; break;
			case 0x04: ST011.in_count = 0; break;
			case 0x05: ST011.in_count = 0; break;
			case 0x06: ST011.in_count = 0; break;
			case 0x07: ST011.in_count = 0; break;
			case 0x0E: ST011.in_count = 0; break;
			default: ST011.waiting4command=true; break;
			}
		}
		else
		{
			ST011.parameters [ST011.in_index] = Byte;
			ST011.in_index++;
		}
	}

	if (ST011.in_count==ST011.in_index)
	{
		// Actually execute the command
		ST011.waiting4command = true;
		ST011.out_index = 0;
		switch (ST011.command)
		{
		// unknown: download playboard
		case 0x01:
			{
				// 9x9 board data: top to bottom, left to right
				// Values represent piece types and ownership
				for( int lcv=0; lcv<9; lcv++ )
					memcpy( board[lcv], ST011.parameters+lcv*10, 9*1 );
			}
			break;

		// unknown
		case 0x02: break;

		// unknown
		case 0x04:
			{
				// outputs
				Memory.SRAM[0x12C] = 0x00;
				//Memory.SRAM[0x12D] = 0x00;
				Memory.SRAM[0x12E] = 0x00;
			}
			break;

		// unknown
		case 0x05: 
			{
				// outputs
				Memory.SRAM[0x12C] = 0x00;
				//Memory.SRAM[0x12D] = 0x00;
				Memory.SRAM[0x12E] = 0x00;
			}
			break;

		// unknown
		case 0x06: break;
		case 0x07: break;

		// unknown
		case 0x0E:
			{
				// outputs
				Memory.SRAM[0x12C] = 0x00;
				Memory.SRAM[0x12D] = 0x00;
			}
			break;
		}
	}
}

