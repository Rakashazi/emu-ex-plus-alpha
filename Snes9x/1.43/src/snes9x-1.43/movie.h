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

  Input recording/playback code
  (c) Copyright 2004 blip
 
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
#ifndef _MOVIE_H_
#define _MOVIE_H_

#include <stdio.h>
#include <time.h>
#include "snes9x.h"

#ifndef SUCCESS
#  define SUCCESS 1
#  define WRONG_FORMAT (-1)
#  define WRONG_VERSION (-2)
#  define FILE_NOT_FOUND (-3)
#endif

#define MOVIE_OPT_FROM_SNAPSHOT 0
#define MOVIE_OPT_FROM_RESET	(1<<0)
#define MOVIE_OPT_PAL           (1<<1)
#define MOVIE_MAX_METADATA		512

START_EXTERN_C
struct MovieInfo
{
	time_t	TimeCreated;
	uint32	LengthFrames;
	uint32	RerecordCount;
	wchar_t	Metadata[MOVIE_MAX_METADATA];		// really should be wchar_t
	uint8	Opts;
	uint8	ControllersMask;
	bool8	ReadOnly;
};

// methods used by the user-interface code
int S9xMovieOpen (const char* filename, bool8 read_only);
int S9xMovieCreate (const char* filename, uint8 controllers_mask, uint8 opts, const wchar_t* metadata, int metadata_length);
int S9xMovieGetInfo (const char* filename, struct MovieInfo* info);
void S9xMovieStop (bool8 suppress_message);
void S9xMovieToggleFrameDisplay ();

// methods used by the emulation
void S9xMovieInit ();
void S9xMovieUpdate ();
//bool8 S9xMovieRewind (uint32 at_frame);
void S9xMovieFreeze (uint8** buf, uint32* size);
bool8 S9xMovieUnfreeze (const uint8* buf, uint32 size);

// accessor functions
bool8 S9xMovieActive ();
// the following accessors return 0/false if !S9xMovieActive()
bool8 S9xMovieReadOnly ();
uint32 S9xMovieGetId ();
uint32 S9xMovieGetLength ();
uint32 S9xMovieGetFrameCounter ();

END_EXTERN_C

#endif
