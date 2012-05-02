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
#include <stdlib.h>

#include "snes9x.h"
#include "memmap.h"
#include "ppu.h"

struct Band
{
    uint32 Left;
    uint32 Right;
};

#undef MIN
#undef MAX
#define MIN(A,B) ((A) < (B) ? (A) : (B))
#define MAX(A,B) ((A) > (B) ? (A) : (B))
#define BAND_EMPTY(B) (B.Left >= B.Right)
#define BANDS_INTERSECT(A,B) ((A.Left >= B.Left && A.Left < B.Right) || \
							  (B.Left >= A.Left && B.Left < A.Right))
#define OR_BANDS(R,A,B) {\
    R.Left = MIN(A.Left, B.Left); \
		R.Right = MAX(A.Right, B.Right);}

#define AND_BANDS(R,A,B) {\
    R.Left = MAX(A.Left, B.Left); \
		R.Right = MIN(A.Right, B.Right);}

static int IntCompare (const void *d1, const void *d2)
{
    if (*(uint32 *) d1 > *(uint32 *) d2)
		return (1);
    else
		if (*(uint32 *) d1 < *(uint32 *) d2)
			return (-1);
		return (0);
}

static int BandCompare (const void *d1, const void *d2)
{
    if (((struct Band *) d1)->Left > ((struct Band *) d2)->Left)
		return (1);
    else
		if (((struct Band *) d1)->Left < ((struct Band *) d2)->Left)
			return (-1);
		return (0);
}

void ComputeClipWindows ()
{
	struct ClipData *pClip = &IPPU.Clip [0];

	// Loop around the main screen then the sub-screen.
	for (int c = 0; c < 2; c++, pClip++)
	{
		// Loop around the colour window then a clip window for each of the
		// background layers.
		for (int w = 5; w >= 0; w--)
		{
			pClip->Count[w] = 0;

			if (w == 5) // The colour window...
			{
				if (c == 0) // ... on the main screen
				{
					if ((Memory.FillRAM [0x2130] & 0xc0) == 0xc0)
					{
						// The whole of the main screen is switched off,
						// completely clip everything.
						for (int i = 0; i < 6; i++)
						{
							IPPU.Clip [c].Count [i] = 1;
							IPPU.Clip [c].Left [0][i] = 1;
							IPPU.Clip [c].Right [0][i] = 0;
						}
						continue;
					}
					else
						if ((Memory.FillRAM [0x2130] & 0xc0) == 0x00)
							continue;
				}
				else
				{
					// .. colour window on the sub-screen.
					if ((Memory.FillRAM [0x2130] & 0x30) == 0x30)
					{
						// The sub-screen is switched off, completely
						// clip everything.
						for (int i = 0; i < 6; i++)
						{
							IPPU.Clip [1].Count [i] = 1;
							IPPU.Clip [1].Left [0][i] = 1;
							IPPU.Clip [1].Right [0][i] = 0;
						}
						return;
					}
					else
						if ((Memory.FillRAM [0x2130] & 0x30) == 0x00)
							continue;
				}
			}
			if (!Settings.DisableGraphicWindows)
			{
				if (w == 5 || pClip->Count [5] ||
					(Memory.FillRAM [0x212c + c] & 
					 Memory.FillRAM [0x212e + c] & (1 << w)))
				{
					struct Band Win1[3];
					struct Band Win2[3];
					uint32 Window1Enabled = 0;
					uint32 Window2Enabled = 0;
					bool8 invert = (w == 5 && 
									((c == 1 && (Memory.FillRAM [0x2130] & 0x30) == 0x10) ||
									 (c == 0 && (Memory.FillRAM [0x2130] & 0xc0) == 0x40)));

					if (w == 5 ||
						(Memory.FillRAM [0x212c + c] & Memory.FillRAM [0x212e + c] & (1 << w)))
					{
						if (PPU.ClipWindow1Enable [w])
						{
							if (!PPU.ClipWindow1Inside [w])
							{
								Win1[Window1Enabled].Left = PPU.Window1Left;
								Win1[Window1Enabled++].Right = PPU.Window1Right + 1;
							}
							else
							{
								if (PPU.Window1Left <= PPU.Window1Right)
								{
									if (PPU.Window1Left > 0)
									{
										Win1[Window1Enabled].Left = 0;
										Win1[Window1Enabled++].Right = PPU.Window1Left;
									}
									if (PPU.Window1Right < 255)
									{
										Win1[Window1Enabled].Left = PPU.Window1Right + 1;
										Win1[Window1Enabled++].Right = 256;
									}
									if (Window1Enabled == 0)
									{
										Win1[Window1Enabled].Left = 1;
										Win1[Window1Enabled++].Right = 0;
									}
								}
								else
								{
									// 'outside' a window with no range - 
									// appears to be the whole screen.
									Win1[Window1Enabled].Left = 0;
									Win1[Window1Enabled++].Right = 256;
								}
							}
						}
						if (PPU.ClipWindow2Enable [w])
						{
							if (!PPU.ClipWindow2Inside [w])
							{
								Win2[Window2Enabled].Left = PPU.Window2Left;
								Win2[Window2Enabled++].Right = PPU.Window2Right + 1;
							}
							else
							{
								if (PPU.Window2Left <= PPU.Window2Right)
								{
									if (PPU.Window2Left > 0)
									{
										Win2[Window2Enabled].Left = 0;
										Win2[Window2Enabled++].Right = PPU.Window2Left;
									}
									if (PPU.Window2Right < 255)
									{
										Win2[Window2Enabled].Left = PPU.Window2Right + 1;
										Win2[Window2Enabled++].Right = 256;
									}
									if (Window2Enabled == 0)
									{
										Win2[Window2Enabled].Left = 1;
										Win2[Window2Enabled++].Right = 0;
									}
								}
								else
								{
									Win2[Window2Enabled].Left = 0;
									Win2[Window2Enabled++].Right = 256;
								}
							}
						}
					}
					if (Window1Enabled && Window2Enabled)
					{
						// Overlap logic
						//
						// Each window will be in one of three states:
						// 1. <no range> (Left > Right. One band)
						// 2. |    ----------------             | (Left >= 0, Right <= 255, Left <= Right. One band)
						// 3. |------------           ----------| (Left1 == 0, Right1 < Left2; Left2 > Right1, Right2 == 255. Two bands)

						struct Band Bands [6];
						int B = 0;
						switch (PPU.ClipWindowOverlapLogic [w] ^ 1)
						{
						  case CLIP_OR:
							if (Window1Enabled == 1)
							{
								if (BAND_EMPTY(Win1[0]))
								{
									B = Window2Enabled;
									memmove (Bands, Win2,
											 sizeof(Win2[0]) * Window2Enabled);
								}
								else
								{
									if (Window2Enabled == 1)
									{
										if (BAND_EMPTY (Win2[0]))
											Bands[B++] = Win1[0];
										else
										{
											if (BANDS_INTERSECT (Win1[0], Win2[0]))
											{
												OR_BANDS(Bands[0],Win1[0], Win2[0])
													B = 1;
											}
											else
											{
												Bands[B++] = Win1[0];
												Bands[B++] = Win2[0];
											}
										}
									}
									else
									{
										if (BANDS_INTERSECT(Win1[0], Win2[0]))
										{
											OR_BANDS(Bands[0], Win1[0], Win2[0])
												if (BANDS_INTERSECT(Win1[0], Win2[1]))
													OR_BANDS(Bands[1], Win1[0], Win2[1])
												else
													Bands[1] = Win2[1];
											B = 1;
											if (BANDS_INTERSECT(Bands[0], Bands[1]))
												OR_BANDS(Bands[0], Bands[0], Bands[1])
											else
												B = 2;
										}
										else
											if (BANDS_INTERSECT(Win1[0], Win2[1]))
											{
												Bands[B++] = Win2[0];
												OR_BANDS(Bands[B], Win1[0], Win2[1]);
												B++;
											}
											else
											{
												Bands[0] = Win2[0];
												Bands[1] = Win1[0];
												Bands[2] = Win2[1];
												B = 3;
											}
									}
								}
							}
							else
								if (Window2Enabled == 1)
								{
									if (BAND_EMPTY(Win2[0]))
									{
										// Window 2 defines an empty range - just
										// use window 1 as the clipping (which
										// could also be empty).
										B = Window1Enabled;
										memmove (Bands, Win1,
												 sizeof(Win1[0]) * Window1Enabled);
									}
									else
									{
										// Window 1 has two bands and Window 2 has one.
										// Neither is an empty region.
										if (BANDS_INTERSECT(Win2[0], Win1[0]))
										{
											OR_BANDS(Bands[0], Win2[0], Win1[0])
												if (BANDS_INTERSECT(Win2[0], Win1[1]))
													OR_BANDS(Bands[1], Win2[0], Win1[1])
												else
													Bands[1] = Win1[1];
											B = 1;
											if (BANDS_INTERSECT(Bands[0], Bands[1]))
												OR_BANDS(Bands[0], Bands[0], Bands[1])
											else
												B = 2;
										}
										else
											if (BANDS_INTERSECT(Win2[0], Win1[1]))
											{
												Bands[B++] = Win1[0];
												OR_BANDS(Bands[B], Win2[0], Win1[1]);
												B++;
											}
											else
											{
												Bands[0] = Win1[0];
												Bands[1] = Win2[0];
												Bands[2] = Win1[1];
												B = 3;
											}
									}
								}
								else
								{
									// Both windows have two bands
									OR_BANDS(Bands[0], Win1[0], Win2[0]);
									OR_BANDS(Bands[1], Win1[1], Win2[1]);
									B = 1;
									if (BANDS_INTERSECT(Bands[0], Bands[1]))
										OR_BANDS(Bands[0], Bands[0], Bands[1])
									else
										B = 2;
								}
							break;

						  case CLIP_AND:
							if (Window1Enabled == 1)
							{
								// Window 1 has one band
								if (BAND_EMPTY(Win1[0]))
									Bands [B++] = Win1[0];
								else
									if (Window2Enabled == 1)
									{
										if (BAND_EMPTY (Win2[0]))
											Bands [B++] = Win2[0];
										else
										{
											AND_BANDS(Bands[0], Win1[0], Win2[0]);
											B = 1;
										}
									}
									else
									{
										AND_BANDS(Bands[0], Win1[0], Win2[0]);
										AND_BANDS(Bands[1], Win1[0], Win2[1]);
										B = 2;
									}
							}
							else
								if (Window2Enabled == 1)
								{
									if (BAND_EMPTY(Win2[0]))
										Bands[B++] = Win2[0];
									else
									{
										// Window 1 has two bands.
										AND_BANDS(Bands[0], Win1[0], Win2[0]);
										AND_BANDS(Bands[1], Win1[1], Win2[0]);
										B = 2;
									}
								}
								else
								{
									// Both windows have two bands.
									AND_BANDS(Bands[0], Win1[0], Win2[0]);
									AND_BANDS(Bands[1], Win1[1], Win2[1]);
									B = 2;
									if (BANDS_INTERSECT(Win1[0], Win2[1]))
									{
										AND_BANDS(Bands[2], Win1[0], Win2[1]);
										B = 3;
									}
									else
										if (BANDS_INTERSECT(Win1[1], Win2[0]))
										{
											AND_BANDS(Bands[2], Win1[1], Win2[0]);
											B = 3;
										}
								}
							break;
						  case CLIP_XNOR:
							invert = !invert;
							// Fall...

						  case CLIP_XOR:
							if (Window1Enabled == 1 && BAND_EMPTY(Win1[0]))
							{
								B = Window2Enabled;
								memmove (Bands, Win2,
										 sizeof(Win2[0]) * Window2Enabled);
							}
							else
								if (Window2Enabled == 1 && BAND_EMPTY(Win2[0]))
								{
									B = Window1Enabled;
									memmove (Bands, Win1,
											 sizeof(Win1[0]) * Window1Enabled);
								}
								else
								{
									uint32 p = 0;
									uint32 points [10];
									uint32 i;

									invert = !invert;
									// Build an array of points (window edges)
									points [p++] = 0;
									for (i = 0; i < Window1Enabled; i++)
									{
										points [p++] = Win1[i].Left;
										points [p++] = Win1[i].Right;
									}
									for (i = 0; i < Window2Enabled; i++)
									{
										points [p++] = Win2[i].Left;
										points [p++] = Win2[i].Right;
									}
									points [p++] = 256;
									// Sort them
									qsort ((void *) points, p, sizeof (points [0]),
										   IntCompare);
									for (i = 0; i < p; i += 2)
									{
										if (points [i] == points [i + 1])
											continue;
										Bands [B].Left = points [i];
										while (i + 2 < p && 
											   points [i + 1] == points [i + 2])
										{
											i += 2;
										}
										Bands [B++].Right = points [i + 1];
									}
								}
							break;
						}
						if (invert)
						{
							int b;
							int j = 0;
							int empty_band_count = 0;

							// First remove all empty bands from the list.
							for (b = 0; b < B; b++)
							{
								if (!BAND_EMPTY(Bands[b]))
								{
									if (b != j)
										Bands[j] = Bands[b];
									j++;
								}
								else
									empty_band_count++;
							}

							if (j > 0)
							{
								if (j == 1)
								{
									j = 0;
									// Easy case to deal with, so special case it.

									if (Bands[0].Left > 0)
									{
										pClip->Left[j][w] = 0;
										pClip->Right[j++][w] = Bands[0].Left + 1;
									}
									if (Bands[0].Right < 256)
									{
										pClip->Left[j][w] = Bands[0].Right;
										pClip->Right[j++][w] = 256;
									}
									if (j == 0)
									{
										pClip->Left[j][w] = 1;
										pClip->Right[j++][w] = 0;
									}
								}
								else
								{
									// Now sort the bands into order
									B = j;
									qsort ((void *) Bands, B,
										   sizeof (Bands [0]), BandCompare);

									// Now invert the area the bands cover
									j = 0;
									for (b = 0; b < B; b++)
									{
										if (b == 0 && Bands[b].Left > 0)
										{
											pClip->Left[j][w] = 0;
											pClip->Right[j++][w] = Bands[b].Left + 1;
										}
										else
											if (b == B - 1 && Bands[b].Right < 256)
											{
												pClip->Left[j][w] = Bands[b].Right;
												pClip->Right[j++][w] = 256;
											}
										if (b < B - 1)
										{
											pClip->Left[j][w] = Bands[b].Right;
											pClip->Right[j++][w] = Bands[b + 1].Left + 1;
										}
									}
								}
							}
							else
							{
								// Inverting a window that consisted of only
								// empty bands is the whole width of the screen.
								// Needed for Mario Kart's rear-view mirror display.
								if (empty_band_count)
								{
									pClip->Left[j][w] = 0;
									pClip->Right[j][w] = 256;
									j++;
								}
							}
							pClip->Count[w] = j;
						}
						else
						{
							for (int j = 0; j < B; j++)
							{
								pClip->Left[j][w] = Bands[j].Left;
								pClip->Right[j][w] = Bands[j].Right;
							}
							pClip->Count [w] = B;
						}
					}
					else
					{
						// Only one window enabled so no need to perform
						// complex overlap logic...

						if (Window1Enabled)
						{
							if (invert)
							{
								int j = 0;

								if (Window1Enabled == 1)
								{
									if (Win1[0].Left <= Win1[0].Right)
									{
										if (Win1[0].Left > 0)
										{
											pClip->Left[j][w] = 0;
											pClip->Right[j++][w] = Win1[0].Left;
										}
										if (Win1[0].Right < 256)
										{
											pClip->Left[j][w] = Win1[0].Right;
											pClip->Right[j++][w] = 256;
										}
										if (j == 0)
										{
											pClip->Left[j][w] = 1;
											pClip->Right[j++][w] = 0;
										}
									}
									else
									{
										pClip->Left[j][w] = 0;
										pClip->Right[j++][w] = 256;
									}
								}
								else
								{
									pClip->Left [j][w] = Win1[0].Right;
									pClip->Right[j++][w] = Win1[1].Left;
								}
								pClip->Count [w] = j;
							}
							else
							{
								for (uint32 j = 0; j < Window1Enabled; j++)
								{
									pClip->Left [j][w] = Win1[j].Left;
									pClip->Right [j][w] = Win1[j].Right;
								}
								pClip->Count [w] = Window1Enabled;
							}
						}
						else
							if (Window2Enabled)
							{
								if (invert)
								{
									int j = 0;
									if (Window2Enabled == 1)
									{
										if (Win2[0].Left <= Win2[0].Right)
										{
											if (Win2[0].Left > 0)
											{
												pClip->Left[j][w] = 0;
												pClip->Right[j++][w] = Win2[0].Left;
											}
											if (Win2[0].Right < 256)
											{
												pClip->Left[j][w] = Win2[0].Right;
												pClip->Right[j++][w] = 256;
											}
											if (j == 0)
											{
												pClip->Left[j][w] = 1;
												pClip->Right[j++][w] = 0;
											}
										}
										else
										{
											pClip->Left[j][w] = 0;
											pClip->Right[j++][w] = 256;
										}
									}
									else
									{
										pClip->Left [j][w] = Win2[0].Right;
										pClip->Right[j++][w] = Win2[1].Left + 1;
									}
									pClip->Count [w] = j;
								}
								else
								{
									for (uint32 j = 0; j < Window2Enabled; j++)
									{
										pClip->Left [j][w] = Win2[j].Left;
										pClip->Right [j][w] = Win2[j].Right;
									}
									pClip->Count [w] = Window2Enabled;
								}
							}
					}

					if (w != 5 && pClip->Count [5])
					{
						// Colour window enabled. Set the
						// clip windows for all remaining backgrounds to be
						// the same as the colour window.
						if (pClip->Count [w] == 0)
						{
							pClip->Count [w] = pClip->Count [5];
							for (uint32 i = 0; i < pClip->Count [w]; i++)
							{
								pClip->Left [i][w] = pClip->Left [i][5];
								pClip->Right [i][w] = pClip->Right [i][5];
							}
						}
						else
						{
							// Intersect the colour window with the bg's
							// own clip window.
							for (uint32 i = 0; i < pClip->Count [w]; i++)
							{
								uint32 j;
								for (j = 0; j < pClip->Count [5]; j++)
								{
									if((pClip->Left[i][w] >= pClip->Left[j][5] && pClip->Left[i][w] < pClip->Right[j][5]) || (pClip->Left[j][5] >= pClip->Left[i][w] && pClip->Left[j][5] < pClip->Right[i][w])){
										// Found an intersection!
										pClip->Left[i][w]=MAX(pClip->Left[i][w], pClip->Left[j][5]);
										pClip->Right[i][w]=MIN(pClip->Right[i][w], pClip->Right[j][5]);
										goto Clip_ok;
									}
								}
								// no intersection, nullify it
								pClip->Left[i][w]=1;
								pClip->Right[i][w]=0;
Clip_ok:
								j=0; // dummy statement
							}
						}
					}
				} // if (w == 5 | ...
			} // if (!Settings.DisableGraphicWindows)
		} // for (int w...
	} // for (int c...
}

