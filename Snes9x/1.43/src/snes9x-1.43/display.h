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

#ifndef _DISPLAY_H_
#define _DISPLAY_H_

START_EXTERN_C
// Routines the port specific code has to implement
void S9xSetPalette ();
void S9xTextMode ();
void S9xGraphicsMode ();
char *S9xParseArgs (char **argv, int argc);
void S9xParseArg (char **argv, int &index, int argc);
void S9xExtraUsage ();
uint32 S9xReadJoypad (int which1_0_to_4);
bool8 S9xReadMousePosition (int which1_0_to_1, int &x, int &y, uint32 &buttons);
bool8 S9xReadSuperScopePosition (int &x, int &y, uint32 &buttons);

void S9xUsage ();
void S9xInitDisplay (int argc, char **argv);
void S9xDeinitDisplay ();
void S9xInitInputDevices ();
void S9xSetTitle (const char *title);
void S9xProcessEvents (bool8 block);
void S9xPutImage (int width, int height);
void S9xParseDisplayArg (char **argv, int &index, int argc);
void S9xToggleSoundChannel (int channel);
void S9xSetInfoString (const char *string);
int S9xMinCommandLineArgs ();
void S9xNextController ();
bool8 S9xLoadROMImage (const char *string);
const char *S9xSelectFilename (const char *def, const char *dir,
			       const char *ext, const char *title);

const char *S9xChooseFilename (bool8 read_only);
bool8 S9xOpenSnapshotFile (const char *base, bool8 read_only, STREAM *file);
void S9xCloseSnapshotFile (STREAM file);

const char *S9xBasename (const char *filename) __attribute__((pure));

int S9xFStrcmp (FILE *, const char *);
const char *S9xGetHomeDirectory ();
const char *S9xGetSnapshotDirectory ();
const char *S9xGetROMDirectory ();
const char *S9xGetSRAMFilename ();
const char *S9xGetFilename (const char *extension);
const char *S9xGetFilenameInc (const char *);
END_EXTERN_C

#endif

