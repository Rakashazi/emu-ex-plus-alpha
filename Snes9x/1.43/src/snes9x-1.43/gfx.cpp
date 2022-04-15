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

#include "snes9x.h"

#include "memmap.h"
#include "ppu.h"
#include "cpuexec.h"
#include "display.h"
#include "gfx.h"
#include "apu.h"
#include "cheats.h"
#include "screenshot.h"

#include <imagine/util/utility.h>

#define M7 19
#define M8 19

void output_png();
void ComputeClipWindows ();
static void S9xDisplayFrameRate ();
static void S9xDisplayString (const char *string);

static const uint8 BitShifts[8][4] =
{
    {2, 2, 2, 2},	// 0
    {4, 4, 2, 0},	// 1
    {4, 4, 0, 0},	// 2
    {8, 4, 0, 0},	// 3
    {8, 2, 0, 0},	// 4
    {4, 2, 0, 0},	// 5
    {4, 0, 0, 0},	// 6
    {8, 0, 0, 0}	// 7
};
static const uint8 TileShifts[8][4] =
{
    {4, 4, 4, 4},	// 0
    {5, 5, 4, 0},	// 1
    {5, 5, 0, 0},	// 2
    {6, 5, 0, 0},	// 3
    {6, 4, 0, 0},	// 4
    {5, 4, 0, 0},	// 5
    {5, 0, 0, 0},	// 6
    {6, 0, 0, 0}	// 7
};
static const uint8 PaletteShifts[8][4] =
{
    {2, 2, 2, 2},	// 0
    {4, 4, 2, 0},	// 1
    {4, 4, 0, 0},	// 2
    {0, 4, 0, 0},	// 3
    {0, 2, 0, 0},	// 4
    {4, 2, 0, 0},	// 5
    {4, 0, 0, 0},	// 6
    {0, 0, 0, 0}	// 7
};
static const uint8 PaletteMasks[8][4] =
{
    {7, 7, 7, 7},	// 0
    {7, 7, 7, 0},	// 1
    {7, 7, 0, 0},	// 2
    {0, 7, 0, 0},	// 3
    {0, 7, 0, 0},	// 4
    {7, 7, 0, 0},	// 5
    {7, 0, 0, 0},	// 6
    {0, 0, 0, 0}	// 7
};

static const uint8 Depths[8][4] =
{
    {TILE_2BIT, TILE_2BIT, TILE_2BIT, TILE_2BIT}, // 0
    {TILE_4BIT, TILE_4BIT, TILE_2BIT, 0},         // 1
    {TILE_4BIT, TILE_4BIT, 0, 0},                 // 2
    {TILE_8BIT, TILE_4BIT, 0, 0},                 // 3
    {TILE_8BIT, TILE_2BIT, 0, 0},                 // 4
    {TILE_4BIT, TILE_2BIT, 0, 0},                 // 5
    {TILE_4BIT, 0, 0, 0},                         // 6
    {0, 0, 0, 0}                                  // 7
};

static const uint8 BGSizes [2] = {
    8, 16
};

/*extern NormalTileRenderer DrawTilePtr;
extern ClippedTileRenderer DrawClippedTilePtr;
extern NormalTileRenderer DrawHiResTilePtr;
extern ClippedTileRenderer DrawHiResClippedTilePtr;
extern LargePixelRenderer DrawLargePixelPtr;*/

static NormalTileRenderer DrawTilePtr = NULL;
static ClippedTileRenderer DrawClippedTilePtr = NULL;
static NormalTileRenderer DrawHiResTilePtr = NULL;
static ClippedTileRenderer DrawHiResClippedTilePtr = NULL;
static LargePixelRenderer DrawLargePixelPtr = NULL;

extern struct SBG BG;

/*extern struct SLineData LineData[240];
extern struct SLineMatrixData LineMatrixData [240];*/

static struct SLineData LineData[240];
static struct SLineMatrixData LineMatrixData [240];

static uint8  Mode7Depths [2];

#define CLIP_10_BIT_SIGNED(a) \
	((a) & ((1 << 10) - 1)) + (((((a) & (1 << 13)) ^ (1 << 13)) - (1 << 13)) >> 3)

#define ON_MAIN(N) \
(GFX.r212c & (1 << (N)) && \
 !(PPU.BG_Forced & (1 << (N))))

#define SUB_OR_ADD(N) \
(GFX.r2131 & (1 << (N)))

#define ON_SUB(N) \
((GFX.r2130 & 0x30) != 0x30 && \
 (GFX.r2130 & 2) && \
 (GFX.r212d & (1 << N)) && \
 !(PPU.BG_Forced & (1 << (N))))

#define ANYTHING_ON_SUB \
((GFX.r2130 & 0x30) != 0x30 && \
 (GFX.r2130 & 2) && \
 (GFX.r212d & 0x1f))

#define ADD_OR_SUB_ON_ANYTHING \
(GFX.r2131 & 0x3f)

#define FIX_INTERLACE(SCREEN, DO_DEPTH, DEPTH) \
    if (IPPU.DoubleHeightPixels && ((PPU.BGMode != 5 && PPU.BGMode != 6) || !IPPU.Interlace)) \
        for (uint32 y = GFX.StartY; y <= GFX.EndY; y++) \
        { \
            memmove (SCREEN + (y * 2 + 1) * GFX.Pitch2, \
                     SCREEN + y * 2 * GFX.Pitch2, \
                     GFX.Pitch2); \
            if(DO_DEPTH){ \
                memmove (DEPTH + (y * 2 + 1) * (GFX.PPLx2>>1), \
                         DEPTH + y * GFX.PPL, \
                         GFX.PPLx2>>1); \
            } \
        }


#define BLACK BUILD_PIXEL(0,0,0)

void DrawTile (uint32 Tile, uint32 Offset, uint32 StartLine,
	       uint32 LineCount);
void DrawClippedTile (uint32 Tile, uint32 Offset,
		      uint32 StartPixel, uint32 Width,
		      uint32 StartLine, uint32 LineCount);
void DrawTilex2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		 uint32 LineCount);
void DrawClippedTilex2 (uint32 Tile, uint32 Offset,
			uint32 StartPixel, uint32 Width,
			uint32 StartLine, uint32 LineCount);
void DrawTilex2x2 (uint32 Tile, uint32 Offset, uint32 StartLine,
	       uint32 LineCount);
void DrawClippedTilex2x2 (uint32 Tile, uint32 Offset,
			  uint32 StartPixel, uint32 Width,
			  uint32 StartLine, uint32 LineCount);
void DrawLargePixel (uint32 Tile, uint32 Offset,
		     uint32 StartPixel, uint32 Pixels,
		     uint32 StartLine, uint32 LineCount);

void DrawTile16 (uint32 Tile, uint32 Offset, uint32 StartLine,
	         uint32 LineCount);
void DrawClippedTile16 (uint32 Tile, uint32 Offset,
		        uint32 StartPixel, uint32 Width,
		        uint32 StartLine, uint32 LineCount);
void DrawTile16x2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		   uint32 LineCount);
void DrawClippedTile16x2 (uint32 Tile, uint32 Offset,
			  uint32 StartPixel, uint32 Width,
			  uint32 StartLine, uint32 LineCount);
void DrawTile16x2x2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		     uint32 LineCount);
void DrawClippedTile16x2x2 (uint32 Tile, uint32 Offset,
			    uint32 StartPixel, uint32 Width,
			    uint32 StartLine, uint32 LineCount);
void DrawLargePixel16 (uint32 Tile, uint32 Offset,
		       uint32 StartPixel, uint32 Pixels,
		       uint32 StartLine, uint32 LineCount);

void DrawTile16Add (uint32 Tile, uint32 Offset, uint32 StartLine,
		    uint32 LineCount);

void DrawClippedTile16Add (uint32 Tile, uint32 Offset,
			   uint32 StartPixel, uint32 Width,
			   uint32 StartLine, uint32 LineCount);

void DrawTile16Add1_2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		       uint32 LineCount);

void DrawClippedTile16Add1_2 (uint32 Tile, uint32 Offset,
			      uint32 StartPixel, uint32 Width,
			      uint32 StartLine, uint32 LineCount);

void DrawTile16FixedAdd1_2 (uint32 Tile, uint32 Offset, uint32 StartLine,
			    uint32 LineCount);

void DrawClippedTile16FixedAdd1_2 (uint32 Tile, uint32 Offset,
				   uint32 StartPixel, uint32 Width,
				   uint32 StartLine, uint32 LineCount);

void DrawTile16Sub (uint32 Tile, uint32 Offset, uint32 StartLine,
		    uint32 LineCount);

void DrawClippedTile16Sub (uint32 Tile, uint32 Offset,
			   uint32 StartPixel, uint32 Width,
			   uint32 StartLine, uint32 LineCount);

void DrawTile16Sub1_2 (uint32 Tile, uint32 Offset, uint32 StartLine,
		       uint32 LineCount);

void DrawClippedTile16Sub1_2 (uint32 Tile, uint32 Offset,
			      uint32 StartPixel, uint32 Width,
			      uint32 StartLine, uint32 LineCount);

void DrawTile16FixedSub1_2 (uint32 Tile, uint32 Offset, uint32 StartLine,
			    uint32 LineCount);

void DrawClippedTile16FixedSub1_2 (uint32 Tile, uint32 Offset,
				   uint32 StartPixel, uint32 Width,
				   uint32 StartLine, uint32 LineCount);

void DrawLargePixel16Add (uint32 Tile, uint32 Offset,
			  uint32 StartPixel, uint32 Pixels,
			  uint32 StartLine, uint32 LineCount);

void DrawLargePixel16Add1_2 (uint32 Tile, uint32 Offset,
			     uint32 StartPixel, uint32 Pixels,
			     uint32 StartLine, uint32 LineCount);

void DrawLargePixel16Sub (uint32 Tile, uint32 Offset,
			  uint32 StartPixel, uint32 Pixels,
			  uint32 StartLine, uint32 LineCount);

void DrawLargePixel16Sub1_2 (uint32 Tile, uint32 Offset,
			     uint32 StartPixel, uint32 Pixels,
			     uint32 StartLine, uint32 LineCount);

bool8 S9xGraphicsInit ()
{
    register uint32 PixelOdd = 1;
    register uint32 PixelEven = 2;

#ifdef GFX_MULTI_FORMAT
    if (GFX.BuildPixel == NULL)
	S9xSetRenderPixelFormat (RGB565);
#endif

    for (uint8 bitshift = 0; bitshift < 4; bitshift++)
    {
	for (register int i = 0; i < 16; i++)
	{
	    register uint32 h = 0;
	    register uint32 l = 0;

#if defined(LSB_FIRST)
	    if (i & 8)
		h |= PixelOdd;
	    if (i & 4)
		h |= PixelOdd << 8;
	    if (i & 2)
		h |= PixelOdd << 16;
	    if (i & 1)
		h |= PixelOdd << 24;
	    if (i & 8)
		l |= PixelOdd;
	    if (i & 4)
		l |= PixelOdd << 8;
	    if (i & 2)
		l |= PixelOdd << 16;
	    if (i & 1)
		l |= PixelOdd << 24;
#else
	    if (i & 8)
		h |= (PixelOdd << 24);
	    if (i & 4)
		h |= (PixelOdd << 16);
	    if (i & 2)
		h |= (PixelOdd << 8);
	    if (i & 1)
		h |= PixelOdd;
	    if (i & 8)
		l |= (PixelOdd << 24);
	    if (i & 4)
		l |= (PixelOdd << 16);
	    if (i & 2)
		l |= (PixelOdd << 8);
	    if (i & 1)
		l |= PixelOdd;
#endif

	    odd_high[bitshift][i] = h;
	    odd_low[bitshift][i] = l;
	    h = l = 0;

#if defined(LSB_FIRST)
	    if (i & 8)
		h |= PixelEven;
	    if (i & 4)
		h |= PixelEven << 8;
	    if (i & 2)
		h |= PixelEven << 16;
	    if (i & 1)
		h |= PixelEven << 24;
	    if (i & 8)
		l |= PixelEven;
	    if (i & 4)
		l |= PixelEven << 8;
	    if (i & 2)
		l |= PixelEven << 16;
	    if (i & 1)
		l |= PixelEven << 24;
#else
	    if (i & 8)
		h |= (PixelEven << 24);
	    if (i & 4)
		h |= (PixelEven << 16);
	    if (i & 2)
		h |= (PixelEven << 8);
	    if (i & 1)
		h |= PixelEven;
	    if (i & 8)
		l |= (PixelEven << 24);
	    if (i & 4)
		l |= (PixelEven << 16);
	    if (i & 2)
		l |= (PixelEven << 8);
	    if (i & 1)
		l |= PixelEven;
#endif

	    even_high[bitshift][i] = h;
	    even_low[bitshift][i] = l;
	}
	PixelEven <<= 2;
	PixelOdd <<= 2;
    }

    GFX.RealPitch = GFX.Pitch2 = GFX.Pitch;
    GFX.ZPitch = GFX.Pitch;
    if (Settings.SixteenBit)
    	GFX.ZPitch >>= 1;
    GFX.Delta = (GFX.SubScreen - GFX.Screen) >> 1;
    GFX.DepthDelta = GFX.SubZBuffer - GFX.ZBuffer;
    //GFX.InfoStringTimeout = 0;
    //GFX.InfoString = NULL;

    PPU.BG_Forced = 0;
    IPPU.OBJChanged = TRUE;
    /*if (Settings.Transparency)
    	Settings.SixteenBit = TRUE;*/

    IPPU.DirectColourMapsNeedRebuild = TRUE;
    GFX.PixSize = 1;
    if (Settings.SixteenBit)
    {
	DrawTilePtr = DrawTile16;
	DrawClippedTilePtr = DrawClippedTile16;
	DrawLargePixelPtr = DrawLargePixel16;
	DrawHiResTilePtr= DrawTile16;
	DrawHiResClippedTilePtr = DrawClippedTile16;
	GFX.PPL = GFX.Pitch >> 1;
	GFX.PPLx2 = GFX.Pitch;
    }
    else
    {
	DrawTilePtr = DrawTile;
	DrawClippedTilePtr = DrawClippedTile;
	DrawLargePixelPtr = DrawLargePixel;
	DrawHiResTilePtr = DrawTile;
	DrawHiResClippedTilePtr = DrawClippedTile;
	GFX.PPL = GFX.Pitch;
	GFX.PPLx2 = GFX.Pitch * 2;
    }
    S9xFixColourBrightness ();

    if (Settings.SixteenBit)
    {
	if (!(GFX.X2 = (uint16 *) malloc (sizeof (uint16) * 0x10000)))
	    return (FALSE);

	if (!(GFX.ZERO_OR_X2 = (uint16 *) malloc (sizeof (uint16) * 0x10000)) ||
	    !(GFX.ZERO = (uint16 *) malloc (sizeof (uint16) * 0x10000)))
	{
	    if (GFX.ZERO_OR_X2)
	    {
		free ((char *) GFX.ZERO_OR_X2);
		GFX.ZERO_OR_X2 = NULL;
	    }
	    if (GFX.X2)
	    {
		free ((char *) GFX.X2);
		GFX.X2 = NULL;
	    }
	    return (FALSE);
	}
	uint32 r, g, b;

	// Build a lookup table that multiplies a packed RGB value by 2 with
	// saturation.
	for (r = 0; r <= MAX_RED; r++)
	{
	    uint32 r2 = r << 1;
	    if (r2 > MAX_RED)
		r2 = MAX_RED;
	    for (g = 0; g <= MAX_GREEN; g++)
	    {
		uint32 g2 = g << 1;
		if (g2 > MAX_GREEN)
		    g2 = MAX_GREEN;
		for (b = 0; b <= MAX_BLUE; b++)
		{
		    uint32 b2 = b << 1;
		    if (b2 > MAX_BLUE)
			b2 = MAX_BLUE;
		    GFX.X2 [BUILD_PIXEL2 (r, g, b)] = BUILD_PIXEL2 (r2, g2, b2);
		    GFX.X2 [BUILD_PIXEL2 (r, g, b) & ~ALPHA_BITS_MASK] = BUILD_PIXEL2 (r2, g2, b2);
		}
	    }
	}
	ZeroMemory (GFX.ZERO, 0x10000 * sizeof (uint16));
	ZeroMemory (GFX.ZERO_OR_X2, 0x10000 * sizeof (uint16));
	// Build a lookup table that if the top bit of the color value is zero
	// then the value is zero, otherwise multiply the value by 2. Used by
	// the color subtraction code.

#if defined(OLD_COLOUR_BLENDING)
	for (r = 0; r <= MAX_RED; r++)
	{
	    uint32 r2 = r;
	    if ((r2 & 0x10) == 0)
		r2 = 0;
	    else
		r2 = (r2 << 1) & MAX_RED;

	    for (g = 0; g <= MAX_GREEN; g++)
	    {
		uint32 g2 = g;
		if ((g2 & GREEN_HI_BIT) == 0)
		    g2 = 0;
		else
		    g2 = (g2 << 1) & MAX_GREEN;

		for (b = 0; b <= MAX_BLUE; b++)
		{
		    uint32 b2 = b;
		    if ((b2 & 0x10) == 0)
			b2 = 0;
		    else
			b2 = (b2 << 1) & MAX_BLUE;

		    GFX.ZERO_OR_X2 [BUILD_PIXEL2 (r, g, b)] = BUILD_PIXEL2 (r2, g2, b2);
		    GFX.ZERO_OR_X2 [BUILD_PIXEL2 (r, g, b) & ~ALPHA_BITS_MASK] = BUILD_PIXEL2 (r2, g2, b2);
		}
	    }
	}
#else
        for (r = 0; r <= MAX_RED; r++)
        {
            uint32 r2 = r;
            if ((r2 & 0x10) == 0)
                r2 = 0;
            else
                r2 = (r2 << 1) & MAX_RED;

            if (r2 == 0)
                r2 = 1;
            for (g = 0; g <= MAX_GREEN; g++)
            {
                uint32 g2 = g;
                if ((g2 & GREEN_HI_BIT) == 0)
                    g2 = 0;
                else
                    g2 = (g2 << 1) & MAX_GREEN;

                if (g2 == 0)
                    g2 = 1;
                for (b = 0; b <= MAX_BLUE; b++)
                {
                    uint32 b2 = b;
                    if ((b2 & 0x10) == 0)
                        b2 = 0;
                    else
                        b2 = (b2 << 1) & MAX_BLUE;

                    if (b2 == 0)
                        b2 = 1;
                    GFX.ZERO_OR_X2 [BUILD_PIXEL2 (r, g, b)] = BUILD_PIXEL2 (r2, g2, b2);
                    GFX.ZERO_OR_X2 [BUILD_PIXEL2 (r, g, b) & ~ALPHA_BITS_MASK] = BUILD_PIXEL2 (r2, g2, b2);
                }
            }
        }
#endif

	// Build a lookup table that if the top bit of the color value is zero
	// then the value is zero, otherwise its just the value.
	for (r = 0; r <= MAX_RED; r++)
	{
	    uint32 r2 = r;
	    if ((r2 & 0x10) == 0)
		r2 = 0;
	    else
		r2 &= ~0x10;

	    for (g = 0; g <= MAX_GREEN; g++)
	    {
		uint32 g2 = g;
		if ((g2 & GREEN_HI_BIT) == 0)
		    g2 = 0;
		else
		    g2 &= ~GREEN_HI_BIT;
		for (b = 0; b <= MAX_BLUE; b++)
		{
		    uint32 b2 = b;
		    if ((b2 & 0x10) == 0)
			b2 = 0;
		    else
			b2 &= ~0x10;

		    GFX.ZERO [BUILD_PIXEL2 (r, g, b)] = BUILD_PIXEL2 (r2, g2, b2);
		    GFX.ZERO [BUILD_PIXEL2 (r, g, b) & ~ALPHA_BITS_MASK] = BUILD_PIXEL2 (r2, g2, b2);
		}
	    }
	}
    }
    else
    {
	GFX.X2 = NULL;
	GFX.ZERO_OR_X2 = NULL;
	GFX.ZERO = NULL;
    }

    return (TRUE);
}

void S9xGraphicsDeinit (void)
{
    // Free any memory allocated in S9xGraphicsInit
    if (GFX.X2)
    {
	free ((char *) GFX.X2);
	GFX.X2 = NULL;
    }
    if (GFX.ZERO_OR_X2)
    {
	free ((char *) GFX.ZERO_OR_X2);
	GFX.ZERO_OR_X2 = NULL;
    }
    if (GFX.ZERO)
    {
	free ((char *) GFX.ZERO);
	GFX.ZERO = NULL;
    }
}

void S9xBuildDirectColourMaps ()
{
    for (uint32 p = 0; p < 8; p++)
    {
	for (uint32 c = 0; c < 256; c++)
	{
// XXX: Brightness
	    DirectColourMaps [p][c] = BUILD_PIXEL (((c & 7) << 2) | ((p & 1) << 1),
						   ((c & 0x38) >> 1) | (p & 2),
						   ((c & 0xc0) >> 3) | (p & 4));
	}
    }
    IPPU.DirectColourMapsNeedRebuild = FALSE;
}

void S9xStartScreenRefresh ()
{
    if (GFX.InfoStringTimeout > 0 && --GFX.InfoStringTimeout == 0)
	GFX.InfoString = NULL;

    if (IPPU.RenderThisFrame)
    {
	/*if (!S9xInitUpdate ())
	{
	    IPPU.RenderThisFrame = FALSE;
	    return;
	}*/
	IPPU.PreviousLine = IPPU.CurrentLine = 0;
	IPPU.MaxBrightness = PPU.Brightness;
	IPPU.LatchedBlanking = PPU.ForcedBlanking;
	if(PPU.BGMode == 5 || PPU.BGMode == 6)
		IPPU.Interlace = (Memory.FillRAM[0x2133] & 1);
	if (Settings.SupportHiRes && (PPU.BGMode == 5 || PPU.BGMode == 6 ||
				      IPPU.Interlace))
	{
		if (PPU.BGMode == 5 || PPU.BGMode == 6 || IPPU.Interlace)
		{
			IPPU.RenderedScreenWidth = 512;
			IPPU.DoubleWidthPixels = TRUE;
		}
		else
		{
			IPPU.RenderedScreenWidth = 256;
			IPPU.DoubleWidthPixels = FALSE;
		}

		if (IPPU.Interlace)
		{
			IPPU.RenderedScreenHeight = PPU.ScreenHeight << 1;
			IPPU.DoubleHeightPixels = TRUE;
			GFX.Pitch2 = GFX.RealPitch;
			GFX.Pitch = GFX.RealPitch * 2;
			if (Settings.SixteenBit)
				GFX.PPL = GFX.PPLx2 = GFX.RealPitch;
			else
				GFX.PPL = GFX.PPLx2 = GFX.RealPitch << 1;
		}
		else
		{
			IPPU.RenderedScreenHeight = PPU.ScreenHeight;
			GFX.Pitch2 = GFX.Pitch = GFX.RealPitch;
            IPPU.DoubleHeightPixels = FALSE;
			if (Settings.SixteenBit)
				GFX.PPL = GFX.Pitch >> 1;
			else
				GFX.PPL = GFX.Pitch;
			GFX.PPLx2 = GFX.PPL << 1;
		}
#if defined(USE_GLIDE) || defined(USE_OPENGL)
	    GFX.ZPitch = GFX.RealPitch;
	    if (Settings.SixteenBit)
		GFX.ZPitch >>= 1;
#endif
	}
	else
	{
	    IPPU.RenderedScreenWidth = 256;
	    IPPU.RenderedScreenHeight = PPU.ScreenHeight;
	    IPPU.DoubleWidthPixels = FALSE;
        IPPU.DoubleHeightPixels = FALSE;
#ifdef USE_GLIDE
	    if (Settings.GlideEnable)
	    {
		// Speed up hack for Glide: render low res. SNES images
		// into a handy 256x256 sized buffer that can be uploaded into
		// texture memory with one Glide call without having to copy it
		// into a second, suitably sized buffer first.
		GFX.Pitch2 = GFX.Pitch = 256 * sizeof (uint16);
		GFX.PPL = 256;
                GFX.PPLx2 = GFX.Pitch;
                GFX.ZPitch = 256;
	    }
	    else
#endif
#ifdef USE_OPENGL
            if (Settings.OpenGLEnable)
            {
                GFX.Pitch2 = GFX.Pitch = 256 * sizeof (uint16);
                GFX.PPL = 256;
                GFX.PPLx2 = GFX.Pitch;
                GFX.ZPitch = 256;
            }
            else
#endif
	    {
		GFX.Pitch2 = GFX.Pitch = GFX.RealPitch;
		GFX.PPL = GFX.PPLx2 >> 1;
		GFX.ZPitch = GFX.RealPitch;
		if (Settings.SixteenBit)
		    GFX.ZPitch >>= 1;
	    }
	}
	PPU.RecomputeClipWindows = TRUE;
	GFX.DepthDelta = GFX.SubZBuffer - GFX.ZBuffer;
	GFX.Delta = (GFX.SubScreen - GFX.Screen) >> 1;
    }
}

void RenderLine (uint8 C)
{
    if (IPPU.RenderThisFrame)
	{
		LineData[C].BG[0].VOffset = PPU.BG[0].VOffset + 1;
		LineData[C].BG[0].HOffset = PPU.BG[0].HOffset;
		LineData[C].BG[1].VOffset = PPU.BG[1].VOffset + 1;
		LineData[C].BG[1].HOffset = PPU.BG[1].HOffset;

		if (PPU.BGMode == 7)
		{
			struct SLineMatrixData *p = &LineMatrixData [C];
			p->MatrixA = PPU.MatrixA;
			p->MatrixB = PPU.MatrixB;
			p->MatrixC = PPU.MatrixC;
			p->MatrixD = PPU.MatrixD;
			p->CentreX = PPU.CentreX;
			p->CentreY = PPU.CentreY;
		}
		else
		{
			if (Settings.StarfoxHack && PPU.BG[2].VOffset == 0 &&
				PPU.BG[2].HOffset == 0xe000)
			{
				LineData[C].BG[2].VOffset = 0xe1;
				LineData[C].BG[2].HOffset = 0;
			}
			else
			{
				LineData[C].BG[2].VOffset = PPU.BG[2].VOffset + 1;
				LineData[C].BG[2].HOffset = PPU.BG[2].HOffset;
				LineData[C].BG[3].VOffset = PPU.BG[3].VOffset + 1;
				LineData[C].BG[3].HOffset = PPU.BG[3].HOffset;
			}
		}
		IPPU.CurrentLine = C + 1;
	} else {
		/* if we're not rendering this frame, we still need to update this */
		// XXX: Check ForceBlank? Or anything else?
		if(IPPU.OBJChanged) S9xSetupOBJ();
		PPU.RangeTimeOver |= GFX.OBJLines[C].RTOFlags;
	}
}

void S9xEndScreenRefresh ()
{
    IPPU.HDMAStarted = FALSE;
    if (IPPU.RenderThisFrame)
    {
	FLUSH_REDRAW ();
	#ifndef NO_COLOR_CHANGE_TRACKING
	if (IPPU.ColorsChanged)
	{
	    uint32 saved = PPU.CGDATA[0];
	    if (!Settings.SixteenBit)
	    {
		// Hack for Super Mario World - to get its sky blue
		// (It uses Fixed colour addition on the backdrop colour)
		if (!(Memory.FillRAM [0x2131] & 0x80) &&
		    (Memory.FillRAM[0x2131] & 0x20) &&
		    (PPU.FixedColourRed || PPU.FixedColourGreen ||
		     PPU.FixedColourBlue))
		{
		    PPU.CGDATA[0] = PPU.FixedColourRed |
				    (PPU.FixedColourGreen << 5) |
				    (PPU.FixedColourBlue << 10);
		}
	    }
	    IPPU.ColorsChanged = FALSE;
	    PPU.CGDATA[0] = saved;
	}
	#endif
        if (
#ifdef USE_GLIDE
            !Settings.GlideEnable &&
#endif
#ifdef USE_OPENGL
            !Settings.OpenGLEnable &&
#endif
            TRUE)
        {
            GFX.Pitch = GFX.Pitch2 = GFX.RealPitch;
            GFX.PPL = GFX.PPLx2 >> 1;
        }

//take screenshot here.
#if 0
        if(Settings.TakeScreenshot)
            S9xDoScreenshot(IPPU.RenderedScreenWidth, IPPU.RenderedScreenHeight);
#endif
	/*	if (Settings.DisplayFrameRate)
	    S9xDisplayFrameRate ();
	if (GFX.InfoString)
	    S9xDisplayString (GFX.InfoString);*/

	S9xDeinitUpdate (IPPU.RenderedScreenWidth, IPPU.RenderedScreenHeight,
			 Settings.SixteenBit);
    }
    S9xApplyCheats ();
#ifdef DEBUGGER
    if (CPU.Flags & FRAME_ADVANCE_FLAG)
    {
	if (ICPU.FrameAdvanceCount)
	{
	    ICPU.FrameAdvanceCount--;
	    IPPU.RenderThisFrame = TRUE;
	}
	else
	{
	    CPU.Flags &= ~FRAME_ADVANCE_FLAG;
	    CPU.Flags |= DEBUG_MODE_FLAG;
	}
    }
#endif
}

void S9xSetInfoString (const char *string)
{
    GFX.InfoString = string;
    GFX.InfoStringTimeout = 120;
}

inline void SelectTileRenderer (bool8 normal)
{
    if (normal)
    {
	DrawTilePtr = DrawTile16;
	DrawClippedTilePtr = DrawClippedTile16;
	DrawLargePixelPtr = DrawLargePixel16;
    }
    else
    {
	if (GFX.r2131 & 0x80)
	{
	    if (GFX.r2131 & 0x40)
	    {
		if (GFX.r2130 & 2)
		{
		    DrawTilePtr = DrawTile16Sub1_2;
		    DrawClippedTilePtr = DrawClippedTile16Sub1_2;
		}
		else
		{
		    // Fixed colour substraction
		    DrawTilePtr = DrawTile16FixedSub1_2;
		    DrawClippedTilePtr = DrawClippedTile16FixedSub1_2;
		}
		DrawLargePixelPtr = DrawLargePixel16Sub1_2;
	    }
	    else
	    {
		DrawTilePtr = DrawTile16Sub;
		DrawClippedTilePtr = DrawClippedTile16Sub;
		DrawLargePixelPtr = DrawLargePixel16Sub;
	    }
	}
	else
	{
	    if (GFX.r2131 & 0x40)
	    {
		if (GFX.r2130 & 2)
		{
		    DrawTilePtr = DrawTile16Add1_2;
		    DrawClippedTilePtr = DrawClippedTile16Add1_2;
		}
		else
		{
		    // Fixed colour addition
		    DrawTilePtr = DrawTile16FixedAdd1_2;
		    DrawClippedTilePtr = DrawClippedTile16FixedAdd1_2;
		}
		DrawLargePixelPtr = DrawLargePixel16Add1_2;
	    }
	    else
	    {
		DrawTilePtr = DrawTile16Add;
		DrawClippedTilePtr = DrawClippedTile16Add;
		DrawLargePixelPtr = DrawLargePixel16Add;
	    }
	}
    }
}

void S9xSetupOBJ ()
{
#ifdef MK_DEBUG_RTO
	if(Settings.BGLayering) fprintf(stderr, "Entering SetupOBJS()\n");
#endif
	int SmallWidth, SmallHeight;
	int LargeWidth, LargeHeight;

	switch (PPU.OBJSizeSelect)
	{
	  case 0:
		SmallWidth = SmallHeight = 8;
		LargeWidth = LargeHeight = 16;
		break;
	  case 1:
		SmallWidth = SmallHeight = 8;
		LargeWidth = LargeHeight = 32;
		break;
	  case 2:
		SmallWidth = SmallHeight = 8;
		LargeWidth = LargeHeight = 64;
		break;
	  case 3:
		SmallWidth = SmallHeight = 16;
		LargeWidth = LargeHeight = 32;
		break;
	  case 4:
		SmallWidth = SmallHeight = 16;
		LargeWidth = LargeHeight = 64;
		break;
	  default:
	  case 5:
		SmallWidth = SmallHeight = 32;
		LargeWidth = LargeHeight = 64;
		break;
	  case 6:
		SmallWidth = 16; SmallHeight = 32;
		LargeWidth = 32; LargeHeight = 64;
		break;
	  case 7:
		SmallWidth = 16; SmallHeight = 32;
		LargeWidth = LargeHeight = 32;
		break;
	}
	if(IPPU.InterlaceSprites)
	{
		SmallHeight>>=1; LargeHeight>>=1;
	}
#ifdef MK_DEBUG_RTO
	if(Settings.BGLayering) fprintf(stderr, "Sizes are %dx%d and %dx%d\n", SmallWidth, SmallHeight, LargeWidth, LargeHeight);
#endif

	/* OK, we have three cases here. Either there's no priority, priority is
	 * normal FirstSprite, or priority is FirstSprite+Y. The first two are
	 * easy, the last is somewhat more ... interesting. So we split them up. */

	int Height;
	uint8 S;

#ifdef MK_DEBUG_RTO
	if(Settings.BGLayering) fprintf(stderr, "Priority rotation=%d, OAMAddr=%d -> ", PPU.OAMPriorityRotation, PPU.OAMAddr*2 | (PPU.OAMFlip&1));
#endif
	if(!PPU.OAMPriorityRotation || !(PPU.OAMFlip&PPU.OAMAddr&1)){
#ifdef MK_DEBUG_RTO
		if(Settings.BGLayering) fprintf(stderr, "normal FirstSprite = %02x\n", PPU.FirstSprite);
#endif
		/* normal case */
		uint8 LineOBJ[SNES_HEIGHT_EXTENDED];
		memset(LineOBJ, 0, sizeof(LineOBJ));
		for(int i=0; i<SNES_HEIGHT_EXTENDED; i++){
			GFX.OBJLines[i].RTOFlags=0;
			GFX.OBJLines[i].Tiles=34;
			for(int j=0; j<32; j++){ GFX.OBJLines[i].OBJ[j].Sprite=-1; }
		}
		uint8 FirstSprite=PPU.FirstSprite;
		S=FirstSprite;
		do {
			if(PPU.OBJ[S].Size){
				GFX.OBJWidths[S]=LargeWidth; Height=LargeHeight;
			} else {
				GFX.OBJWidths[S]=SmallWidth; Height=SmallHeight;
			}
			int HPos=PPU.OBJ[S].HPos; if(HPos==-256) HPos=256;
			if(HPos>-GFX.OBJWidths[S] && HPos<=256)
			{
				if(HPos<0){
					GFX.OBJVisibleTiles[S]=(GFX.OBJWidths[S]+HPos+7)>>3;
				} else if(HPos+GFX.OBJWidths[S]>=257){
					GFX.OBJVisibleTiles[S]=(257-HPos+7)>>3;
				} else {
					GFX.OBJVisibleTiles[S]=GFX.OBJWidths[S]>>3;
				}
				for(uint8 line=0, Y=(uint8)(PPU.OBJ[S].VPos&0xff); line<Height; Y++, line++){
					if(Y>=SNES_HEIGHT_EXTENDED) continue;
					if(LineOBJ[Y]>=32){
						GFX.OBJLines[Y].RTOFlags|=0x40;
#ifdef MK_DEBUG_RTO
						if(Settings.BGLayering) fprintf(stderr, "%d: OBJ %02x ranged over\n", Y, S);
#endif
						continue;
					}
					GFX.OBJLines[Y].Tiles-=GFX.OBJVisibleTiles[S];
					if(GFX.OBJLines[Y].Tiles<0) GFX.OBJLines[Y].RTOFlags|=0x80;
					GFX.OBJLines[Y].OBJ[LineOBJ[Y]].Sprite=S;
					if(PPU.OBJ[S].VFlip){
						// Yes, Width not Height. It so happens that the
						// sprites with H=2*W flip as two WxW sprites.
						GFX.OBJLines[Y].OBJ[LineOBJ[Y]].Line=line^(GFX.OBJWidths[S]-1);
					} else {
						GFX.OBJLines[Y].OBJ[LineOBJ[Y]].Line=line;
					}
					LineOBJ[Y]++;
				}
			}
			S=(S+1)&0x7F;
		} while(S!=FirstSprite);

		for(int Y=1; Y<SNES_HEIGHT_EXTENDED; Y++){
			GFX.OBJLines[Y].RTOFlags |= GFX.OBJLines[Y-1].RTOFlags;
		}
	} else {
		/* evil FirstSprite+Y case */
#ifdef MK_DEBUG_RTO
		if(Settings.BGLayering) fprintf(stderr, "FirstSprite+Y\n");
#endif

		/* First, find out which sprites are on which lines */
		uint8 OBJOnLine[SNES_HEIGHT_EXTENDED][128];
		memset(OBJOnLine, 0, sizeof(OBJOnLine));

		for(S=0; S<128; S++){
			if(PPU.OBJ[S].Size){
				GFX.OBJWidths[S]=LargeWidth; Height=LargeHeight;
			} else {
				GFX.OBJWidths[S]=SmallWidth; Height=SmallHeight;
			}
			int HPos=PPU.OBJ[S].HPos; if(HPos==-256) HPos=256;
			if(HPos>-GFX.OBJWidths[S] && HPos<=256)
			{
				if(HPos<0){
					GFX.OBJVisibleTiles[S]=(GFX.OBJWidths[S]+HPos+7)>>3;
				} else if(HPos+GFX.OBJWidths[S]>=257){
					GFX.OBJVisibleTiles[S]=(257-HPos+7)>>3;
				} else {
					GFX.OBJVisibleTiles[S]=GFX.OBJWidths[S]>>3;
				}
				for(uint8 line=0, Y=(uint8)(PPU.OBJ[S].VPos&0xff); line<Height; Y++, line++){
					if(Y>=SNES_HEIGHT_EXTENDED) continue;
					if(PPU.OBJ[S].VFlip){
						// Yes, Width not Height. It so happens that the
						// sprites with H=2*W flip as two WxW sprites.
						OBJOnLine[Y][S]=(line^(GFX.OBJWidths[S]-1)) | 0x80;
					} else {
						OBJOnLine[Y][S]=line | 0x80;
					}
				}
			}
		}

		/* Now go through and pull out those OBJ that are actually visible. */
		int j;
		for(int Y=0; Y<SNES_HEIGHT_EXTENDED; Y++){
			GFX.OBJLines[Y].RTOFlags=Y?GFX.OBJLines[Y-1].RTOFlags:0;

			GFX.OBJLines[Y].Tiles=34;
			uint8 FirstSprite=(PPU.FirstSprite+Y)&0x7F;
			S=FirstSprite; j=0;
			do {
				if(OBJOnLine[Y][S]){
					if(j>=32){
						GFX.OBJLines[Y].RTOFlags|=0x40;
#ifdef MK_DEBUG_RTO
						if(Settings.BGLayering) fprintf(stderr, "%d: OBJ %02x ranged over\n", Y, S);
#endif
						break;
					}
					GFX.OBJLines[Y].Tiles-=GFX.OBJVisibleTiles[S];
					if(GFX.OBJLines[Y].Tiles<0) GFX.OBJLines[Y].RTOFlags|=0x80;
					GFX.OBJLines[Y].OBJ[j].Sprite=S;
					GFX.OBJLines[Y].OBJ[j++].Line=OBJOnLine[Y][S]&~0x80;
				}
				S=(S+1)&0x7F;
			} while(S!=FirstSprite);
			if(j<32) GFX.OBJLines[Y].OBJ[j].Sprite=-1;
		}
	}

#ifdef MK_DEBUG_RTO
	if(Settings.BGLayering) {
		fprintf(stderr, "Sprites per line:\n");
		for(int xxx=0; xxx<SNES_HEIGHT_EXTENDED; xxx++){
			fprintf(stderr, "Line %d: RTO=%02x Tiles=%d", xxx, GFX.OBJLines[xxx].RTOFlags, 34-GFX.OBJLines[xxx].Tiles);
			for(int j=0; j<32 && GFX.OBJLines[xxx].OBJ[j].Sprite>=0; j++){
				fprintf(stderr, " %02x.%d", GFX.OBJLines[xxx].OBJ[j].Sprite, GFX.OBJLines[xxx].OBJ[j].Line);
			}
			fprintf(stderr, "\n");
		}

		fprintf(stderr, "Exiting SetupObj()\n");
	}
#endif

	IPPU.OBJChanged = FALSE;
}

void DrawOBJS (bool8 OnMain = FALSE, uint8 D = 0)
{
#ifdef MK_DEBUG_RTO
	if(Settings.BGLayering) fprintf(stderr, "Entering DrawOBJS() for %d-%d\n", GFX.StartY, GFX.EndY);
#endif
	CHECK_SOUND();

	BG.BitShift = 4;
	BG.TileShift = 5;
	BG.TileAddress = PPU.OBJNameBase;
	BG.StartPalette = 128;
	BG.PaletteShift = 4;
	BG.PaletteMask = 7;
	BG.Buffer = IPPU.TileCache [TILE_4BIT];
	BG.Buffered = IPPU.TileCached [TILE_4BIT];
	BG.NameSelect = PPU.OBJNameSelect;
	BG.DirectColourMode = FALSE;

	GFX.PixSize = 1;

	struct {
		uint16 Pos;
		bool8 Value;
	} Windows[7];
	int clipcount = GFX.pCurrentClip->Count [4];
	if (!clipcount){
		Windows[0].Pos=0;
		Windows[0].Value=TRUE;
		Windows[1].Pos=256;
		Windows[1].Value=FALSE;
		Windows[2].Pos=1000;
		Windows[2].Value=FALSE;
	} else {
		Windows[0].Pos=1000;
		Windows[0].Value=FALSE;
		for(int clip=0, i=1; clip<clipcount; clip++){
			if(GFX.pCurrentClip->Right[clip][4]<=GFX.pCurrentClip->Left[clip][4]) continue;
			int j;
			for(j=0; j<i && Windows[j].Pos<GFX.pCurrentClip->Left[clip][4]; j++);
			if(j<i && Windows[j].Pos==GFX.pCurrentClip->Left[clip][4]){
				Windows[j].Value = TRUE;
			} else {
				if(j<i) memmove(&Windows[j+1], &Windows[j], sizeof(Windows[0])*(i-j));
				Windows[j].Pos = GFX.pCurrentClip->Left[clip][4];
				Windows[j].Value = TRUE;
				i++;
			}
			for(j=0; j<i && Windows[j].Pos<GFX.pCurrentClip->Right[clip][4]; j++);
			if(j>=i || Windows[j].Pos!=GFX.pCurrentClip->Right[clip][4]){
				if(j<i) memmove(&Windows[j+1], &Windows[j], sizeof(Windows[0])*(i-j));
				Windows[j].Pos = GFX.pCurrentClip->Right[clip][4];
				Windows[j].Value = FALSE;
				i++;
			}
		}
	}

#ifdef MK_DEBUG_RTO
if(Settings.BGLayering) {
	fprintf(stderr, "Windows:\n");
	for(int xxx=0; xxx<6; xxx++){ fprintf(stderr, "%d: %d = %d\n", xxx, Windows[xxx].Pos, Windows[xxx].Value); }
}
#endif

	if (Settings.SupportHiRes)
	{
		if (PPU.BGMode == 5 || PPU.BGMode == 6)
		{
			// Bah, OnMain is never used except to determine if calling
			// SelectTileRenderer is necessary. So let's hack it to false here
			// to stop SelectTileRenderer from being called when it causes
			// problems.
			OnMain = FALSE;
			GFX.PixSize = 2;
			if (IPPU.DoubleHeightPixels)

			{
				if (Settings.SixteenBit)
				{
					DrawTilePtr = DrawTile16x2x2;
					DrawClippedTilePtr = DrawClippedTile16x2x2;
				}
				else
				{
					DrawTilePtr = DrawTilex2x2;
					DrawClippedTilePtr = DrawClippedTilex2x2;
				}
			}
			else
			{
				if (Settings.SixteenBit)
				{
					DrawTilePtr = DrawTile16x2;
					DrawClippedTilePtr = DrawClippedTile16x2;
				}
				else
				{
					DrawTilePtr = DrawTilex2;
					DrawClippedTilePtr = DrawClippedTilex2;
				}
			}
		}
		else
		{
			if (Settings.SixteenBit)
			{
				DrawTilePtr = DrawTile16;
				DrawClippedTilePtr = DrawClippedTile16;
			}
			else
			{
				DrawTilePtr = DrawTile;
				DrawClippedTilePtr = DrawClippedTile;
			}
		}
	}
	GFX.Z1 = D + 2;

	for(uint32 Y=GFX.StartY, Offset=Y*GFX.PPL; Y<=GFX.EndY; Y++, Offset+=GFX.PPL){
#ifdef MK_DEBUG_RTO
		bool8 Flag=0;
#endif
		int I = 0;
#ifdef MK_DISABLE_TIME_OVER
		int tiles=0;
#else
		int tiles=GFX.OBJLines[Y].Tiles;
#endif
		for (int S = GFX.OBJLines[Y].OBJ[I].Sprite; S >= 0 && I<32; S = GFX.OBJLines[Y].OBJ[++I].Sprite)
		{
			tiles+=GFX.OBJVisibleTiles[S];
			if(tiles<=0){
#ifdef MK_DEBUG_RTO
if(Settings.BGLayering) {
				if(!Flag){ Flag=1; fprintf(stderr, "Line %d:", Y); }
				fprintf(stderr, " [%02x]", S);
}
#endif
				continue;
			}

#ifdef MK_DEBUG_RTO
if(Settings.BGLayering) {
			if(!Flag){ Flag=1; fprintf(stderr, "Line %d:", Y); }
			fprintf(stderr, " %02x", S);
}
#endif

			if (OnMain && SUB_OR_ADD(4))
			{
				SelectTileRenderer (!GFX.Pseudo && PPU.OBJ [S].Palette < 4);
			}

			int BaseTile = (((GFX.OBJLines[Y].OBJ[I].Line<<1) + (PPU.OBJ[S].Name&0xf0))&0xf0) | (PPU.OBJ[S].Name&0x100) | (PPU.OBJ[S].Palette << 10);
			int TileX = PPU.OBJ[S].Name&0x0f;
			int TileLine = (GFX.OBJLines[Y].OBJ[I].Line&7)*8;
			int TileInc = 1;

			if (PPU.OBJ[S].HFlip)
			{
				TileX = (TileX + (GFX.OBJWidths[S] >> 3) - 1) & 0x0f;
				BaseTile |= H_FLIP;
				TileInc = -1;
			}

			GFX.Z2 = (PPU.OBJ[S].Priority + 1) * 4 + D;

			bool8 WinStat=TRUE;
			int WinIdx=0, NextPos=-1000;
			int X=PPU.OBJ[S].HPos; if(X==-256) X=256;
			for(int t=tiles, O=Offset+X*GFX.PixSize; X<=256 && X<PPU.OBJ[S].HPos+GFX.OBJWidths[S]; TileX=(TileX+TileInc)&0x0f, X+=8, O+=8*GFX.PixSize){
#ifdef MK_DEBUG_RTO
if(Settings.BGLayering) {
				if(X<-7) continue;
				if((t-1)<0) fprintf(stderr, "-[%d]", 35-t);
				else fprintf(stderr, "-%d", 35-t);
}
#endif
				if(X<-7 || --t<0 || X==256) continue;
				if(X>=NextPos){
					for(; WinIdx<7 && Windows[WinIdx].Pos<=X; WinIdx++);
					if(WinIdx==0) WinStat=FALSE;
					else WinStat=Windows[WinIdx-1].Value;
					NextPos=(WinIdx<7)?Windows[WinIdx].Pos:1000;
				}

				if(X+8<NextPos){
					if(WinStat) (*DrawTilePtr) (BaseTile|TileX, O, TileLine, 1);
				} else {
					int x=X;
					while(x<X+8){
						if(WinStat) (*DrawClippedTilePtr) (BaseTile|TileX, O, x-X, NextPos-x, TileLine, 1);
						x=NextPos;
						for(; WinIdx<7 && Windows[WinIdx].Pos<=x; WinIdx++);
						if(WinIdx==0) WinStat=FALSE;
						else WinStat=Windows[WinIdx-1].Value;
						NextPos=(WinIdx<7)?Windows[WinIdx].Pos:1000;
						if(NextPos>X+8) NextPos=X+8;
					}
				}
			}
		}
#ifdef MK_DEBUG_RTO
		if(Settings.BGLayering) if(Flag) fprintf(stderr, "\n");
#endif
	}
#ifdef MK_DEBUG_RTO
	if(Settings.BGLayering) fprintf(stderr, "Exiting DrawOBJS() for %d-%d\n", GFX.StartY, GFX.EndY);
#endif
}

void DrawBackgroundMosaic (uint32 BGMode, uint32 bg, uint8 Z1, uint8 Z2)
{
    CHECK_SOUND();

    uint32 Tile;
    uint16 *SC0;
    uint16 *SC1;
    uint16 *SC2;
    uint16 *SC3;
    uint8 depths [2] = {Z1, Z2};
    
    if (BGMode == 0)
	BG.StartPalette = bg << 5;
    else
	BG.StartPalette = 0;

    SC0 = (uint16 *) &Memory.VRAM[PPU.BG[bg].SCBase << 1];

    if (PPU.BG[bg].SCSize & 1)
	SC1 = SC0 + 1024;
    else
	SC1 = SC0;

	if(((uint8*)SC1-Memory.VRAM)>=0x10000)
		SC1-=0x08000;


    if (PPU.BG[bg].SCSize & 2)
	SC2 = SC1 + 1024;
    else
	SC2 = SC0;

	if(((uint8*)SC2-Memory.VRAM)>=0x10000)
		SC2-=0x08000;


    if (PPU.BG[bg].SCSize & 1)
	SC3 = SC2 + 1024;
    else
	SC3 = SC2;

	if(((uint8*)SC3-Memory.VRAM)>=0x10000)
		SC3-=0x08000;

    uint32 Lines;
    uint32 OffsetMask;
    uint32 OffsetShift;

    if (BG.TileSize == 16)
    {
	OffsetMask = 0x3ff;
	OffsetShift = 4;
    }
    else
    {
	OffsetMask = 0x1ff;
	OffsetShift = 3;
    }

	int m5 = (BGMode == 5 || BGMode == 6) ? 1 : 0;

    for (uint32 Y = GFX.StartY; Y <= GFX.EndY; Y += Lines)
    {
	uint32 VOffset = LineData [Y].BG[bg].VOffset;
	uint32 HOffset = LineData [Y].BG[bg].HOffset;
	uint32 MosaicOffset = Y % PPU.Mosaic;

	for (Lines = 1; Lines < PPU.Mosaic - MosaicOffset; Lines++)
	    if ((VOffset != LineData [Y + Lines].BG[bg].VOffset) ||
		(HOffset != LineData [Y + Lines].BG[bg].HOffset))
		break;
	
	uint32 MosaicLine = VOffset + Y - MosaicOffset;

	if (Y + Lines > GFX.EndY)
	    Lines = GFX.EndY + 1 - Y;
	uint32 VirtAlign = (MosaicLine & 7) << 3;
	
	uint16 *b1;
	uint16 *b2;

	uint32 ScreenLine = MosaicLine >> OffsetShift;
	uint32 Rem16 = MosaicLine & 15;

	if (ScreenLine & 0x20)
	    b1 = SC2, b2 = SC3;
	else
	    b1 = SC0, b2 = SC1;

	b1 += (ScreenLine & 0x1f) << 5;
	b2 += (ScreenLine & 0x1f) << 5;
	uint16 *t;
	uint32 Left = 0;
	uint32 Right = 256 << m5;

	HOffset <<= m5;

	uint32 ClipCount = GFX.pCurrentClip->Count [bg];
	uint32 HPos = HOffset;
	uint32 PixWidth = (PPU.Mosaic << m5);


	if (!ClipCount)
	    ClipCount = 1;

	for (uint32 clip = 0; clip < ClipCount; clip++)
	{
	    if (GFX.pCurrentClip->Count [bg])
	    {
		Left = GFX.pCurrentClip->Left [clip][bg] << m5;
		Right = GFX.pCurrentClip->Right [clip][bg] << m5;

		uint32 r = Left % (PPU.Mosaic << m5);
		HPos = HOffset + Left;
		PixWidth = (PPU.Mosaic << m5) - r;
	    }
	    uint32 s = Y * GFX.PPL + Left * GFX.PixSize;
	    for (uint32 x = Left; x < Right; x += PixWidth, 
		 s += PixWidth * GFX.PixSize,
		 HPos += PixWidth, PixWidth = (PPU.Mosaic << m5))
	    {
		uint32 Quot = (HPos & OffsetMask) >> 3;

		if (x + PixWidth >= Right)
		    PixWidth = Right - x;

		if (BG.TileSize == 8 && !m5)
		{
		    if (Quot > 31)
			t = b2 + (Quot & 0x1f);
		    else
			t = b1 + Quot;
		}
		else
		{
		    if (Quot > 63)
			t = b2 + ((Quot >> 1) & 0x1f);
		    else
			t = b1 + (Quot >> 1);
		}

		Tile = READ_2BYTES (t);
		GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];

		// Draw tile...
		if (BG.TileSize != 8)
		{
		    if (Tile & H_FLIP)
		    {
			// Horizontal flip, but what about vertical flip ?
			if (Tile & V_FLIP)
			{
			    // Both horzontal & vertical flip
			    if (Rem16 < 8)
			    {
				(*DrawLargePixelPtr) (Tile + 17 - (Quot & 1), s,
						      HPos & 7, PixWidth,
						      VirtAlign, Lines);
			    }
			    else
			    {
				(*DrawLargePixelPtr) (Tile + 1 - (Quot & 1), s,
						      HPos & 7, PixWidth,
						      VirtAlign, Lines);
			    }
			}
			else
			{
			    // Horizontal flip only
			    if (Rem16 > 7)
			    {
				(*DrawLargePixelPtr) (Tile + 17 - (Quot & 1), s,
						      HPos & 7, PixWidth,
						      VirtAlign, Lines);
			    }
			    else
			    {
				(*DrawLargePixelPtr) (Tile + 1 - (Quot & 1), s,
						      HPos & 7, PixWidth,
						      VirtAlign, Lines);
			    }
			}
		    }
		    else
		    {
			// No horizontal flip, but is there a vertical flip ?
			if (Tile & V_FLIP)
			{
			    // Vertical flip only
			    if (Rem16 < 8)
			    {
				(*DrawLargePixelPtr) (Tile + 16 + (Quot & 1), s,
						      HPos & 7, PixWidth,
						      VirtAlign, Lines);
			    }
			    else
			    {
				(*DrawLargePixelPtr) (Tile + (Quot & 1), s,
						      HPos & 7, PixWidth,
						      VirtAlign, Lines);
			    }
			}
			else
			{
			    // Normal unflipped
			    if (Rem16 > 7)
			    {
				(*DrawLargePixelPtr) (Tile + 16 + (Quot & 1), s,
						      HPos & 7, PixWidth,
						      VirtAlign, Lines);
			    }
			    else
			    {
				(*DrawLargePixelPtr) (Tile + (Quot & 1), s,
						      HPos & 7, PixWidth,
						      VirtAlign, Lines);
			    }
			}
		    }
		}
		else
		    (*DrawLargePixelPtr) (Tile + (Quot & 1) * m5, s, HPos & 7, PixWidth,
					  VirtAlign, Lines);
	    }
	}
    }
}

void DrawBackgroundOffset (uint32 BGMode, uint32 bg, uint8 Z1, uint8 Z2)
{
    CHECK_SOUND();
	
    uint32 Tile;
    uint16 *SC0;
    uint16 *SC1;
    uint16 *SC2;
    uint16 *SC3;
    uint16 *BPS0;
    uint16 *BPS1;
    uint16 *BPS2;
    uint16 *BPS3;
    uint32 Width;
    int VOffsetOffset = BGMode == 4 ? 0 : 32;
    uint8 depths [2] = {Z1, Z2};
    
    BG.StartPalette = 0;
	
    BPS0 = (uint16 *) &Memory.VRAM[PPU.BG[2].SCBase << 1];
	
    if (PPU.BG[2].SCSize & 1)
		BPS1 = BPS0 + 1024;
    else
		BPS1 = BPS0;
	
    if (PPU.BG[2].SCSize & 2)
		BPS2 = BPS1 + 1024;
    else
		BPS2 = BPS0;
	
    if (PPU.BG[2].SCSize & 1)
		BPS3 = BPS2 + 1024;
    else
		BPS3 = BPS2;
    
    SC0 = (uint16 *) &Memory.VRAM[PPU.BG[bg].SCBase << 1];
	
    if (PPU.BG[bg].SCSize & 1)
		SC1 = SC0 + 1024;
    else
		SC1 = SC0;
	
	if(((uint8*)SC1-Memory.VRAM)>=0x10000)
		SC1-=0x08000;


    if (PPU.BG[bg].SCSize & 2)
		SC2 = SC1 + 1024;
    else
		SC2 = SC0;

	if(((uint8*)SC2-Memory.VRAM)>=0x10000)
		SC2-=0x08000;


    if (PPU.BG[bg].SCSize & 1)
		SC3 = SC2 + 1024;
    else
		SC3 = SC2;
	
	if(((uint8*)SC3-Memory.VRAM)>=0x10000)
		SC3-=0x08000;


    static const int Lines = 1;
    int OffsetMask;
    int OffsetShift;
    int OffsetEnableMask = 1 << (bg + 13);
	
    if (BG.TileSize == 16)
    {
		OffsetMask = 0x3ff;
		OffsetShift = 4;
    }
    else
    {
		OffsetMask = 0x1ff;
		OffsetShift = 3;
    }
	
    for (uint32 Y = GFX.StartY; Y <= GFX.EndY; Y++)
    {
		uint32 VOff = LineData [Y].BG[2].VOffset - 1;
//		uint32 VOff = LineData [Y].BG[2].VOffset;
		uint32 HOff = LineData [Y].BG[2].HOffset;

		int VirtAlign;
		int ScreenLine = VOff >> 3;
		int t1;
		int t2;
		uint16 *s0;
		uint16 *s1;
		uint16 *s2;
		
		if (ScreenLine & 0x20)
			s1 = BPS2, s2 = BPS3;
		else
			s1 = BPS0, s2 = BPS1;
		
		s1 += (ScreenLine & 0x1f) << 5;
		s2 += (ScreenLine & 0x1f) << 5;
		
		if(BGMode != 4)
		{
			if((ScreenLine & 0x1f) == 0x1f)
			{
				if(ScreenLine & 0x20)
					VOffsetOffset = BPS0 - BPS2 - 0x1f*32;
				else
					VOffsetOffset = BPS2 - BPS0 - 0x1f*32;
			}
			else
			{
				VOffsetOffset = 32;
			}
		}
		
		int clipcount = GFX.pCurrentClip->Count [bg];
		if (!clipcount)
			clipcount = 1;
		
		for (int clip = 0; clip < clipcount; clip++)
		{
			uint32 Left;
			uint32 Right;
			
			if (!GFX.pCurrentClip->Count [bg])
			{
				Left = 0;
				Right = 256;
			}
			else
			{
				Left = GFX.pCurrentClip->Left [clip][bg];
				Right = GFX.pCurrentClip->Right [clip][bg];
				
				if (Right <= Left)
					continue;
			}
			
			uint32 VOffset;
			uint32 HOffset;
			//added:
			uint32 LineHOffset=LineData [Y].BG[bg].HOffset;
			
			uint32 Offset;
			uint32 HPos;
			uint32 Quot;
			uint32 Count;
			uint16 *t;
			uint32 Quot2;
			uint32 VCellOffset;
			uint32 HCellOffset;
			uint16 *b1;
			uint16 *b2;
			uint32 TotalCount = 0;
			uint32 MaxCount = 8;
			
			uint32 s = Left * GFX.PixSize + Y * GFX.PPL;
			bool8 left_hand_edge = (Left == 0);
			Width = Right - Left;
			
			if (Left & 7)
				MaxCount = 8 - (Left & 7);
			
			while (Left < Right) 
			{
				if (left_hand_edge)
				{
					// The SNES offset-per-tile background mode has a
					// hardware limitation that the offsets cannot be set
					// for the tile at the left-hand edge of the screen.
					VOffset = LineData [Y].BG[bg].VOffset;

					//MKendora; use temp var to reduce memory accesses
					//HOffset = LineData [Y].BG[bg].HOffset;

					HOffset = LineHOffset;
					//End MK

					left_hand_edge = FALSE;
				}
				else

				{
					// All subsequent offset tile data is shifted left by one,
					// hence the - 1 below.

					Quot2 = ((HOff + Left - 1) & OffsetMask) >> 3;
					
					if (Quot2 > 31)
						s0 = s2 + (Quot2 & 0x1f);
					else
						s0 = s1 + Quot2;
					
					HCellOffset = READ_2BYTES (s0);
					
					if (BGMode == 4)
					{
						VOffset = LineData [Y].BG[bg].VOffset;
						
						//MKendora another mem access hack
						//HOffset = LineData [Y].BG[bg].HOffset;
						HOffset=LineHOffset;
						//end MK

						if ((HCellOffset & OffsetEnableMask))
						{
							if (HCellOffset & 0x8000)
								VOffset = HCellOffset + 1;
							else
								HOffset = HCellOffset;
						}
					}
					else
					{
						VCellOffset = READ_2BYTES (s0 + VOffsetOffset);
						if ((VCellOffset & OffsetEnableMask))
							VOffset = VCellOffset + 1;
						else
							VOffset = LineData [Y].BG[bg].VOffset;

						//MKendora Strike Gunner fix
						if ((HCellOffset & OffsetEnableMask))
						{
							//HOffset= HCellOffset;
							
							HOffset = (HCellOffset & ~7)|(LineHOffset&7);
							//HOffset |= LineData [Y].BG[bg].HOffset&7;
						}
						else
							HOffset=LineHOffset;
							//HOffset = LineData [Y].BG[bg].HOffset - 
							//Settings.StrikeGunnerOffsetHack;
						//HOffset &= (~7);
						//end MK
					}
				}
				VirtAlign = ((Y + VOffset) & 7) << 3;
				ScreenLine = (VOffset + Y) >> OffsetShift;
				
				if (((VOffset + Y) & 15) > 7)
				{
					t1 = 16;
					t2 = 0;
				}
				else
				{
					t1 = 0;
					t2 = 16;
				}
				
				if (ScreenLine & 0x20)
					b1 = SC2, b2 = SC3;
				else
					b1 = SC0, b2 = SC1;
				
				b1 += (ScreenLine & 0x1f) << 5;
				b2 += (ScreenLine & 0x1f) << 5;
				
				HPos = (HOffset + Left) & OffsetMask;
				
				Quot = HPos >> 3;
				
				if (BG.TileSize == 8)
				{
					if (Quot > 31)
						t = b2 + (Quot & 0x1f);
					else
						t = b1 + Quot;
				}
				else
				{
					if (Quot > 63)
						t = b2 + ((Quot >> 1) & 0x1f);
					else
						t = b1 + (Quot >> 1);
				}
				
				if (MaxCount + TotalCount > Width)
					MaxCount = Width - TotalCount;
				
				Offset = HPos & 7;
				
				//Count =1;
				Count = 8 - Offset;
				if (Count > MaxCount)
					Count = MaxCount;
				
				s -= Offset * GFX.PixSize;
				Tile = READ_2BYTES(t);
				GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];
				
				if (BG.TileSize == 8)
					(*DrawClippedTilePtr) (Tile, s, Offset, Count, VirtAlign, Lines);
				else
				{
					if (!(Tile & (V_FLIP | H_FLIP)))
					{
						// Normal, unflipped
						(*DrawClippedTilePtr) (Tile + t1 + (Quot & 1),
							s, Offset, Count, VirtAlign, Lines);
					}
					else
						if (Tile & H_FLIP)
						{
							if (Tile & V_FLIP)
							{
								// H & V flip
								(*DrawClippedTilePtr) (Tile + t2 + 1 - (Quot & 1),
									s, Offset, Count, VirtAlign, Lines);
							}
							else
							{
								// H flip only
								(*DrawClippedTilePtr) (Tile + t1 + 1 - (Quot & 1),
									s, Offset, Count, VirtAlign, Lines);
							}
						}
						else
						{
							// V flip only
							(*DrawClippedTilePtr) (Tile + t2 + (Quot & 1),
								s, Offset, Count, VirtAlign, Lines);
						}
				}
				
				Left += Count;
				TotalCount += Count;
				s += (Offset + Count) * GFX.PixSize;
				MaxCount = 8;
		}
	}
    }
}

void DrawBackgroundMode5 (uint32 /* BGMODE */, uint32 bg, uint8 Z1, uint8 Z2)
{
    CHECK_SOUND();

    if(IPPU.Interlace)
	{
		GFX.Pitch = GFX.RealPitch;
		GFX.PPL = GFX.PPLx2 >> 1;
	}
    GFX.PixSize = 1;
    uint8 depths [2] = {Z1, Z2};
	
    uint32 Tile;
    uint16 *SC0;
    uint16 *SC1;
    uint16 *SC2;
    uint16 *SC3;
    uint32 Width;
    
    BG.StartPalette = 0;
	
    SC0 = (uint16 *) &Memory.VRAM[PPU.BG[bg].SCBase << 1];
	
    if ((PPU.BG[bg].SCSize & 1))
		SC1 = SC0 + 1024;
    else
		SC1 = SC0;
	
	if((SC1-(unsigned short*)Memory.VRAM)>0x10000)
		SC1=(uint16*)&Memory.VRAM[(((uint8*)SC1)-Memory.VRAM)%0x10000];
	
    if ((PPU.BG[bg].SCSize & 2))
		SC2 = SC1 + 1024;
    else SC2 = SC0;
	
	if(((uint8*)SC2-Memory.VRAM)>=0x10000)
		SC2-=0x08000;

	
	
    if ((PPU.BG[bg].SCSize & 1))
		SC3 = SC2 + 1024;
    else
		SC3 = SC2;
    
	if(((uint8*)SC3-Memory.VRAM)>=0x10000)
		SC3-=0x08000;

	
	
    int Lines;
    int VOffsetMask;
    int VOffsetShift;
	
    if (BG.TileSize == 16)
    {
		VOffsetMask = 0x3ff;
		VOffsetShift = 4;
    }
    else
    {
		VOffsetMask = 0x1ff;
		VOffsetShift = 3;
    }
    int endy = IPPU.Interlace ? 1 + (GFX.EndY << 1) : GFX.EndY;
	
    for (int Y = IPPU.Interlace ? GFX.StartY << 1 : GFX.StartY; Y <= endy; Y += Lines)
    {
		int y = IPPU.Interlace ? (Y >> 1) : Y;
		uint32 VOffset = LineData [y].BG[bg].VOffset;
		uint32 HOffset = LineData [y].BG[bg].HOffset;
		int VirtAlign = (Y + VOffset) & 7;
		
		for (Lines = 1; Lines < 8 - VirtAlign; Lines++)
			if ((VOffset != LineData [y + Lines].BG[bg].VOffset) ||
				(HOffset != LineData [y + Lines].BG[bg].HOffset))
				break;
			
			HOffset <<= 1;
			if (Y + Lines > endy)
				Lines = endy + 1 - Y;
			VirtAlign <<= 3;
			
			int ScreenLine = (VOffset + Y) >> VOffsetShift;
			int t1;
			int t2;
			if (((VOffset + Y) & 15) > 7)
			{
				t1 = 16;
				t2 = 0;
			}
			else
			{
				t1 = 0;
				t2 = 16;
			}
			uint16 *b1;
			uint16 *b2;
			
			if (ScreenLine & 0x20)
				b1 = SC2, b2 = SC3;
			else
				b1 = SC0, b2 = SC1;
			
			b1 += (ScreenLine & 0x1f) << 5;
			b2 += (ScreenLine & 0x1f) << 5;
			
			int clipcount = GFX.pCurrentClip->Count [bg];
			if (!clipcount)
				clipcount = 1;
			for (int clip = 0; clip < clipcount; clip++)
			{
				int Left;
				int Right;
				
				if (!GFX.pCurrentClip->Count [bg])
				{
					Left = 0;
					Right = 512;
				}
				else
				{
					Left = GFX.pCurrentClip->Left [clip][bg] * 2;
					Right = GFX.pCurrentClip->Right [clip][bg] * 2;
					
					if (Right <= Left)
						continue;
				}
				
				uint32 s = Left * GFX.PixSize + Y * GFX.PPL;
				uint32 HPos = (HOffset + Left * GFX.PixSize) & 0x3ff;
				
				uint32 Quot = HPos >> 3;
				uint32 Count = 0;
				
				uint16 *t;
				if (Quot > 63)
					t = b2 + ((Quot >> 1) & 0x1f);
				else
					t = b1 + (Quot >> 1);
				
				Width = Right - Left;
				// Left hand edge clipped tile
				if (HPos & 7)
				{
					int Offset = (HPos & 7);
					Count = 8 - Offset;
					if (Count > Width)
						Count = Width;
					s -= Offset;
					Tile = READ_2BYTES (t);
					GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];
					
					if (BG.TileSize == 8)
					{
						if (!(Tile & H_FLIP))
						{
							// Normal, unflipped
							(*DrawHiResClippedTilePtr) (Tile + (Quot & 1),
								s, Offset, Count, VirtAlign, Lines);
						}
						else
						{
							// H flip
							(*DrawHiResClippedTilePtr) (Tile + 1 - (Quot & 1),
								s, Offset, Count, VirtAlign, Lines);
						}
					}
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							// Normal, unflipped
							(*DrawHiResClippedTilePtr) (Tile + t1 + (Quot & 1),
								s, Offset, Count, VirtAlign, Lines);
						}
						else
							if (Tile & H_FLIP)
							{
								if (Tile & V_FLIP)
								{
									// H & V flip
									(*DrawHiResClippedTilePtr) (Tile + t2 + 1 - (Quot & 1),
										s, Offset, Count, VirtAlign, Lines);
								}
								else
								{
									// H flip only
									(*DrawHiResClippedTilePtr) (Tile + t1 + 1 - (Quot & 1),
										s, Offset, Count, VirtAlign, Lines);
								}
							}
							else
							{
								// V flip only
								(*DrawHiResClippedTilePtr) (Tile + t2 + (Quot & 1),
									s, Offset, Count, VirtAlign, Lines);
							}
					}
					
					t += Quot & 1;
					if (Quot == 63)
						t = b2;
					else if (Quot == 127)
						t = b1;
					Quot++;
					s += 8;
				}
				
				// Middle, unclipped tiles
				Count = Width - Count;
				int Middle = Count >> 3;
				Count &= 7;
				for (int C = Middle; C > 0; s += 8, Quot++, C--)
				{
					Tile = READ_2BYTES(t);
					GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];
					if (BG.TileSize == 8)
					{
						if (!(Tile & H_FLIP))
						{
							// Normal, unflipped
							(*DrawHiResTilePtr) (Tile + (Quot & 1),
								s, VirtAlign, Lines);
						}
						else
						{
							// H flip
							(*DrawHiResTilePtr) (Tile + 1 - (Quot & 1),
								s, VirtAlign, Lines);
						}
					}
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							// Normal, unflipped
							(*DrawHiResTilePtr) (Tile + t1 + (Quot & 1),
								s, VirtAlign, Lines);
						}
						else
							if (Tile & H_FLIP)
							{
								if (Tile & V_FLIP)
								{
									// H & V flip
									(*DrawHiResTilePtr) (Tile + t2 + 1 - (Quot & 1),
										s, VirtAlign, Lines);
								}
								else
								{
									// H flip only
									(*DrawHiResTilePtr) (Tile + t1 + 1 - (Quot & 1),
										s, VirtAlign, Lines);
								}
							}
							else
							{
								// V flip only
								(*DrawHiResTilePtr) (Tile + t2 + (Quot & 1),
									s, VirtAlign, Lines);
							}
					}
					
					t += Quot & 1;
					if (Quot == 63)
						t = b2;
					else
						if (Quot == 127)
							t = b1;
				}
				
				// Right-hand edge clipped tiles
				if (Count)
				{
					Tile = READ_2BYTES(t);
					GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];
					if (BG.TileSize == 8)
					{
						if (!(Tile & H_FLIP))
						{
							// Normal, unflipped
							(*DrawHiResClippedTilePtr) (Tile + (Quot & 1),
								s, 0, Count, VirtAlign, Lines);
						}
						else
						{
							// H flip
							(*DrawHiResClippedTilePtr) (Tile + 1 - (Quot & 1),
								s, 0, Count, VirtAlign, Lines);
						}
					}
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							// Normal, unflipped
							(*DrawHiResClippedTilePtr) (Tile + t1 + (Quot & 1),
								s, 0, Count, VirtAlign, Lines);
						}
						else
							if (Tile & H_FLIP)
							{
								if (Tile & V_FLIP)
								{
									// H & V flip
									(*DrawHiResClippedTilePtr) (Tile + t2 + 1 - (Quot & 1),
										s, 0, Count, VirtAlign, Lines);
								}
								else
								{
									// H flip only
									(*DrawHiResClippedTilePtr) (Tile + t1 + 1 - (Quot & 1),
										s, 0, Count, VirtAlign, Lines);
								}
							}
							else
							{
								// V flip only
								(*DrawHiResClippedTilePtr) (Tile + t2 + (Quot & 1),
									s, 0, Count, VirtAlign, Lines);
							}
					}
				}
	}
    }
    GFX.Pitch = IPPU.DoubleHeightPixels ? GFX.RealPitch * 2 : GFX.RealPitch;
    GFX.PPL = IPPU.DoubleHeightPixels ? GFX.PPLx2 : (GFX.PPLx2 >> 1);

}

void DrawBackground (uint32 BGMode, uint32 bg, uint8 Z1, uint8 Z2)
{
    GFX.PixSize = 1;
	
    BG.TileSize = BGSizes [PPU.BG[bg].BGSize];
    BG.BitShift = BitShifts[BGMode][bg];
    BG.TileShift = TileShifts[BGMode][bg];
    BG.TileAddress = PPU.BG[bg].NameBase << 1;
    BG.NameSelect = 0;
    BG.Buffer = IPPU.TileCache [Depths [BGMode][bg]];
    BG.Buffered = IPPU.TileCached [Depths [BGMode][bg]];
    BG.PaletteShift = PaletteShifts[BGMode][bg];
    BG.PaletteMask = PaletteMasks[BGMode][bg];
    BG.DirectColourMode = (BGMode == 3 || BGMode == 4) && bg == 0 &&
		(GFX.r2130 & 1);
	
    if (PPU.BGMosaic [bg] && PPU.Mosaic > 1)
    {
		DrawBackgroundMosaic (BGMode, bg, Z1, Z2);
		return;
		
    }
    switch (BGMode)
    {
    case 2:
    case 4: // Used by Puzzle Bobble
        DrawBackgroundOffset (BGMode, bg, Z1, Z2);
		return;
		
    case 5:
    case 6: // XXX: is also offset per tile.
		if (Settings.SupportHiRes)
		{
			DrawBackgroundMode5 (BGMode, bg, Z1, Z2);
			return;
		}
		break;
    }
    CHECK_SOUND();
	
    uint32 Tile;
    uint16 *SC0;
    uint16 *SC1;
    uint16 *SC2;
    uint16 *SC3;
    uint32 Width;
    uint8 depths [2] = {Z1, Z2};
    
    if (BGMode == 0)
		BG.StartPalette = bg << 5;
    else BG.StartPalette = 0;
	
    SC0 = (uint16 *) &Memory.VRAM[PPU.BG[bg].SCBase << 1];
	
    if (PPU.BG[bg].SCSize & 1)
		SC1 = SC0 + 1024;
    else
		SC1 = SC0;
	
	if(SC1>=(unsigned short*)(Memory.VRAM+0x10000))
		SC1=(uint16*)&Memory.VRAM[((uint8*)SC1-&Memory.VRAM[0])%0x10000];
	
    if (PPU.BG[bg].SCSize & 2)
		SC2 = SC1 + 1024;
    else
		SC2 = SC0;
	
	if(((uint8*)SC2-Memory.VRAM)>=0x10000)
		SC2-=0x08000;
	
    if (PPU.BG[bg].SCSize & 1)
		SC3 = SC2 + 1024;
    else
		SC3 = SC2;
    
	if(((uint8*)SC3-Memory.VRAM)>=0x10000)
		SC3-=0x08000;

	
	
    int Lines;
    int OffsetMask;
    int OffsetShift;
	
    if (BG.TileSize == 16)
    {
		OffsetMask = 0x3ff;
		OffsetShift = 4;
    }
    else
    {
		OffsetMask = 0x1ff;
		OffsetShift = 3;
    }
	
    for (uint32 Y = GFX.StartY; Y <= GFX.EndY; Y += Lines)
    {
		uint32 VOffset = LineData [Y].BG[bg].VOffset;
		uint32 HOffset = LineData [Y].BG[bg].HOffset;
		int VirtAlign = (Y + VOffset) & 7;
		
		for (Lines = 1; Lines < 8 - VirtAlign; Lines++)
			if ((VOffset != LineData [Y + Lines].BG[bg].VOffset) ||
				(HOffset != LineData [Y + Lines].BG[bg].HOffset))
				break;
			
			if (Y + Lines > GFX.EndY)
				Lines = GFX.EndY + 1 - Y;
			
			VirtAlign <<= 3;
			
			uint32 ScreenLine = (VOffset + Y) >> OffsetShift;
			uint32 t1;
			uint32 t2;
			if (((VOffset + Y) & 15) > 7)
			{
				t1 = 16;
				t2 = 0;
			}
			else
			{
				t1 = 0;
				t2 = 16;
			}
			uint16 *b1;
			uint16 *b2;
			
			if (ScreenLine & 0x20)
				b1 = SC2, b2 = SC3;
			else
				b1 = SC0, b2 = SC1;
			
			b1 += (ScreenLine & 0x1f) << 5;
			b2 += (ScreenLine & 0x1f) << 5;
			
			int clipcount = GFX.pCurrentClip->Count [bg];
			if (!clipcount)
				clipcount = 1;
			for (int clip = 0; clip < clipcount; clip++)
			{
				uint32 Left;
				uint32 Right;
				
				if (!GFX.pCurrentClip->Count [bg])
				{
					Left = 0;
					Right = 256;
				}
				else
				{
					Left = GFX.pCurrentClip->Left [clip][bg];
					Right = GFX.pCurrentClip->Right [clip][bg];
					
					if (Right <= Left)
						continue;
				}
				
				uint32 s = Left * GFX.PixSize + Y * GFX.PPL;
				uint32 HPos = (HOffset + Left) & OffsetMask;
				
				uint32 Quot = HPos >> 3;
				uint32 Count = 0;
				
				uint16 *t;
				if (BG.TileSize == 8)
				{
					if (Quot > 31)
						t = b2 + (Quot & 0x1f);
					else
						t = b1 + Quot;
				}
				else
				{
					if (Quot > 63)
						t = b2 + ((Quot >> 1) & 0x1f);
					else
						t = b1 + (Quot >> 1);
				}
				
				Width = Right - Left;
				// Left hand edge clipped tile
				if (HPos & 7)
				{
					uint32 Offset = (HPos & 7);
					Count = 8 - Offset;
					if (Count > Width)
						Count = Width;
					s -= Offset * GFX.PixSize;
					Tile = READ_2BYTES(t);
					GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];
					
					if (BG.TileSize == 8)
					{
						(*DrawClippedTilePtr) (Tile, s, Offset, Count, VirtAlign,
							Lines);
					}
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							// Normal, unflipped
							(*DrawClippedTilePtr) (Tile + t1 + (Quot & 1),
								s, Offset, Count, VirtAlign, Lines);
						}
						else
							if (Tile & H_FLIP)
							{
								if (Tile & V_FLIP)
								{
									// H & V flip
									(*DrawClippedTilePtr) (Tile + t2 + 1 - (Quot & 1),
										s, Offset, Count, VirtAlign, Lines);
								}
								else
								{
									// H flip only
									(*DrawClippedTilePtr) (Tile + t1 + 1 - (Quot & 1),
										s, Offset, Count, VirtAlign, Lines);
								}
							}
							else
							{
								// V flip only
								(*DrawClippedTilePtr) (Tile + t2 + (Quot & 1), s, 
									Offset, Count, VirtAlign, Lines);
							}
					}
					
					if (BG.TileSize == 8)
					{
						t++;
						if (Quot == 31)
							t = b2;
						else if (Quot == 63)
							t = b1;
					}
					else
					{
						t += Quot & 1;
						if (Quot == 63)
							t = b2;
						else if (Quot == 127)
							t = b1;
					}
					Quot++;
					s += 8 * GFX.PixSize;
				}
				
				// Middle, unclipped tiles
				Count = Width - Count;
				int Middle = Count >> 3;
				Count &= 7;
				for (int C = Middle; C > 0; s += 8 * GFX.PixSize, Quot++, C--)
				{
					Tile = READ_2BYTES(t);
					GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];
					
					if (BG.TileSize != 8)
					{
						if (Tile & H_FLIP)
						{
							// Horizontal flip, but what about vertical flip ?
							if (Tile & V_FLIP)
							{
								// Both horzontal & vertical flip
								(*DrawTilePtr) (Tile + t2 + 1 - (Quot & 1), s, 
									VirtAlign, Lines);
							}
							else
							{
								// Horizontal flip only
								(*DrawTilePtr) (Tile + t1 + 1 - (Quot & 1), s, 
									VirtAlign, Lines);
							}
						}
						else
						{
							// No horizontal flip, but is there a vertical flip ?
							if (Tile & V_FLIP)
							{
								// Vertical flip only
								(*DrawTilePtr) (Tile + t2 + (Quot & 1), s,
									VirtAlign, Lines);
							}
							else
							{
								// Normal unflipped
								(*DrawTilePtr) (Tile + t1 + (Quot & 1), s,
									VirtAlign, Lines);
							}
						}
					}
					else
					{
						(*DrawTilePtr) (Tile, s, VirtAlign, Lines);
					}
					
					if (BG.TileSize == 8)
					{
						t++;
						if (Quot == 31)
							t = b2;
						else
							if (Quot == 63)
								t = b1;
					}
					else
					{
						t += Quot & 1;
						if (Quot == 63)
							t = b2;
						else
							if (Quot == 127)
								t = b1;
					}
				}
				// Right-hand edge clipped tiles
				if (Count)
				{
					Tile = READ_2BYTES(t);
					GFX.Z1 = GFX.Z2 = depths [(Tile & 0x2000) >> 13];
					
					if (BG.TileSize == 8)
						(*DrawClippedTilePtr) (Tile, s, 0, Count, VirtAlign, 
						Lines);
					else
					{
						if (!(Tile & (V_FLIP | H_FLIP)))
						{
							// Normal, unflipped
							(*DrawClippedTilePtr) (Tile + t1 + (Quot & 1), s, 0, 
								Count, VirtAlign, Lines);
						}
						else if (Tile & H_FLIP)
						{
							if (Tile & V_FLIP)
							{
								// H & V flip
								(*DrawClippedTilePtr) (Tile + t2 + 1 - (Quot & 1),
									s, 0, Count, VirtAlign, 
									Lines);
							}
							else
							{
								// H flip only
								(*DrawClippedTilePtr) (Tile + t1 + 1 - (Quot & 1),
									s, 0, Count, VirtAlign,
									Lines);
							}
						}
						else
						{
							// V flip only
							(*DrawClippedTilePtr) (Tile + t2 + (Quot & 1),
								s, 0, Count, VirtAlign, 
								Lines);
						}
					}
				}
		}
    }
}

#define RENDER_BACKGROUND_MODE7(TYPE,FUNC) \
    CHECK_SOUND(); \
\
    uint8 *VRAM1 = Memory.VRAM + 1; \
    if (GFX.r2130 & 1) \
    { \
	if (IPPU.DirectColourMapsNeedRebuild) \
	    S9xBuildDirectColourMaps (); \
	GFX.ScreenColors = DirectColourMaps [0]; \
    } \
    else \
	GFX.ScreenColors = IPPU.ScreenColors; \
\
    int aa, cc; \
    int dir; \
    int startx, endx; \
    uint32 Left = 0; \
    uint32 Right = 256; \
    uint32 ClipCount = GFX.pCurrentClip->Count [bg]; \
\
    if (!ClipCount) \
	ClipCount = 1; \
\
    Screen += GFX.StartY * GFX.Pitch; \
    uint8 *Depth = GFX.DB + GFX.StartY * GFX.PPL; \
    struct SLineMatrixData *l = &LineMatrixData [GFX.StartY]; \
\
    for (uint32 Line = GFX.StartY; Line <= GFX.EndY; Line++, Screen += GFX.Pitch, Depth += GFX.PPL, l++) \
    { \
	int yy; \
\
	int32 HOffset = ((int32) LineData [Line].BG[0].HOffset << M7) >> M7; \
	int32 VOffset = ((int32) LineData [Line].BG[0].VOffset << M7) >> M7; \
\
	int32 CentreX = ((int32) l->CentreX << M7) >> M7; \
	int32 CentreY = ((int32) l->CentreY << M7) >> M7; \
\
	if (PPU.Mode7VFlip) \
	    yy = 255 - (int) Line; \
	else \
	    yy = Line; \
\
    yy += CLIP_10_BIT_SIGNED(VOffset - CentreY); \
\
  int BB = l->MatrixB * yy + (CentreX << 8); \
	int DD = l->MatrixD * yy + (CentreY << 8); \
\
	for (uint32 clip = 0; clip < ClipCount; clip++) \
	{ \
	    if (GFX.pCurrentClip->Count [bg]) \
	    { \
		Left = GFX.pCurrentClip->Left [clip][bg]; \
		Right = GFX.pCurrentClip->Right [clip][bg]; \
		if (Right <= Left) \
		    continue; \
	    } \
	    TYPE *p = (TYPE *) Screen + Left; \
	    uint8 *d = Depth + Left; \
\
	    if (PPU.Mode7HFlip) \
	    { \
		startx = Right - 1; \
		endx = Left - 1; \
		dir = -1; \
		aa = -l->MatrixA; \
		cc = -l->MatrixC; \
	    } \
	    else \
	    { \
		startx = Left; \
		endx = Right; \
		dir = 1; \
		aa = l->MatrixA; \
		cc = l->MatrixC; \
	    } \
\
	int xx = startx + CLIP_10_BIT_SIGNED(HOffset - CentreX); \
	    int AA = l->MatrixA * xx; \
	    int CC = l->MatrixC * xx; \
\
	    if (!PPU.Mode7Repeat) \
	    { \
		for (int x = startx; x != endx; x += dir, AA += aa, CC += cc, p++, d++) \
		{ \
		    int X = ((AA + BB) >> 8) & 0x3ff; \
		    int Y = ((CC + DD) >> 8) & 0x3ff; \
		    uint8 *TileData = VRAM1 + (Memory.VRAM[((Y & ~7) << 5) + ((X >> 2) & ~1)] << 7); \
		    uint32 b = *(TileData + ((Y & 7) << 4) + ((X & 7) << 1)); \
		    GFX.Z1 = Mode7Depths [(b & GFX.Mode7PriorityMask) >> 7]; \
		    if (GFX.Z1 > *d && (b & GFX.Mode7Mask) ) \
		    { \
			*p = (FUNC); \
			*d = GFX.Z1; \
		    } \
		} \
	    } \
	    else \
	    { \
		for (int x = startx; x != endx; x += dir, AA += aa, CC += cc, p++, d++) \
		{ \
		    int X = ((AA + BB) >> 8); \
		    int Y = ((CC + DD) >> 8); \
\
		    if (((X | Y) & ~0x3ff) == 0) \
		    { \
			uint8 *TileData = VRAM1 + (Memory.VRAM[((Y & ~7) << 5) + ((X >> 2) & ~1)] << 7); \
			uint32 b = *(TileData + ((Y & 7) << 4) + ((X & 7) << 1)); \
			GFX.Z1 = Mode7Depths [(b & GFX.Mode7PriorityMask) >> 7]; \
			if (GFX.Z1 > *d && (b & GFX.Mode7Mask) ) \
			{ \
			    *p = (FUNC); \
			    *d = GFX.Z1; \
			} \
		    } \
		    else \
		    { \
			if (PPU.Mode7Repeat == 3) \
			{ \
			    X = (x + HOffset) & 7; \
			    Y = (yy + CentreY) & 7; \
			    uint32 b = *(VRAM1 + ((Y & 7) << 4) + ((X & 7) << 1)); \
			    GFX.Z1 = Mode7Depths [(b & GFX.Mode7PriorityMask) >> 7]; \
			    if (GFX.Z1 > *d && (b & GFX.Mode7Mask) ) \
			    { \
				*p = (FUNC); \
				*d = GFX.Z1; \
			    } \
			} \
		    } \
		} \
	    } \
	} \
    }

void DrawBGMode7Background (uint8 *Screen, int bg)
{
    RENDER_BACKGROUND_MODE7 (uint8, (uint8) (b & GFX.Mode7Mask))
}

void DrawBGMode7Background16 (uint8 *Screen, int bg)
{
    RENDER_BACKGROUND_MODE7 (uint16, GFX.ScreenColors [b & GFX.Mode7Mask]);
}

void DrawBGMode7Background16Add (uint8 *Screen, int bg)
{
    RENDER_BACKGROUND_MODE7 (uint16, *(d + GFX.DepthDelta) ?
					(*(d + GFX.DepthDelta) != 1 ?
					    COLOR_ADD (GFX.ScreenColors [b & GFX.Mode7Mask],
						       p [GFX.Delta]) :
					    COLOR_ADD (GFX.ScreenColors [b & GFX.Mode7Mask],
						       GFX.FixedColour)) :
					 GFX.ScreenColors [b & GFX.Mode7Mask]);
}

void DrawBGMode7Background16Add1_2 (uint8 *Screen, int bg)
{
    RENDER_BACKGROUND_MODE7 (uint16, *(d + GFX.DepthDelta) ?
					(*(d + GFX.DepthDelta) != 1 ?
					    COLOR_ADD1_2 (GFX.ScreenColors [b & GFX.Mode7Mask],
						       p [GFX.Delta]) :
					    COLOR_ADD (GFX.ScreenColors [b & GFX.Mode7Mask],
						       GFX.FixedColour)) :
					 GFX.ScreenColors [b & GFX.Mode7Mask]);
}

void DrawBGMode7Background16Sub (uint8 *Screen, int bg)
{
    RENDER_BACKGROUND_MODE7 (uint16, *(d + GFX.DepthDelta) ?
					(*(d + GFX.DepthDelta) != 1 ?
					    COLOR_SUB (GFX.ScreenColors [b & GFX.Mode7Mask],
						       p [GFX.Delta]) :
					    COLOR_SUB (GFX.ScreenColors [b & GFX.Mode7Mask],
						       GFX.FixedColour)) :
					 GFX.ScreenColors [b & GFX.Mode7Mask]);
}

void DrawBGMode7Background16Sub1_2 (uint8 *Screen, int bg)
{
    RENDER_BACKGROUND_MODE7 (uint16, *(d + GFX.DepthDelta) ?
					(*(d + GFX.DepthDelta) != 1 ?
					    COLOR_SUB1_2 (GFX.ScreenColors [b & GFX.Mode7Mask],
						       p [GFX.Delta]) :
					    COLOR_SUB (GFX.ScreenColors [b & GFX.Mode7Mask],
						       GFX.FixedColour)) :
					 GFX.ScreenColors [b & GFX.Mode7Mask]);
}

#define RENDER_BACKGROUND_MODE7_i(TYPE,FUNC,COLORFUNC) \
    CHECK_SOUND(); \
\
    uint8 *VRAM1 = Memory.VRAM + 1; \
    if (GFX.r2130 & 1) \
    { \
        if (IPPU.DirectColourMapsNeedRebuild) \
            S9xBuildDirectColourMaps (); \
        GFX.ScreenColors = DirectColourMaps [0]; \
    } \
    else \
        GFX.ScreenColors = IPPU.ScreenColors; \
    \
    int aa, cc; \
    int dir; \
    int startx, endx; \
    uint32 Left = 0; \
    uint32 Right = 256; \
    uint32 ClipCount = GFX.pCurrentClip->Count [bg]; \
    \
    if (!ClipCount) \
        ClipCount = 1; \
    \
    Screen += GFX.StartY * GFX.Pitch; \
    uint8 *Depth = GFX.DB + GFX.StartY * GFX.PPL; \
    struct SLineMatrixData *l = &LineMatrixData [GFX.StartY]; \
    bool8 allowSimpleCase = FALSE; \
    if (!l->MatrixB && !l->MatrixC && (l->MatrixA == 0x0100) && (l->MatrixD == 0x0100) \
        && !LineMatrixData[GFX.EndY].MatrixB && !LineMatrixData[GFX.EndY].MatrixC \
        && (LineMatrixData[GFX.EndY].MatrixA == 0x0100) && (LineMatrixData[GFX.EndY].MatrixD == 0x0100) \
        ) \
        allowSimpleCase = TRUE;  \
    \
    for (uint32 Line = GFX.StartY; Line <= GFX.EndY; Line++, Screen += GFX.Pitch, Depth += GFX.PPL, l++) \
    { \
        int yy; \
        \
        int HOffset = ((int) LineData [Line].BG[0].HOffset << M7) >> M7; \
        int VOffset = ((int) LineData [Line].BG[0].VOffset << M7) >> M7; \
        \
        int CentreX = ((int) l->CentreX << M7) >> M7; \
        int CentreY = ((int) l->CentreY << M7) >> M7; \
        \
        if (PPU.Mode7VFlip) \
            yy = 255 - (int) Line; \
        else \
            yy = Line; \
        \
	\
	    yy += CLIP_10_BIT_SIGNED(VOffset - CentreY); \
        bool8 simpleCase = FALSE; \
        int BB; \
        int DD; \
        /* Make a special case for the identity matrix, since it's a common case and */ \
        /* can be done much more quickly without special effects */ \
        if (allowSimpleCase && !l->MatrixB && !l->MatrixC && (l->MatrixA == 0x0100) && (l->MatrixD == 0x0100)) \
        { \
            BB = CentreX << 8; \
            DD = (yy + CentreY) << 8; \
            simpleCase = TRUE; \
        } \
        else \
        { \
            BB = l->MatrixB * yy + (CentreX << 8); \
            DD = l->MatrixD * yy + (CentreY << 8); \
        } \
        \
        for (uint32 clip = 0; clip < ClipCount; clip++) \
        { \
            if (GFX.pCurrentClip->Count [bg]) \
            { \
                Left = GFX.pCurrentClip->Left [clip][bg]; \
                Right = GFX.pCurrentClip->Right [clip][bg]; \
                if (Right <= Left) \
                    continue; \
            } \
            TYPE *p = (TYPE *) Screen + Left; \
            uint8 *d = Depth + Left; \
            \
            if (PPU.Mode7HFlip) \
            { \
                startx = Right - 1; \
                endx = Left - 1; \
                dir = -1; \
                aa = -l->MatrixA; \
                cc = -l->MatrixC; \
            } \
            else \
            { \
                startx = Left; \
                endx = Right; \
                dir = 1; \
                aa = l->MatrixA; \
                cc = l->MatrixC; \
            } \
            int xx; \
	\
			xx = startx + CLIP_10_BIT_SIGNED(HOffset - CentreX); \
            int AA, CC = 0; \
            if (simpleCase) \
            { \
                AA = xx << 8; \
            } \
            else \
            { \
                AA = l->MatrixA * xx; \
                CC = l->MatrixC * xx; \
            } \
            if (simpleCase) \
            { \
                if (!PPU.Mode7Repeat) \
                { \
                    int x = startx; \
                    do \
                    { \
                        int X = ((AA + BB) >> 8) & 0x3ff; \
                        int Y = (DD >> 8) & 0x3ff; \
                        uint8 *TileData = VRAM1 + (Memory.VRAM[((Y & ~7) << 5) + ((X >> 2) & ~1)] << 7); \
                        uint32 b = *(TileData + ((Y & 7) << 4) + ((X & 7) << 1)); \
			GFX.Z1 = Mode7Depths [(b & GFX.Mode7PriorityMask) >> 7]; \
                        if (GFX.Z1 > *d && (b & GFX.Mode7Mask) ) \
                        { \
                            TYPE theColor = COLORFUNC; \
                            *p = (FUNC) | ALPHA_BITS_MASK; \
                            *d = GFX.Z1; \
                        } \
                        AA += aa, p++, d++; \
			x += dir; \
                    } while (x != endx); \
                } \
                else \
                { \
                    int x = startx; \
                    do { \
                        int X = (AA + BB) >> 8; \
                        int Y = DD >> 8; \
\
                        if (((X | Y) & ~0x3ff) == 0) \
                        { \
                            uint8 *TileData = VRAM1 + (Memory.VRAM[((Y & ~7) << 5) + ((X >> 2) & ~1)] << 7); \
			    uint32 b = *(TileData + ((Y & 7) << 4) + ((X & 7) << 1)); \
			    GFX.Z1 = Mode7Depths [(b & GFX.Mode7PriorityMask) >> 7]; \
                            if (GFX.Z1 > *d && (b & GFX.Mode7Mask) ) \
                            { \
                                TYPE theColor = COLORFUNC; \
                                *p = (FUNC) | ALPHA_BITS_MASK; \
                                *d = GFX.Z1; \
                            } \
                        } \
                        else if (PPU.Mode7Repeat == 3) \
                        { \
                            X = (x + HOffset) & 7; \
                            Y = (yy + CentreY) & 7; \
			    uint8 *TileData = VRAM1 + (Memory.VRAM[((Y & ~7) << 5) + ((X >> 2) & ~1)] << 7); \
			    uint32 b = *(TileData + ((Y & 7) << 4) + ((X & 7) << 1)); \
			    GFX.Z1 = Mode7Depths [(b & GFX.Mode7PriorityMask) >> 7]; \
                            if (GFX.Z1 > *d && (b & GFX.Mode7Mask) ) \
                            { \
                                TYPE theColor = COLORFUNC; \
                                *p = (FUNC) | ALPHA_BITS_MASK; \
                                *d = GFX.Z1; \
                            } \
                        } \
                        AA += aa; p++; d++; \
			x += dir; \
                    } while (x != endx); \
                } \
            } \
            else if (!PPU.Mode7Repeat) \
            { \
                /* The bilinear interpolator: get the colors at the four points surrounding */ \
                /* the location of one point in the _sampled_ image, and weight them according */ \
                /* to their (city block) distance.  It's very smooth, but blurry with "close up" */ \
                /* points. */ \
                \
                /* 460 (slightly less than 2 source pixels per displayed pixel) is an educated */ \
                /* guess for where bilinear filtering will become a poor method for averaging. */ \
                /* (When reducing the image, the weighting used by a bilinear filter becomes */ \
                /* arbitrary, and a simple mean is a better way to represent the source image.) */ \
                /* You can think of this as a kind of mipmapping. */ \
                if ((aa < 460 && aa > -460) && (cc < 460 && cc > -460)) \
                {\
                    for (int x = startx; x != endx; x += dir, AA += aa, CC += cc, p++, d++) \
                    { \
                        uint32 xPos = AA + BB; \
                        uint32 xPix = xPos >> 8; \
                        uint32 yPos = CC + DD; \
                        uint32 yPix = yPos >> 8; \
                        uint32 X = xPix & 0x3ff; \
                        uint32 Y = yPix & 0x3ff; \
                        uint8 *TileData = VRAM1 + (Memory.VRAM[((Y & ~7) << 5) + ((X >> 2) & ~1)] << 7); \
			uint32 b = *(TileData + ((Y & 7) << 4) + ((X & 7) << 1)); \
			GFX.Z1 = Mode7Depths [(b & GFX.Mode7PriorityMask) >> 7]; \
                        if (GFX.Z1 > *d && (b & GFX.Mode7Mask) ) \
                        { \
                            /* X10 and Y01 are the X and Y coordinates of the next source point over. */ \
                            uint32 X10 = (xPix + dir) & 0x3ff; \
                            uint32 Y01 = (yPix + (PPU.Mode7VFlip?-1:1)) & 0x3ff; \
                            uint8 *TileData10 = VRAM1 + (Memory.VRAM[((Y & ~7) << 5) + ((X10 >> 2) & ~1)] << 7); \
                            uint8 *TileData11 = VRAM1 + (Memory.VRAM[((Y01 & ~7) << 5) + ((X10 >> 2) & ~1)] << 7); \
                            uint8 *TileData01 = VRAM1 + (Memory.VRAM[((Y01 & ~7) << 5) + ((X >> 2) & ~1)] << 7); \
                            uint32 p1 = COLORFUNC; \
                            p1 = (p1 & FIRST_THIRD_COLOR_MASK) | ((p1 & SECOND_COLOR_MASK) << 16); \
                            b = *(TileData10 + ((Y & 7) << 4) + ((X10 & 7) << 1)); \
                            uint32 p2 = COLORFUNC; \
                            p2 = (p2 & FIRST_THIRD_COLOR_MASK) | ((p2 & SECOND_COLOR_MASK) << 16); \
                            b = *(TileData11 + ((Y01 & 7) << 4) + ((X10 & 7) << 1)); \
                            uint32 p4 = COLORFUNC; \
                            p4 = (p4 & FIRST_THIRD_COLOR_MASK) | ((p4 & SECOND_COLOR_MASK) << 16); \
                            b = *(TileData01 + ((Y01 & 7) << 4) + ((X & 7) << 1)); \
                            uint32 p3 = COLORFUNC; \
                            p3 = (p3 & FIRST_THIRD_COLOR_MASK) | ((p3 & SECOND_COLOR_MASK) << 16); \
                            /* Xdel, Ydel: position (in 1/32nds) between the points */ \
                            uint32 Xdel = (xPos >> 3) & 0x1F; \
                            uint32 Ydel = (yPos >> 3) & 0x1F; \
                            uint32 XY = (Xdel*Ydel) >> 5; \
                            uint32 area1 = 0x20 + XY - Xdel - Ydel; \
                            uint32 area2 = Xdel - XY; \
                            uint32 area3 = Ydel - XY; \
                            uint32 area4 = XY; \
                            if(PPU.Mode7HFlip){ \
                                uint32 tmp=area1; area1=area2; area2=tmp; \
                                tmp=area3; area3=area4; area4=tmp; \
                            } \
                            if(PPU.Mode7VFlip){ \
                                uint32 tmp=area1; area1=area3; area3=tmp; \
                                tmp=area2; area2=area4; area4=tmp; \
                            } \
                            uint32 tempColor = ((area1 * p1) + \
                                                (area2 * p2) + \
                                                (area3 * p3) + \
                                                (area4 * p4)) >> 5; \
                            TYPE theColor = (tempColor & FIRST_THIRD_COLOR_MASK) | ((tempColor >> 16) & SECOND_COLOR_MASK); \
                            *p = (FUNC) | ALPHA_BITS_MASK; \
                            *d = GFX.Z1; \
                        } \
                    } \
                } \
                else \
                    /* The oversampling method: get the colors at four corners of a square */ \
                    /* in the _displayed_ image, and average them.  It's sharp and clean, but */ \
                    /* gives the usual huge pixels when the source image gets "close." */ \
                { \
                    /* Find the dimensions of the square in the source image whose corners will be examined. */ \
                    uint32 aaDelX = aa >> 1; \
                    uint32 ccDelX = cc >> 1; \
                    uint32 bbDelY = l->MatrixB >> 1; \
                    uint32 ddDelY = l->MatrixD >> 1; \
                    /* Offset the location within the source image so that the four sampled points */ \
                    /* center around where the single point would otherwise have been drawn. */ \
                    BB -= (bbDelY >> 1); \
                    DD -= (ddDelY >> 1); \
                    AA -= (aaDelX >> 1); \
                    CC -= (ccDelX >> 1); \
                    uint32 BB10 = BB + aaDelX; \
                    uint32 BB01 = BB + bbDelY; \
                    uint32 BB11 = BB + aaDelX + bbDelY; \
                    uint32 DD10 = DD + ccDelX; \
                    uint32 DD01 = DD + ddDelY; \
                    uint32 DD11 = DD + ccDelX + ddDelY; \
                    for (int x = startx; x != endx; x += dir, AA += aa, CC += cc, p++, d++) \
                    { \
                        uint32 X = ((AA + BB) >> 8) & 0x3ff; \
                        uint32 Y = ((CC + DD) >> 8) & 0x3ff; \
                        uint8 *TileData = VRAM1 + (Memory.VRAM[((Y & ~7) << 5) + ((X >> 2) & ~1)] << 7); \
			uint32 b = *(TileData + ((Y & 7) << 4) + ((X & 7) << 1)); \
			GFX.Z1 = Mode7Depths [(b & GFX.Mode7PriorityMask) >> 7]; \
                        if (GFX.Z1 > *d && (b & GFX.Mode7Mask) ) \
                        { \
                            /* X, Y, X10, Y10, etc. are the coordinates of the four pixels within the */ \
                            /* source image that we're going to examine. */ \
                            uint32 X10 = ((AA + BB10) >> 8) & 0x3ff; \
                            uint32 Y10 = ((CC + DD10) >> 8) & 0x3ff; \
                            uint32 X01 = ((AA + BB01) >> 8) & 0x3ff; \
                            uint32 Y01 = ((CC + DD01) >> 8) & 0x3ff; \
                            uint32 X11 = ((AA + BB11) >> 8) & 0x3ff; \
                            uint32 Y11 = ((CC + DD11) >> 8) & 0x3ff; \
                            uint8 *TileData10 = VRAM1 + (Memory.VRAM[((Y10 & ~7) << 5) + ((X10 >> 2) & ~1)] << 7); \
                            uint8 *TileData01 = VRAM1 + (Memory.VRAM[((Y01 & ~7) << 5) + ((X01 >> 2) & ~1)] << 7); \
                            uint8 *TileData11 = VRAM1 + (Memory.VRAM[((Y11 & ~7) << 5) + ((X11 >> 2) & ~1)] << 7); \
                            TYPE p1 = COLORFUNC; \
                            b = *(TileData10 + ((Y10 & 7) << 4) + ((X10 & 7) << 1)); \
                            TYPE p2 = COLORFUNC; \
                            b = *(TileData01 + ((Y01 & 7) << 4) + ((X01 & 7) << 1)); \
                            TYPE p3 = COLORFUNC; \
                            b = *(TileData11 + ((Y11 & 7) << 4) + ((X11 & 7) << 1)); \
                            TYPE p4 = COLORFUNC; \
                            TYPE theColor = Q_INTERPOLATE(p1, p2, p3, p4); \
                            *p = (FUNC) | ALPHA_BITS_MASK; \
                            *d = GFX.Z1; \
                        } \
                    } \
                } \
            } \
            else \
            { \
                for (int x = startx; x != endx; x += dir, AA += aa, CC += cc, p++, d++) \
                { \
                    uint32 xPos = AA + BB; \
                    uint32 xPix = xPos >> 8; \
                    uint32 yPos = CC + DD; \
                    uint32 yPix = yPos >> 8; \
                    uint32 X = xPix; \
                    uint32 Y = yPix; \
                    \
\
                    if (((X | Y) & ~0x3ff) == 0) \
                    { \
                        uint8 *TileData = VRAM1 + (Memory.VRAM[((Y & ~7) << 5) + ((X >> 2) & ~1)] << 7); \
			uint32 b = *(TileData + ((Y & 7) << 4) + ((X & 7) << 1)); \
			GFX.Z1 = Mode7Depths [(b & GFX.Mode7PriorityMask) >> 7]; \
                        if (GFX.Z1 > *d && (b & GFX.Mode7Mask) ) \
                        { \
                            /* X10 and Y01 are the X and Y coordinates of the next source point over. */ \
                            uint32 X10 = (xPix + dir) & 0x3ff; \
                            uint32 Y01 = (yPix + dir) & 0x3ff; \
                            uint8 *TileData10 = VRAM1 + (Memory.VRAM[((Y & ~7) << 5) + ((X10 >> 2) & ~1)] << 7); \
                            uint8 *TileData11 = VRAM1 + (Memory.VRAM[((Y01 & ~7) << 5) + ((X10 >> 2) & ~1)] << 7); \
                            uint8 *TileData01 = VRAM1 + (Memory.VRAM[((Y01 & ~7) << 5) + ((X >> 2) & ~1)] << 7); \
                            uint32 p1 = COLORFUNC; \
                            p1 = (p1 & FIRST_THIRD_COLOR_MASK) | ((p1 & SECOND_COLOR_MASK) << 16); \
                            b = *(TileData10 + ((Y & 7) << 4) + ((X10 & 7) << 1)); \
                            uint32 p2 = COLORFUNC; \
                            p2 = (p2 & FIRST_THIRD_COLOR_MASK) | ((p2 & SECOND_COLOR_MASK) << 16); \
                            b = *(TileData11 + ((Y01 & 7) << 4) + ((X10 & 7) << 1)); \
                            uint32 p4 = COLORFUNC; \
                            p4 = (p4 & FIRST_THIRD_COLOR_MASK) | ((p4 & SECOND_COLOR_MASK) << 16); \
                            b = *(TileData01 + ((Y01 & 7) << 4) + ((X & 7) << 1)); \
                            uint32 p3 = COLORFUNC; \
                            p3 = (p3 & FIRST_THIRD_COLOR_MASK) | ((p3 & SECOND_COLOR_MASK) << 16); \
                            /* Xdel, Ydel: position (in 1/32nds) between the points */ \
                            uint32 Xdel = (xPos >> 3) & 0x1F; \
                            uint32 Ydel = (yPos >> 3) & 0x1F; \
                            uint32 XY = (Xdel*Ydel) >> 5; \
                            uint32 area1 = 0x20 + XY - Xdel - Ydel; \
                            uint32 area2 = Xdel - XY; \
                            uint32 area3 = Ydel - XY; \
                            uint32 area4 = XY; \
                            uint32 tempColor = ((area1 * p1) + \
                                                (area2 * p2) + \
                                                (area3 * p3) + \
                                                (area4 * p4)) >> 5; \
                            TYPE theColor = (tempColor & FIRST_THIRD_COLOR_MASK) | ((tempColor >> 16) & SECOND_COLOR_MASK); \
                            *p = (FUNC) | ALPHA_BITS_MASK; \
                            *d = GFX.Z1; \
                        } \
                    } \
                    else \
                    { \
                        if (PPU.Mode7Repeat == 3) \
                        { \
                            X = (x + HOffset) & 7; \
                            Y = (yy + CentreY) & 7; \
			    uint32 b = *(VRAM1 + ((Y & 7) << 4) + ((X & 7) << 1)); \
			    GFX.Z1 = Mode7Depths [(b & GFX.Mode7PriorityMask) >> 7]; \
                            if (GFX.Z1 > *d && (b & GFX.Mode7Mask) ) \
                            { \
                                TYPE theColor = COLORFUNC; \
                                *p = (FUNC) | ALPHA_BITS_MASK; \
                                *d = GFX.Z1; \
                            } \
                        } \
                    } \
                } \
            } \
        } \
    }

STATIC uint32 Q_INTERPOLATE(uint32 A, uint32 B, uint32 C, uint32 D)
{
    register uint32 x = ((A >> 2) & HIGH_BITS_SHIFTED_TWO_MASK) +
                            ((B >> 2) & HIGH_BITS_SHIFTED_TWO_MASK) +
                            ((C >> 2) & HIGH_BITS_SHIFTED_TWO_MASK) +
                            ((D >> 2) & HIGH_BITS_SHIFTED_TWO_MASK);
    register uint32 y = (A & TWO_LOW_BITS_MASK) +
                            (B & TWO_LOW_BITS_MASK) +
                            (C & TWO_LOW_BITS_MASK) +
                            (D & TWO_LOW_BITS_MASK);
    y = (y>>2) & TWO_LOW_BITS_MASK;
    return x+y;
}

void DrawBGMode7Background16_i (uint8 *Screen, int bg)
{
    RENDER_BACKGROUND_MODE7_i (uint16, theColor, (GFX.ScreenColors[b & GFX.Mode7Mask]));
}

void DrawBGMode7Background16Add_i (uint8 *Screen, int bg)
{
    RENDER_BACKGROUND_MODE7_i (uint16, *(d + GFX.DepthDelta) ?
					(*(d + GFX.DepthDelta) != 1 ?
					    (COLOR_ADD (theColor,
						       p [GFX.Delta])) :
					    (COLOR_ADD (theColor,
						       GFX.FixedColour))) :
					 theColor, (GFX.ScreenColors[b & GFX.Mode7Mask]));
}

void DrawBGMode7Background16Add1_2_i (uint8 *Screen, int bg)
{
    RENDER_BACKGROUND_MODE7_i (uint16, *(d + GFX.DepthDelta) ?
					(*(d + GFX.DepthDelta) != 1 ?
					    COLOR_ADD1_2 (theColor,
						          p [GFX.Delta]) :
					    COLOR_ADD (theColor,
						       GFX.FixedColour)) :
					 theColor, (GFX.ScreenColors[b & GFX.Mode7Mask]));
}

void DrawBGMode7Background16Sub_i (uint8 *Screen, int bg)
{
    RENDER_BACKGROUND_MODE7_i (uint16, *(d + GFX.DepthDelta) ?
					(*(d + GFX.DepthDelta) != 1 ?
					    COLOR_SUB (theColor,
						       p [GFX.Delta]) :
					    COLOR_SUB (theColor,
						       GFX.FixedColour)) :
					 theColor, (GFX.ScreenColors[b & GFX.Mode7Mask]));
}

void DrawBGMode7Background16Sub1_2_i (uint8 *Screen, int bg)
{
    RENDER_BACKGROUND_MODE7_i (uint16, *(d + GFX.DepthDelta) ?
					(*(d + GFX.DepthDelta) != 1 ?
					    COLOR_SUB1_2 (theColor,
						       p [GFX.Delta]) :
					    COLOR_SUB (theColor,
						       GFX.FixedColour)) :
					 theColor, (GFX.ScreenColors[b & GFX.Mode7Mask]));
}

#define _BUILD_SETUP(F) \
GFX.BuildPixel = BuildPixel##F; \
GFX.BuildPixel2 = BuildPixel2##F; \
GFX.DecomposePixel = DecomposePixel##F; \
RED_LOW_BIT_MASK = RED_LOW_BIT_MASK_##F; \
GREEN_LOW_BIT_MASK = GREEN_LOW_BIT_MASK_##F; \
BLUE_LOW_BIT_MASK = BLUE_LOW_BIT_MASK_##F; \
RED_HI_BIT_MASK = RED_HI_BIT_MASK_##F; \
GREEN_HI_BIT_MASK = GREEN_HI_BIT_MASK_##F; \
BLUE_HI_BIT_MASK = BLUE_HI_BIT_MASK_##F; \
MAX_RED = MAX_RED_##F; \
MAX_GREEN = MAX_GREEN_##F; \
MAX_BLUE = MAX_BLUE_##F; \
GREEN_HI_BIT = ((MAX_GREEN_##F + 1) >> 1); \
SPARE_RGB_BIT_MASK = SPARE_RGB_BIT_MASK_##F; \
RGB_LOW_BITS_MASK = (RED_LOW_BIT_MASK_##F | \
 		     GREEN_LOW_BIT_MASK_##F | \
		     BLUE_LOW_BIT_MASK_##F); \
RGB_HI_BITS_MASK = (RED_HI_BIT_MASK_##F | \
		    GREEN_HI_BIT_MASK_##F | \
		    BLUE_HI_BIT_MASK_##F); \
RGB_HI_BITS_MASKx2 = ((RED_HI_BIT_MASK_##F | \
		       GREEN_HI_BIT_MASK_##F | \
		       BLUE_HI_BIT_MASK_##F) << 1); \
RGB_REMOVE_LOW_BITS_MASK = ~RGB_LOW_BITS_MASK; \
FIRST_COLOR_MASK = FIRST_COLOR_MASK_##F; \
SECOND_COLOR_MASK = SECOND_COLOR_MASK_##F; \
THIRD_COLOR_MASK = THIRD_COLOR_MASK_##F; \
ALPHA_BITS_MASK = ALPHA_BITS_MASK_##F; \
FIRST_THIRD_COLOR_MASK = FIRST_COLOR_MASK | THIRD_COLOR_MASK; \
TWO_LOW_BITS_MASK = RGB_LOW_BITS_MASK | (RGB_LOW_BITS_MASK << 1); \
HIGH_BITS_SHIFTED_TWO_MASK = (( (FIRST_COLOR_MASK | SECOND_COLOR_MASK | THIRD_COLOR_MASK) & \
                                ~TWO_LOW_BITS_MASK ) >> 2);

void RenderScreen (uint8 *Screen, bool8 sub, bool8 force_no_add, uint8 D)
{
    bool8 BG0;
    bool8 BG1;
    bool8 BG2;
    bool8 BG3;
    bool8 OB;
	
    GFX.S = Screen;
	
    if (!sub)
    {
		GFX.pCurrentClip = &IPPU.Clip [0];
		BG0 = ON_MAIN (0);
		BG1 = ON_MAIN (1);
		BG2 = ON_MAIN (2);
		BG3 = ON_MAIN (3);
		OB  = ON_MAIN (4);
    }
    else
    {
		GFX.pCurrentClip = &IPPU.Clip [1];
		BG0 = ON_SUB (0);
		BG1 = ON_SUB (1);
		BG2 = ON_SUB (2);
		BG3 = ON_SUB (3);
		OB  = ON_SUB (4);
    }
	
    sub |= force_no_add;
	
    if (PPU.BGMode <= 1)
    {
		if (OB)
		{
			SelectTileRenderer (sub || !SUB_OR_ADD(4));
			DrawOBJS (!sub, D);
		}
		if (BG0)
		{
			SelectTileRenderer (sub || !SUB_OR_ADD(0));
			DrawBackground (PPU.BGMode, 0, D + 10, D + 14);
		}
		if (BG1)
		{
			SelectTileRenderer (sub || !SUB_OR_ADD(1));
			DrawBackground (PPU.BGMode, 1, D + 9, D + 13);
		}
		if (BG2)
		{
			SelectTileRenderer (sub || !SUB_OR_ADD(2));
			DrawBackground (PPU.BGMode, 2, D + 3, 
				PPU.BG3Priority ? D + 17 : D + 6);
		}
		if (BG3 && PPU.BGMode == 0)
		{
			SelectTileRenderer (sub || !SUB_OR_ADD(3));
			DrawBackground (PPU.BGMode, 3, D + 2, D + 5);
		}
    }
    else if (PPU.BGMode != 7)
    {
		if (OB)
		{
			SelectTileRenderer (sub || !SUB_OR_ADD(4));
			DrawOBJS (!sub, D);
		}
		if (BG0)
		{
			SelectTileRenderer (sub || !SUB_OR_ADD(0));
			DrawBackground (PPU.BGMode, 0, D + 5, D + 13);
		}
		if (PPU.BGMode != 6 && BG1)
		{
			SelectTileRenderer (sub || !SUB_OR_ADD(1));
			DrawBackground (PPU.BGMode, 1, D + 2, D + 9);
		}
    }
    else
    {
		if (OB)
		{
			SelectTileRenderer (sub || !SUB_OR_ADD(4));
			DrawOBJS (!sub, D);
		}
		if (BG0 || ((Memory.FillRAM [0x2133] & 0x40) && BG1))
		{
			int bg;
			
			if ((Memory.FillRAM [0x2133] & 0x40)&&BG1)
			{
				GFX.Mode7Mask = 0x7f;
				GFX.Mode7PriorityMask = 0x80;
				Mode7Depths [0] = (BG0?5:1) + D;
				Mode7Depths [1] = 9 + D;
				bg = 1;
			}
			else
			{
				GFX.Mode7Mask = 0xff;
				GFX.Mode7PriorityMask = 0;
				Mode7Depths [0] = 5 + D;
				Mode7Depths [1] = 5 + D;
				bg = 0;
			}
			if (sub || !SUB_OR_ADD(0))
			{
				if (1/*!Settings.Mode7Interpolate*/)
					DrawBGMode7Background16 (Screen, bg);
				else
					DrawBGMode7Background16_i (Screen, bg);
			}
			else
			{
				if (GFX.r2131 & 0x80)
				{
					if (GFX.r2131 & 0x40)
					{
						if (1/*!Settings.Mode7Interpolate*/)
							DrawBGMode7Background16Sub1_2 (Screen, bg);
						else
							DrawBGMode7Background16Sub1_2_i (Screen, bg);
					}
					else
					{
						if (1/*!Settings.Mode7Interpolate*/)
							DrawBGMode7Background16Sub (Screen, bg);
						else
							DrawBGMode7Background16Sub_i (Screen, bg);
					}
				}
				else
				{
					if (GFX.r2131 & 0x40)
					{
						if (1/*!Settings.Mode7Interpolate*/)
							DrawBGMode7Background16Add1_2 (Screen, bg);
						else
							DrawBGMode7Background16Add1_2_i (Screen, bg);
					}
					else
					{
						if (1/*!Settings.Mode7Interpolate*/)
							DrawBGMode7Background16Add (Screen, bg);
						else
							DrawBGMode7Background16Add_i (Screen, bg);
					}
				}
			}
		}
    }
}

#if 0
#include "font.h"

void DisplayChar (uint8 *Screen, uint8 c)
{
    int line = (((c & 0x7f) - 32) >> 4) * font_height;
    int offset = (((c & 0x7f) - 32) & 15) * font_width;
    if (Settings.SixteenBit)
    {
		int h, w, rws;
		uint16 *s = (uint16 *) Screen;
		rws = Settings.OpenGLEnable ? IPPU.RenderedScreenWidth : GFX.PPL;
		for (h = 0; h < font_height; h++, line++,
			s += rws - font_width)
		{
			for (w = 0; w < font_width; w++, s++)
			{
				uint8 p = font [line][offset + w];
				
				if (p == '#')
				{
					/*
					if(Memory.Hacked)
						*s= BUILD_PIXEL(31,0,0);
					else if(Memory.Iffy)
						*s= BUILD_PIXEL(31,31,0);
					else if(Memory.Iformat==1)
						*s= BUILD_PIXEL(0,31,0);
					else if(Memory.Iformat==2)
						*s= BUILD_PIXEL(0,31,31);
					else *s = 0xffff;
					*/
					*s=Settings.DisplayColor;
				}
				else
					if (p == '.')
						*s = BLACK;
			}
		}
    }
    else
    {
		int h, w;
		uint8 *s = Screen;
		for (h = 0; h < font_height; h++, line++,
			s += GFX.PPL - font_width)
		{
			for (w = 0; w < font_width; w++, s++)
			{
				uint8 p = font [line][offset + w];
				
				if (p == '#')
					*s = 255;
				else
					if (p == '.')
						*s = BLACK;
			}
		}
    }
}

static void S9xDisplayFrameRate ()
{
    uint8 *Screen = GFX.Screen + 2 +
		    (IPPU.RenderedScreenHeight - font_height - 1) * GFX.Pitch2;
    char string [10];
    int len = 5;

    sprintf (string, "%02d/%02d", IPPU.DisplayedRenderedFrameCount,
	     (int) Memory.ROMFramesPerSecond);

    int i;
    for (i = 0; i < len; i++)
    {
	DisplayChar (Screen, string [i]);
	Screen += Settings.SixteenBit ? (font_width - 1) * sizeof (uint16) :
		  (font_width - 1);
    }
}

static void S9xDisplayString (const char *string)
{
    uint8 *Screen = GFX.Screen + 2 +
		    (IPPU.RenderedScreenHeight - font_height * 5) * GFX.Pitch2;
    int len = strlen (string);
    int max_chars = IPPU.RenderedScreenWidth / (font_width - 1);
    int char_count = 0;
    int i;

    for (i = 0; i < len; i++, char_count++)
    {
	if (char_count >= max_chars || string [i] < 32)
	{
	    Screen -= Settings.SixteenBit ?
			(font_width - 1) * sizeof (uint16) * max_chars :
			(font_width - 1) * max_chars;
	    Screen += font_height * GFX.Pitch;
	    if (Screen >= GFX.Screen + GFX.Pitch * IPPU.RenderedScreenHeight)
		break;
	    char_count -= max_chars;
	}
	if (string [i] < 32)
	    continue;
	DisplayChar (Screen, string [i]);
	Screen += Settings.SixteenBit ? (font_width - 1) * sizeof (uint16) :
		  (font_width - 1);
    }
}
#endif

void S9xUpdateScreen ()
{
    int32 x2 = 1;
	
    GFX.S = GFX.Screen;
    GFX.r2131 = Memory.FillRAM [0x2131];
    GFX.r212c = Memory.FillRAM [0x212c];
    GFX.r212d = Memory.FillRAM [0x212d];
    GFX.r2130 = Memory.FillRAM [0x2130];

#ifdef JP_FIX

    GFX.Pseudo = (Memory.FillRAM [0x2133] & 8) != 0 &&
				 (GFX.r212c & 15) != (GFX.r212d & 15) &&
				 (GFX.r2131 == 0x3f);

#else

    GFX.Pseudo = (Memory.FillRAM [0x2133] & 8) != 0 &&
		(GFX.r212c & 15) != (GFX.r212d & 15) &&
		(GFX.r2131 & 0x3f) == 0;

#endif
	
    if (IPPU.OBJChanged)
		S9xSetupOBJ ();
	
    if (PPU.RecomputeClipWindows)
    {
		ComputeClipWindows ();
		PPU.RecomputeClipWindows = FALSE;
    }
	
    GFX.StartY = IPPU.PreviousLine;
    if ((GFX.EndY = IPPU.CurrentLine - 1) >= PPU.ScreenHeight)
		GFX.EndY = PPU.ScreenHeight - 1;

	// XXX: Check ForceBlank? Or anything else?
	PPU.RangeTimeOver |= GFX.OBJLines[GFX.EndY].RTOFlags;
	
    uint32 starty = GFX.StartY;
    uint32 endy = GFX.EndY;
	
    if (Settings.SupportHiRes &&
		(PPU.BGMode == 5 || PPU.BGMode == 6 ||
		IPPU.Interlace || IPPU.DoubleHeightPixels))
    {
		if (PPU.BGMode == 5 || PPU.BGMode == 6|| IPPU.Interlace)
		{
			IPPU.RenderedScreenWidth = 512;
			x2 = 2;
		}
		if (IPPU.DoubleHeightPixels)
		{
			starty = GFX.StartY * 2;
			endy = GFX.EndY * 2 + 1;
		}
		if ((PPU.BGMode == 5 || PPU.BGMode == 6) && !IPPU.DoubleWidthPixels)
		{
			// The game has switched from lo-res to hi-res mode part way down
			// the screen. Scale any existing lo-res pixels on screen
			if (Settings.SixteenBit)
			{
#if defined (USE_GLIDE) || defined (USE_OPENGL)
                if (
#ifdef USE_GLIDE
                    (Settings.GlideEnable && GFX.Pitch == 512) ||
#endif
#ifdef USE_OPENGL
                    (Settings.OpenGLEnable && GFX.Pitch == 512) ||
#endif
                    0)
				{
					// Have to back out of the speed up hack where the low res.
					// SNES image was rendered into a 256x239 sized buffer,
					// ignoring the true, larger size of the buffer.
					for (register int32 y = (int32) starty - 1; y >= 0; y--)
					{
						register uint16 *p = (uint16 *) (GFX.Screen + y * GFX.Pitch2) + 255;
						register uint16 *q = (uint16 *) (GFX.Screen + y * GFX.RealPitch) + 510;
						for (register int x = 255; x >= 0; x--, p--, q -= 2)
							*q = *(q + 1) = *p;
					}
					
					GFX.Pitch = GFX.Pitch2 = GFX.RealPitch;
                    GFX.PPL = GFX.Pitch >> 1;
                    GFX.PPLx2 = GFX.Pitch;
                    GFX.ZPitch = GFX.PPL;
				}
				else
#endif
					for (register uint32 y = 0; y < starty; y++)
					{
						register uint16 *p = (uint16 *) (GFX.Screen + y * GFX.Pitch2) + 255;
						register uint16 *q = (uint16 *) (GFX.Screen + y * GFX.Pitch2) + 510;
						
						for (register int x = 255; x >= 0; x--, p--, q -= 2)
							*q = *(q + 1) = *p;
					}
			}
			else
			{
				for (register uint32 y = 0; y < starty; y++)
				{
					register uint8 *p = GFX.Screen + y * GFX.Pitch2 + 255;
					register uint8 *q = GFX.Screen + y * GFX.Pitch2 + 510;
					for (register int x = 255; x >= 0; x--, p--, q -= 2)
						*q = *(q + 1) = *p;
				}
			}
			IPPU.DoubleWidthPixels = TRUE;
		}
        // BJ: And we have to change the height if Interlace gets set,
        //     too.
		if (IPPU.Interlace && !IPPU.DoubleHeightPixels)
		{
			starty = GFX.StartY * 2;
			endy = GFX.EndY * 2 + 1;
            IPPU.RenderedScreenHeight = PPU.ScreenHeight << 1;
            IPPU.DoubleHeightPixels = TRUE;
            GFX.Pitch2 = GFX.RealPitch;
            GFX.Pitch = GFX.RealPitch * 2;
            if (Settings.SixteenBit)
                GFX.PPL = GFX.PPLx2 = GFX.RealPitch;
            else
                GFX.PPL = GFX.PPLx2 = GFX.RealPitch << 1;
			
            // The game has switched from non-interlaced to interlaced mode
            // part way down the screen. Scale everything.
            for (register int32 y = (int32) GFX.StartY - 1; y >= 0; y--)
			{
				memmove (GFX.Screen + y * 2 * GFX.Pitch2,
					GFX.Screen + y * GFX.Pitch2,
					GFX.Pitch2);
				memmove (GFX.Screen + (y * 2 + 1) * GFX.Pitch2,
					GFX.Screen + y * GFX.Pitch2,
					GFX.Pitch2);
			}
		}
		
    }
	
    uint32 black = BLACK | (BLACK << 16);
	
    if (Settings.Transparency && Settings.SixteenBit)
    {
		if (GFX.Pseudo)
		{
			GFX.r2131 = 0x5f;
            GFX.r212c &= (Memory.FillRAM [0x212d] | 0xf0);
            GFX.r212d |= (Memory.FillRAM [0x212c] & 0x0f);
			GFX.r2130 |= 2;
		}
		
		if (!PPU.ForcedBlanking && ADD_OR_SUB_ON_ANYTHING &&
			(GFX.r2130 & 0x30) != 0x30 &&
			!((GFX.r2130 & 0x30) == 0x10 && IPPU.Clip[1].Count[5] == 0))
		{
			struct ClipData *pClip;
			
			GFX.FixedColour = BUILD_PIXEL (IPPU.XB [PPU.FixedColourRed],
				IPPU.XB [PPU.FixedColourGreen],
				IPPU.XB [PPU.FixedColourBlue]);
			
			// Clear the z-buffer, marking areas 'covered' by the fixed
			// colour as depth 1.
			pClip = &IPPU.Clip [1];
			
			// Clear the z-buffer
			if (pClip->Count [5])
			{
				// Colour window enabled.
				for (uint32 y = starty; y <= endy; y++)
				{
					ZeroMemory (GFX.SubZBuffer + y * GFX.ZPitch,
						IPPU.RenderedScreenWidth);
					ZeroMemory (GFX.ZBuffer + y * GFX.ZPitch,
						IPPU.RenderedScreenWidth);
					if (IPPU.Clip [0].Count [5])
					{
						uint32 *p = (uint32 *) (GFX.SubScreen + y * GFX.Pitch2);
						uint32 *q = (uint32 *) ((uint16 *) p + IPPU.RenderedScreenWidth);
						while (p < q)
							*p++ = black;
					}
					for (uint32 c = 0; c < pClip->Count [5]; c++)
					{
						if (pClip->Right [c][5] > pClip->Left [c][5])
						{
							memset (GFX.SubZBuffer + y * GFX.ZPitch + pClip->Left [c][5] * x2,
								1, (pClip->Right [c][5] - pClip->Left [c][5]) * x2);
							if (IPPU.Clip [0].Count [5])
							{
								// Blast, have to clear the sub-screen to the fixed-colour
								// because there is a colour window in effect clipping
								// the main screen that will allow the sub-screen
								// 'underneath' to show through.
								
								uint16 *p = (uint16 *) (GFX.SubScreen + y * GFX.Pitch2);
								uint16 *q = p + pClip->Right [c][5] * x2;
								p += pClip->Left [c][5] * x2;
								
								while (p < q)
									*p++ = (uint16) GFX.FixedColour;
							}
						}
					}
				}
			}
			else
			{
				for (uint32 y = starty; y <= endy; y++)
				{
					ZeroMemory (GFX.ZBuffer + y * GFX.ZPitch,
						IPPU.RenderedScreenWidth);
					memset (GFX.SubZBuffer + y * GFX.ZPitch, 1,
						IPPU.RenderedScreenWidth);
					
					if (IPPU.Clip [0].Count [5])
					{
						// Blast, have to clear the sub-screen to the fixed-colour
						// because there is a colour window in effect clipping
						// the main screen that will allow the sub-screen
						// 'underneath' to show through.
						
						uint32 b = GFX.FixedColour | (GFX.FixedColour << 16);
						uint32 *p = (uint32 *) (GFX.SubScreen + y * GFX.Pitch2);
						uint32 *q = (uint32 *) ((uint16 *) p + IPPU.RenderedScreenWidth);
						
						while (p < q)
							*p++ = b;
					}
				}
			}
			if (ANYTHING_ON_SUB)
			{
				GFX.DB = GFX.SubZBuffer;
				RenderScreen (GFX.SubScreen, TRUE, TRUE, SUB_SCREEN_DEPTH);
			}
			
			if (IPPU.Clip [0].Count [5])
			{
				for (uint32 y = starty; y <= endy; y++)
				{
					register uint16 *p = (uint16 *) (GFX.Screen + y * GFX.Pitch2);
					register uint8 *d = GFX.SubZBuffer + y * GFX.ZPitch;
					register uint8 *e = d + IPPU.RenderedScreenWidth;
					
					while (d < e)
					{
						if (*d > 1)
							*p = *(p + GFX.Delta);
						else
							*p = BLACK;
						d++;
						p++;
					}
				}
			}
			
			GFX.DB = GFX.ZBuffer;
			RenderScreen (GFX.Screen, FALSE, FALSE, MAIN_SCREEN_DEPTH);
			
			if (SUB_OR_ADD(5))
			{
				uint32 back = IPPU.ScreenColors [0];
				uint32 Left = 0;
				uint32 Right = 256;
				uint32 Count;
				
				pClip = &IPPU.Clip [0];
				for (uint32 y = starty; y <= endy; y++)
				{
					if (!(Count = pClip->Count [5]))
					{
						Left = 0;
						Right = 256 * x2;
						Count = 1;
					}
					
					for (uint32 b = 0; b < Count; b++)
					{
						if (pClip->Count [5])
						{
							Left = pClip->Left [b][5] * x2;
							Right = pClip->Right [b][5] * x2;
							if (Right <= Left)
								continue;
						}
						
						if (GFX.r2131 & 0x80)
						{
							if (GFX.r2131 & 0x40)
							{
								// Subtract, halving the result.
								register uint16 *p = (uint16 *) (GFX.Screen + y * GFX.Pitch2) + Left;
								register uint8 *d = GFX.ZBuffer + y * GFX.ZPitch;
								register uint8 *s = GFX.SubZBuffer + y * GFX.ZPitch + Left;
								register uint8 *e = d + Right;
								uint16 back_fixed = COLOR_SUB (back, GFX.FixedColour);
								
								d += Left;
								while (d < e)
								{
									if (*d == 0)
									{
										if (*s)
										{
											if (*s != 1)
												*p = COLOR_SUB1_2 (back, *(p + GFX.Delta));
											else
												*p = back_fixed;
										}
										else
											*p = (uint16) back;
									}
									d++;
									p++;
									s++;
								}
							}
							else
							{
								// Subtract
								register uint16 *p = (uint16 *) (GFX.Screen + y * GFX.Pitch2) + Left;
								register uint8 *s = GFX.SubZBuffer + y * GFX.ZPitch + Left;
								register uint8 *d = GFX.ZBuffer + y * GFX.ZPitch;
								register uint8 *e = d + Right;
								uint16 back_fixed = COLOR_SUB (back, GFX.FixedColour);
								
								d += Left;
								while (d < e)
								{
									if (*d == 0)
									{
										if (*s)
										{
											if (*s != 1)
												*p = COLOR_SUB (back, *(p + GFX.Delta));
											else
												*p = back_fixed;
										}
										else
											*p = (uint16) back;
									}
									d++;
									p++;
									s++;
								}
							}
						}
						else
							if (GFX.r2131 & 0x40)
							{
								register uint16 *p = (uint16 *) (GFX.Screen + y * GFX.Pitch2) + Left;
								register uint8 *d = GFX.ZBuffer + y * GFX.ZPitch;
								register uint8 *s = GFX.SubZBuffer + y * GFX.ZPitch + Left;
								register uint8 *e = d + Right;
								uint16 back_fixed = COLOR_ADD (back, GFX.FixedColour);
								d += Left;
								while (d < e)
								{
									if (*d == 0)
									{
										if (*s)
										{
											if (*s != 1)
												*p = COLOR_ADD1_2 (back, *(p + GFX.Delta));
											else
												*p = back_fixed;
										}
										else
											*p = (uint16) back;
									}
									d++;
									p++;
									s++;
								}
							}
							else
								if (back != 0)
								{
									register uint16 *p = (uint16 *) (GFX.Screen + y * GFX.Pitch2) + Left;
									register uint8 *d = GFX.ZBuffer + y * GFX.ZPitch;
									register uint8 *s = GFX.SubZBuffer + y * GFX.ZPitch + Left;
									register uint8 *e = d + Right;
									uint16 back_fixed = COLOR_ADD (back, GFX.FixedColour);
									d += Left;
									while (d < e)
									{
										if (*d == 0)
										{
											if (*s)
											{
												if (*s != 1)
													*p = COLOR_ADD (back, *(p + GFX.Delta));
												else	
													*p = back_fixed;
											}
											else
												*p = (uint16) back;
										}
										d++;
										p++;
										s++;
									}
								}
								else
								{
									if (!pClip->Count [5])
									{
										// The backdrop has not been cleared yet - so
										// copy the sub-screen to the main screen
										// or fill it with the back-drop colour if the
										// sub-screen is clear.
										register uint16 *p = (uint16 *) (GFX.Screen + y * GFX.Pitch2) + Left;
										register uint8 *d = GFX.ZBuffer + y * GFX.ZPitch;
										register uint8 *s = GFX.SubZBuffer + y * GFX.ZPitch + Left;
										register uint8 *e = d + Right;
										d += Left;
										while (d < e)
										{
											if (*d == 0)
											{
												if (*s)
												{
													if (*s != 1)
														*p = *(p + GFX.Delta);
													else	
														*p = GFX.FixedColour;
												}
												else
													*p = (uint16) back;
											}
											d++;
											p++;
											s++;
										}
									}
								}
			}
		}
		}
		else
		{
			// Subscreen not being added to back
			uint32 back = IPPU.ScreenColors [0] | (IPPU.ScreenColors [0] << 16);
			pClip = &IPPU.Clip [0];
			
			if (pClip->Count [5])
			{
				for (uint32 y = starty; y <= endy; y++)
				{
					for (uint32 b = 0; b < pClip->Count [5]; b++)
					{
						uint32 Left = pClip->Left [b][5] * x2;
						uint32 Right = pClip->Right [b][5] * x2;
						uint16 *p = (uint16 *) (GFX.Screen + y * GFX.Pitch2) + Left;
						uint8 *d = GFX.ZBuffer + y * GFX.ZPitch;
						uint8 *e = d + Right;
						d += Left;
						
						while (d < e)
						{
							if (*d == 0)
								*p = (int16) back;
							d++;
							p++;
						}
					}
				}
			}
			else
			{
				for (uint32 y = starty; y <= endy; y++)
				{
					uint16 *p = (uint16 *) (GFX.Screen + y * GFX.Pitch2);
					uint8 *d = GFX.ZBuffer + y * GFX.ZPitch;
					uint8 *e = d + 256 * x2;
					
					while (d < e)
					{
						if (*d == 0)
							*p = (int16) back;
						d++;
						p++;
					}
				}
			}
		}
	}
	else
	{
		// 16bit and transparency but currently no transparency effects in
		// operation.
		
		uint32 back = IPPU.ScreenColors [0] | 
			(IPPU.ScreenColors [0] << 16);
		
		if (PPU.ForcedBlanking)
			back = black;
		
		if (IPPU.Clip [0].Count[5])
		{
			for (uint32 y = starty; y <= endy; y++)
			{
				uint32 *p = (uint32 *) (GFX.Screen + y * GFX.Pitch2);
				uint32 *q = (uint32 *) ((uint16 *) p + IPPU.RenderedScreenWidth);
				
				while (p < q)
					*p++ = black;
				
				for (uint32 c = 0; c < IPPU.Clip [0].Count [5]; c++)
				{
					if (IPPU.Clip [0].Right [c][5] > IPPU.Clip [0].Left [c][5])
					{
						uint16 *p = (uint16 *) (GFX.Screen + y * GFX.Pitch2);
						uint16 *q = p + IPPU.Clip [0].Right [c][5] * x2;
						p += IPPU.Clip [0].Left [c][5] * x2;
						
						while (p < q)
							*p++ = (uint16) back;
					}
				}
			}
		}
		else
		{
			for (uint32 y = starty; y <= endy; y++)
			{
				uint32 *p = (uint32 *) (GFX.Screen + y * GFX.Pitch2);
				uint32 *q = (uint32 *) ((uint16 *) p + IPPU.RenderedScreenWidth);
				while (p < q)
					*p++ = back;
			}
		}
		if (!PPU.ForcedBlanking)
		{
			for (uint32 y = starty; y <= endy; y++)
			{
				ZeroMemory (GFX.ZBuffer + y * GFX.ZPitch,
					IPPU.RenderedScreenWidth);
			}
			GFX.DB = GFX.ZBuffer;
			RenderScreen (GFX.Screen, FALSE, TRUE, SUB_SCREEN_DEPTH);
		}
	}
    }
    else
    {
		if (Settings.SixteenBit)
		{
			uint32 back = IPPU.ScreenColors [0] | (IPPU.ScreenColors [0] << 16);
			if (PPU.ForcedBlanking)
				back = black;
			else
				SelectTileRenderer (TRUE);
			
			for (uint32 y = starty; y <= endy; y++)
			{
				uint32 *p = (uint32 *) (GFX.Screen + y * GFX.Pitch2);
				uint32 *q = (uint32 *) ((uint16 *) p + IPPU.RenderedScreenWidth);
				while (p < q)
					*p++ = back;
			}
		}
		else
		{
			for (uint32 y = starty; y <= endy; y++)
			{
				ZeroMemory (GFX.Screen + y * GFX.Pitch2,
					IPPU.RenderedScreenWidth);
			}
		}
		if (!PPU.ForcedBlanking)
		{
			for (uint32 y = starty; y <= endy; y++)
			{
				ZeroMemory (GFX.ZBuffer + y * GFX.ZPitch,
					IPPU.RenderedScreenWidth);
			}
			GFX.DB = GFX.ZBuffer;
			GFX.pCurrentClip = &IPPU.Clip [0];
			
#define FIXCLIP(n)\
	if (GFX.r212c & (1 << (n))) \
    GFX.pCurrentClip = &IPPU.Clip [0]; \
	else \
			GFX.pCurrentClip = &IPPU.Clip [1]
			
			
#define DISPLAY(n)\
    (!(PPU.BG_Forced & n) && \
	(GFX.r212c & n) || \
			((GFX.r212d & n) && subadd))
			
			uint8 subadd = GFX.r2131 & 0x3f;
			
			bool8 BG0 = DISPLAY(1);
			bool8 BG1 = DISPLAY(2);
			bool8 BG2 = DISPLAY(4);
			bool8 BG3 = DISPLAY(8);
			bool8 OB  = DISPLAY(16);
			
			if (PPU.BGMode <= 1)
			{
				if (OB)
				{
					FIXCLIP(4);
					DrawOBJS ();
				}
				if (BG0)
				{
					FIXCLIP(0);
					DrawBackground (PPU.BGMode, 0, 10, 14);
				}
				if (BG1)
				{
					FIXCLIP(1);
					DrawBackground (PPU.BGMode, 1, 9, 13);
				}
				if (BG2)
				{
					FIXCLIP(2);
					DrawBackground (PPU.BGMode, 2, 3,
						PPU.BG3Priority ? 17 : 6);
				}
				if (BG3 && PPU.BGMode == 0)
				{
					FIXCLIP(3);
					DrawBackground (PPU.BGMode, 3, 2, 5);
				}
			}
			else if (PPU.BGMode != 7)
			{
				if (OB)
				{
					FIXCLIP(4);
					DrawOBJS ();
				}
				if (BG0)
				{
					FIXCLIP(0);
					DrawBackground (PPU.BGMode, 0, 5, 13);
				}
				if (BG1 && PPU.BGMode != 6)
				{
					FIXCLIP(1);
					DrawBackground (PPU.BGMode, 1, 2, 9);
				}
			}
			else
			{
				if (OB)
				{
					FIXCLIP(4);
					DrawOBJS ();
				}
				if (BG0 || ((Memory.FillRAM [0x2133] & 0x40) && BG1))
				{
					int bg;
					FIXCLIP(0);
					if ((Memory.FillRAM [0x2133] & 0x40) && BG1)
					{
						GFX.Mode7Mask = 0x7f;
						GFX.Mode7PriorityMask = 0x80;
						Mode7Depths [0] = (BG0?5:1);
						Mode7Depths [1] = 9;
						bg = 1;
					}
					else
					{
						GFX.Mode7Mask = 0xff;
						GFX.Mode7PriorityMask = 0;
						Mode7Depths [0] = 5;
						Mode7Depths [1] = 5;
						bg = 0;
					}
					
					if (!Settings.SixteenBit)
						DrawBGMode7Background (GFX.Screen, bg);
					else
					{
						if (1/*!Settings.Mode7Interpolate*/)
							DrawBGMode7Background16 (GFX.Screen, bg);
						else
							DrawBGMode7Background16_i (GFX.Screen, bg);
					}
				}
			}
	}
    }
    if (Settings.SupportHiRes)
    {
		if (PPU.BGMode != 5 && PPU.BGMode != 6 && IPPU.DoubleWidthPixels)
		{
			// Mixure of background modes used on screen - scale width
			// of all non-mode 5 and 6 pixels.
			if (Settings.SixteenBit)
			{
				for (register uint32 y = starty; y <= endy; y++)
				{
					register uint16 *p = (uint16 *) (GFX.Screen + y * GFX.Pitch2) + 255;
					register uint16 *q = (uint16 *) (GFX.Screen + y * GFX.Pitch2) + 510;
					for (register int x = 255; x >= 0; x--, p--, q -= 2)
						*q = *(q + 1) = *p;
				}
			}
			else
			{
				for (register uint32 y = starty; y <= endy; y++)
				{
					register uint8 *p = GFX.Screen + y * GFX.Pitch2 + 255;
					register uint8 *q = GFX.Screen + y * GFX.Pitch2 + 510;
					for (register int x = 255; x >= 0; x--, p--, q -= 2)
						*q = *(q + 1) = *p;
				}
			}
		}
		
        // Double the height of the pixels just drawn
		FIX_INTERLACE(GFX.Screen, FALSE, GFX.ZBuffer);
		
    }
    IPPU.PreviousLine = IPPU.CurrentLine;
}

#ifdef GFX_MULTI_FORMAT

#define _BUILD_PIXEL(F) \
uint32 BuildPixel##F(uint32 R, uint32 G, uint32 B) \
{ \
    return (BUILD_PIXEL_##F(R,G,B)); \
}\
uint32 BuildPixel2##F(uint32 R, uint32 G, uint32 B) \
{ \
    return (BUILD_PIXEL2_##F(R,G,B)); \
} \
void DecomposePixel##F(uint32 pixel, uint32 &R, uint32 &G, uint32 &B) \
{ \
    DECOMPOSE_PIXEL_##F(pixel,R,G,B); \
}

_BUILD_PIXEL(RGB565)
_BUILD_PIXEL(RGB555)
_BUILD_PIXEL(BGR565)
_BUILD_PIXEL(BGR555)
_BUILD_PIXEL(GBR565)
_BUILD_PIXEL(GBR555)
_BUILD_PIXEL(RGB5551)

bool8 S9xSetRenderPixelFormat (int format)
{
    extern uint32 current_graphic_format;

    current_graphic_format = format;

    switch (format)
    {
    case RGB565:
	_BUILD_SETUP(RGB565)
	return (TRUE);
    case RGB555:
	_BUILD_SETUP(RGB555)
	return (TRUE);
    case BGR565:
	_BUILD_SETUP(BGR565)
	return (TRUE);
    case BGR555:
	_BUILD_SETUP(BGR555)
	return (TRUE);
    case GBR565:
	_BUILD_SETUP(GBR565)
	return (TRUE);
    case GBR555:
	_BUILD_SETUP(GBR555)
	return (TRUE);
    case RGB5551:
        _BUILD_SETUP(RGB5551)
        return (TRUE);
    default:
	break;
    }
    return (FALSE);
}
#endif

