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
#include "seta.h"
#include "memmap.h"

ST018_Regs ST018;

static int line;	// line counter

extern "C"{
uint8 S9xGetST018(uint32 Address)
{
	uint8 t;
	uint16 address = (uint16) Address & 0xFFFF;

	line++;

	// these roles may be flipped
	// op output
	if (address == 0x3804)
	{
		if (ST018.out_count)
		{
			t = (uint8) ST018.output [ST018.out_index];
			ST018.out_index++;
			if (ST018.out_count==ST018.out_index)
				ST018.out_count=0;
		}
		else
			t = 0x81;
	}
	// status register
	else if (address == 0x3800)
		t = ST018.status;
	
	printf( "ST018 R: %06X %02X\n", Address, t);

	return t;
}

void S9xSetST018(uint8 Byte, uint32 Address)
{
	uint16 address = (uint16) Address&0xFFFF;
	static bool reset = false;

	printf( "ST018 W: %06X %02X\n", Address, Byte );

	line++;

	if (!reset)
	{
		// bootup values
		ST018.waiting4command = true;
		ST018.part_command = 0;
		reset = true;
	}

	Memory.SRAM[address]=Byte;

	// default status for now
	ST018.status = 0x00;

	// op data goes through this address
	if (address==0x3804)
	{
		// check for new commands: 3 bytes length
		if(ST018.waiting4command && ST018.part_command==2)
		{
			ST018.waiting4command = false;
			ST018.command <<= 8;
			ST018.command |= Byte;
			ST018.in_index = 0;
			ST018.out_index = 0;
			ST018.part_command = 0;	// 3-byte commands
			ST018.pass = 0;	// data streams into the chip
			switch(ST018.command & 0xFFFFFF)
			{
			case 0x0100: ST018.in_count = 0; break;
			case 0xFF00: ST018.in_count = 0; break;
			default: ST018.waiting4command = true; break;
			}
		}
		else if(ST018.waiting4command)
		{
			// 3-byte commands
			ST018.part_command++;
			ST018.command <<= 8;
			ST018.command |= Byte;
		}
	}
	// extra parameters
	else if (address==0x3802)
	{
		ST018.parameters[ST018.in_index] = Byte;
		ST018.in_index++;
	}

	if (ST018.in_count==ST018.in_index)
	{
		// Actually execute the command
		ST018.waiting4command = true;
		ST018.in_index = 0;
		ST018.out_index = 0;
		switch (ST018.command)
		{
		// hardware check?
		case 0x0100:
			ST018.waiting4command = false;
			ST018.pass++;
			if (ST018.pass==1)
			{
				ST018.in_count = 1;
				ST018.out_count = 2;

				// Overload's research
				ST018.output[0x00] = 0x81;
				ST018.output[0x01] = 0x81;
			}
			else
			{
				//ST018.in_count = 1;
				ST018.out_count = 3;

				// no reason to change this
				//ST018.output[0x00] = 0x81;
				//ST018.output[0x01] = 0x81;
				ST018.output[0x02] = 0x81;

				// done processing requests
				if (ST018.pass==3)
					ST018.waiting4command = true;
			}
			break;

		// unknown: feels like a security detection
		// format identical to 0x0100
		case 0xFF00:
			ST018.waiting4command = false;
			ST018.pass++;
			if (ST018.pass==1)
			{
				ST018.in_count = 1;
				ST018.out_count = 2;

				// Overload's research
				ST018.output[0x00] = 0x81;
				ST018.output[0x01] = 0x81;
			}
			else
			{
				static int a=0;

				//ST018.in_count = 1;
				ST018.out_count = 3;

				// no reason to change this
				//ST018.output[0x00] = 0x81;
				//ST018.output[0x01] = 0x81;
				ST018.output[0x02] = 0x81;

				// done processing requests
				if (ST018.pass==3)
					ST018.waiting4command = true;
			}
			break;
		}
	}
}
}

