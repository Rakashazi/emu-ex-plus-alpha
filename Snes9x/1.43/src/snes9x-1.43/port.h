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

#pragma once

#include <limits.h>

#ifndef STORM
#include <memory.h>
#include <string.h>
#else
#include <strings.h>
#include <clib/powerpc_protos.h>
#endif

#ifndef ACCEPT_SIZE_T
#ifdef __WIN32__
#define ACCEPT_SIZE_T int
#else
#define ACCEPT_SIZE_T unsigned int
#endif
#endif

#include <sys/types.h>

/* #define PIXEL_FORMAT RGB565 */
//#define GFX_MULTI_FORMAT

#if 0 //defined(TARGET_OS_MAC) && TARGET_OS_MAC

#include "zlib.h"
#define ZLIB
#define EXECUTE_SUPERFX_PER_LINE
#define SOUND
#define VAR_CYCLES
#define CPU_SHUTDOWN
#define SPC700_SHUTDOWN
#define PIXEL_FORMAT RGB555
#define CHECK_SOUND()
#define M_PI 3.14159265359
#undef  _MAX_PATH

#undef DEBUGGER /* Apple Universal Headers sometimes #define DEBUGGER */
#undef GFX_MULTI_FORMAT

int    strncasecmp(const char *s1, const char *s2, unsigned n);
int    strcasecmp(const char *s1, const char *s2 );

#endif /* TARGET_OS_MAC */

#ifndef NOASM
#define USE_X86_ASM
#endif

#ifndef snes9x_types_defined
#define snes9x_types_defined

typedef unsigned char bool8;

/* FIXME: Refactor this by moving out the BORLAND part and unifying typedefs */
#ifndef __WIN32__
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef signed char int8;
typedef short int16;
typedef int int32;
typedef unsigned int uint32;
# ifdef __GNUC__  /* long long is not part of ISO C++ */
__extension__
# endif
#include <imagine/util/ansiTypes.h>
#else /* __WIN32__ */

# ifdef __BORLANDC__
#   include <systypes.h>
# else

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef signed char int8;
typedef short int16;

# ifndef WSAAPI
/* winsock2.h typedefs int32 as well. */
typedef long int32;

#   define PLAT_SOUND_BUFFER SoundBuffer
#   define RIGHTSHIFT_IS_SAR
# endif

typedef unsigned int uint32;

# endif /* __BORLANDC__ */

typedef __int64 int64;

#endif /* __WIN32__ */
#endif /* snes9x_types_defined */


#include "pixform.h"

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef STORM
#define EXTERN_C
#define START_EXTERN_C
#define END_EXTERN_C
#else
#if defined(__cplusplus) || defined(c_plusplus)
#define EXTERN_C extern "C"
#define START_EXTERN_C extern "C" {
#define END_EXTERN_C }
#else
#define EXTERN_C extern
#define START_EXTERN_C
#define END_EXTERN_C
#endif
#endif

#ifndef __WIN32__

#ifndef PATH_MAX
#define PATH_MAX 1024
#endif

#define _MAX_DIR PATH_MAX
#define _MAX_DRIVE 1
#define _MAX_FNAME PATH_MAX
#define _MAX_EXT PATH_MAX
#define _MAX_PATH PATH_MAX

#define ZeroMemory(a,b) memset((a),0,(b))

void _makepath (char *path, const char *drive, const char *dir,
		const char *fname, const char *ext);
void _splitpath (const char *path, char *drive, char *dir, char *fname,
		 char *ext);
#else /* __WIN32__ */
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

EXTERN_C void S9xGenerateSound ();

#ifdef STORM
EXTERN_C int soundsignal;
EXTERN_C void MixSound(void);
/* Yes, CHECK_SOUND is getting defined correctly! */
#define CHECK_SOUND if (Settings.APUEnabled) if(SetSignalPPC(0L, soundsignal) & soundsignal) MixSound
#else
#define CHECK_SOUND()
#endif

#ifdef __DJGPP
#define SLASH_STR "\\"
#define SLASH_CHAR '\\'
#else
#define SLASH_STR "/"
#define SLASH_CHAR '/'
#endif

/* Taken care of in signal.h on Linux.
 * #ifdef __linux
 * typedef void (*SignalHandler)(int);
 * #define SIG_PF SignalHandler
 * #endif
 */

/* If including signal.h, do it before snes9.h and port.h to avoid clashes. */
#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif

#define LSB_FIRST
#define FAST_LSB_WORD_ACCESS

#ifdef __sun
#define TITLE "Snes9X: Solaris"
#endif

#ifdef __linux
#define TITLE "Snes9X: Linux"
#endif

#ifndef TITLE
#define TITLE "Snes9x"
#endif

#ifdef STORM
#define STATIC
#define strncasecmp strnicmp
#else
#define STATIC static
#endif
