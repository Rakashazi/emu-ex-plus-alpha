//---------------------------------------------------------------------------
// NEOPOP : Emulator as in Dreamland
//
// Copyright (c) 2001-2002 by neopop_uk
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version. See also the license.txt file for
//	additional informations.
//---------------------------------------------------------------------------

/*
//---------------------------------------------------------------------------
//=========================================================================

	gfx.c

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

24 JUL 2002 - neopop_uk
=======================================
- Added Negative/Positive switch - used in "Faselei!" in B&W mode.

16 AUG 2002 - neopop_uk
=======================================
- Removed data8, data16 and data32

//---------------------------------------------------------------------------
*/

#include "neopop.h"
#include "mem.h"
#include "gfx.h"

//=============================================================================

uint16 cfb[256*256] __attribute__ ((aligned (8))) {0};
uint8 zbuffer[256];

uint16* cfb_scanline;	//set = scanline * SCREEN_WIDTH
uint8 scanline;

uint8 winx = 0, winw = SCREEN_WIDTH;
uint8 winy = 0, winh = SCREEN_HEIGHT;
uint8 scroll1x = 0, scroll1y = 0;
uint8 scroll2x = 0, scroll2y = 0;
uint8 scrollsprx = 0, scrollspry = 0;
uint8 planeSwap = 0;
uint8 bgc = 0, oowc = 0;

uint8 negative;	//Negative / Positive switched?

//=============================================================================

void gfx_delayed_settings(void)
{
	//Window dimensions
	winx = ram[0x8002];
	winy = ram[0x8003];
	winw = ram[0x8004];
	winh = ram[0x8005];

	//Scroll Planes (Confirmed delayed)
	scroll1x = ram[0x8032];
	scroll1y = ram[0x8033];
	scroll2x = ram[0x8034];
	scroll2y = ram[0x8035];

	//Sprite offset (Confirmed delayed)
	scrollsprx = ram[0x8020];
	scrollspry = ram[0x8021];

	//Plane Priority (Confirmed delayed)
	planeSwap = ram[0x8030] & 0x80;

	//Background colour register (Confirmed delayed)
	bgc = ram[0x8118];

	//2D Control register (Confirmed delayed)
	oowc = ram[0x8012] & 7;
	negative = ram[0x8012] & 0x80;
}

//=============================================================================
