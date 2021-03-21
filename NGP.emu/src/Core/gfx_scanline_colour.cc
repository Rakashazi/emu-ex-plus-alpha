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

	gfx_scanline_colour.c

//=========================================================================
//---------------------------------------------------------------------------

  History of changes:
  ===================

20 JUL 2002 - neopop_uk
=======================================
- Cleaned and tidied up for the source release

22 JUL 2002 - neopop_uk
=======================================
- Removed delayed settings retrieval, this is now done by 'interrupt.c'
- Removed hack for scanline 0!

24 JUL 2002 - neopop_uk
=======================================
- Added Negative/Positive colour switching

25 JUL 2002 - neopop_uk
=======================================
- Fixed a stupid bug in the dimensions of the
	right side of the hardware window.

01 AUG 2002 - neopop_uk
=======================================
- Forced the background colour to be on - always.

06 AUG 2002 - neopop_uk
=======================================
- Switched to 16-bit [0BGR] rendering, should be much faster.

15 AUG 2002 - neopop_uk
=======================================
- Changed parameter 4 of drawPattern from bool to uint16, for performance
and compatiblity reasons.
  
16 AUG 2002 - neopop_uk
=======================================
- Optimised things a little by removing some extraneous pointer work
	and shifting.

//---------------------------------------------------------------------------
*/

#include "neopop.h"
#include "mem.h"
#include "gfx.h"
#include <imagine/util/algorithm.h>
#include <algorithm>

//=============================================================================

static unsigned char mirrored[] = {
    0x00, 0x40, 0x80, 0xc0, 0x10, 0x50, 0x90, 0xd0,
    0x20, 0x60, 0xa0, 0xe0, 0x30, 0x70, 0xb0, 0xf0,
    0x04, 0x44, 0x84, 0xc4, 0x14, 0x54, 0x94, 0xd4,
    0x24, 0x64, 0xa4, 0xe4, 0x34, 0x74, 0xb4, 0xf4,
    0x08, 0x48, 0x88, 0xc8, 0x18, 0x58, 0x98, 0xd8,
    0x28, 0x68, 0xa8, 0xe8, 0x38, 0x78, 0xb8, 0xf8,
    0x0c, 0x4c, 0x8c, 0xcc, 0x1c, 0x5c, 0x9c, 0xdc,
    0x2c, 0x6c, 0xac, 0xec, 0x3c, 0x7c, 0xbc, 0xfc,
    0x01, 0x41, 0x81, 0xc1, 0x11, 0x51, 0x91, 0xd1,
    0x21, 0x61, 0xa1, 0xe1, 0x31, 0x71, 0xb1, 0xf1,
    0x05, 0x45, 0x85, 0xc5, 0x15, 0x55, 0x95, 0xd5,
    0x25, 0x65, 0xa5, 0xe5, 0x35, 0x75, 0xb5, 0xf5,
    0x09, 0x49, 0x89, 0xc9, 0x19, 0x59, 0x99, 0xd9,
    0x29, 0x69, 0xa9, 0xe9, 0x39, 0x79, 0xb9, 0xf9,
    0x0d, 0x4d, 0x8d, 0xcd, 0x1d, 0x5d, 0x9d, 0xdd,
    0x2d, 0x6d, 0xad, 0xed, 0x3d, 0x7d, 0xbd, 0xfd,
    0x02, 0x42, 0x82, 0xc2, 0x12, 0x52, 0x92, 0xd2,
    0x22, 0x62, 0xa2, 0xe2, 0x32, 0x72, 0xb2, 0xf2,
    0x06, 0x46, 0x86, 0xc6, 0x16, 0x56, 0x96, 0xd6,
    0x26, 0x66, 0xa6, 0xe6, 0x36, 0x76, 0xb6, 0xf6,
    0x0a, 0x4a, 0x8a, 0xca, 0x1a, 0x5a, 0x9a, 0xda,
    0x2a, 0x6a, 0xaa, 0xea, 0x3a, 0x7a, 0xba, 0xfa,
    0x0e, 0x4e, 0x8e, 0xce, 0x1e, 0x5e, 0x9e, 0xde,
    0x2e, 0x6e, 0xae, 0xee, 0x3e, 0x7e, 0xbe, 0xfe,
    0x03, 0x43, 0x83, 0xc3, 0x13, 0x53, 0x93, 0xd3,
    0x23, 0x63, 0xa3, 0xe3, 0x33, 0x73, 0xb3, 0xf3,
    0x07, 0x47, 0x87, 0xc7, 0x17, 0x57, 0x97, 0xd7,
    0x27, 0x67, 0xa7, 0xe7, 0x37, 0x77, 0xb7, 0xf7,
    0x0b, 0x4b, 0x8b, 0xcb, 0x1b, 0x5b, 0x9b, 0xdb,
    0x2b, 0x6b, 0xab, 0xeb, 0x3b, 0x7b, 0xbb, 0xfb,
    0x0f, 0x4f, 0x8f, 0xcf, 0x1f, 0x5f, 0x9f, 0xdf,
    0x2f, 0x6f, 0xaf, 0xef, 0x3f, 0x7f, 0xbf, 0xff
};

//=============================================================================

#define drawPattern drawPatternC
#define gfx_draw_scroll1 gfx_draw_scroll1C
#define gfx_draw_scroll2 gfx_draw_scroll2C


/*static uint16 makeColor(uint16 palCol)
{
	// make compatible with GL_UNSIGNED_SHORT_4_4_4_4
	return swapBits(palCol, 0, 8, 4) << 4;
}*/

static uint16 makeColor(uint16 palCol)
{
	// make compatible with GL_UNSIGNED_SHORT_5_6_5
	uint16 r = uint16(((palCol & 0xF00) >> 8) * (31./15.));
	uint16 g = uint16(((palCol & 0xF0) >> 4) * (63./15.)) << 5;
	uint16 b = uint16((palCol & 0xF) * (31./15.)) << 11;
	return r | g | b;
}

static uint16 colorConvMap[0x1000] = { 0 };

void gfx_buildColorConvMap()
{
	iterateTimes(0x1000, i)
	{
		colorConvMap[i] = makeColor(i);
	}
}

static void drawPattern(uint8 screenx, uint16 tile, uint8 tiley, uint16 mirror,
				 uint16* palette_ptr, uint8 pal, uint8 depth)
{
	using namespace IG;
	int index, x, left, right, highmark, xx;
	uint16 data16;

	x = screenx;
	if (x > 0xf8)
		x -= 256;
	if (x >= SCREEN_WIDTH)
		return;

	//Get the data for the "tiley'th" line of "tile".
	index = le16toh(*(uint16*)(ram + 0xA000 + (tile * 16) + (tiley * 2)));

	//Horizontal Flip
	if (mirror)
		index = mirrored[(index & 0xff00)>>8] | (mirrored[(index & 0xff)] << 8);

	palette_ptr += pal << 2;
	left = std::max(std::max(x, (int)winx), 0);
	right = x+7;

	highmark = std::min(winw+winx, SCREEN_WIDTH)-1;

	if (right > highmark) {
		index >>= (right - highmark)*2;
		right = highmark;
	}

	for (xx=right; xx>=left; --xx,index>>=2) {
		if (depth <= zbuffer[xx] || (index&3)==0) 
			continue;
		zbuffer[xx] = depth;

		//Get the colour of the pixel
		data16 = colorConvMap[le16toh(palette_ptr[index&3])];
		
		if (negative)
			cfb_scanline[xx] = ~data16;
		else
			cfb_scanline[xx] = data16;
	}
}

static void gfx_draw_scroll1(uint8 depth)
{
	uint8 tx, row, line;
	uint16 data16;

	line = scanline + scroll1y;
	row = line & 7;	//Which row?

	//Draw Foreground scroll plane (Scroll 1)
	for (tx = 0; tx < 32; tx++)
	{
		data16 = le16toh(*(uint16*)(ram + 0x9000 + ((tx + ((line >> 3) << 5)) << 1)));
		
		//Draw the line of the tile
		drawPattern((tx << 3) - scroll1x, data16 & 0x01FF, 
			(data16 & 0x4000) ? (7 - row) : row, data16 & 0x8000, (uint16*)(ram + 0x8280),
			(data16 & 0x1E00) >> 9, depth);
	}
}

static void gfx_draw_scroll2(uint8 depth)
{
	uint8 tx, row, line;
	uint16 data16;

	line = scanline + scroll2y;
	row = line & 7;	//Which row?

	//Draw Background scroll plane (Scroll 2)
	for (tx = 0; tx < 32; tx++)
	{
		data16 = le16toh(*(uint16*)(ram + 0x9800 + ((tx + ((line >> 3) << 5)) << 1)));
		
		//Draw the line of the tile
		drawPattern((tx << 3) - scroll2x, data16 & 0x01FF, 
			(data16 & 0x4000) ? (7 - row) : row, data16 & 0x8000, (uint16*)(ram + 0x8300),
			(data16 & 0x1E00) >> 9, depth);
	}
}

void gfx_draw_scanline_colour(void)
{
	using namespace IG;
	int16 lastSpriteX;
	int16 lastSpriteY;
	int spr, x;
	uint16 data16;

	//Get the current scanline
	scanline = ram[0x8009];
	cfb_scanline = cfb + (scanline * SCREEN_WIDTH);	//Calculate fast offset

	memset(cfb_scanline, 0, SCREEN_WIDTH * sizeof(uint16));
	memset(zbuffer, 0, SCREEN_WIDTH);

	//Window colour
	data16 = colorConvMap[le16toh(*(uint16*)(ram + 0x83F0 + (oowc << 1)))];
	if (negative) data16 = ~data16;

	//Top
	if (scanline < winy)
	{
		for (x = 0; x < SCREEN_WIDTH; x++)
			cfb_scanline[x] = data16;
	}
	else
	{
		//Middle
		if (scanline < winy + winh)
		{
			for (x = 0; x < std::min((int)winx, SCREEN_WIDTH); x++)
				cfb_scanline[x] = data16;
			
			for (x = std::min(winx + winw, SCREEN_WIDTH); x < SCREEN_WIDTH; x++)
				cfb_scanline[x] = data16;
		}
		else	//Bottom
		{
			for (x = 0; x < SCREEN_WIDTH; x++)
				cfb_scanline[x] = data16;
		}
	}

	//Ignore above and below the window's top and bottom
	if (scanline >= winy && scanline < winy + winh)
	{
		//Background colour Enabled?	HACK: 01 AUG 2002 - Always on!
	//	if ((bgc & 0xC0) == 0x80)
		{
			data16 = colorConvMap[le16toh(*(uint16*)(uint8*)(ram + 0x83E0 + ((bgc & 7) << 1)))];
		}
	//	else data16 = 0;

		if (negative) data16 = ~data16;
		
		//Draw background!
		for (x = winx; x < std::min(winx + winw, SCREEN_WIDTH); x++)
			cfb_scanline[x] = data16;

		//Swap Front/Back scroll planes?
		if (planeSwap)
		{
			gfx_draw_scroll1(ZDEPTH_BACKGROUND_SCROLL);		//Swap
			gfx_draw_scroll2(ZDEPTH_FOREGROUND_SCROLL);
		}
		else
		{
			gfx_draw_scroll2(ZDEPTH_BACKGROUND_SCROLL);		//Normal
			gfx_draw_scroll1(ZDEPTH_FOREGROUND_SCROLL);
		}

		//Draw Sprites
		//Last sprite position, (defaults to top-left, sure?)
		lastSpriteX = 0;
		lastSpriteY = 0;
		for (spr = 0; spr < 64; spr++)
		{
			uint8 priority, row;
			uint8 sx = ram[0x8800 + (spr * 4) + 2];	//X position
			uint8 sy = ram[0x8800 + (spr * 4) + 3];	//Y position
			int16 x = sx;
			int16 y = sy;
			uint16 data16;
			
			data16 = le16toh(*(uint16*)(ram + 0x8800 + (spr * 4)));
			priority = (data16 & 0x1800) >> 11;

			if (data16 & 0x0400) x = lastSpriteX + sx;	//Horizontal chain?
			if (data16 & 0x0200) y = lastSpriteY + sy;	//Vertical chain?

			//Store the position for chaining
			lastSpriteX = x;
			lastSpriteY = y;
			
			//Visible?
			if (priority == 0)	continue;

			//Scroll the sprite
			x += scrollsprx;
			y += scrollspry;

			//Off-screen?
			if (x > 248 && x < 256)	x = x - 256; else x &= 0xFF;
			if (y > 248 && y < 256)	y = y - 256; else y &= 0xFF;

			//In range?
			if (scanline >= y && scanline <= y + 7)
			{
				row = (scanline - y) & 7;	//Which row?
				drawPattern((uint8)x, data16 & 0x01FF,
					(data16 & 0x4000) ? 7 - row : row, data16 & 0x8000,
					(uint16*)(ram + 0x8200), ram[0x8C00 + spr] & 0xF, priority << 1);
			}
		}

		//==========
	}

}

#undef drawPattern
#undef gfx_draw_scroll1
#undef gfx_draw_scroll2

//=============================================================================
