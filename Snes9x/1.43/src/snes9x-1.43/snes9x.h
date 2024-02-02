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
#ifndef _SNES9X_H_
#define _SNES9X_H_

#define VERSION "1.43"

#include <stdio.h>
#include <stdlib.h>

#ifdef __WIN32__
#include "..\wsnes9x.h"
#include "..\zlib\zlib.h"
#endif

#include "port.h"
#include "65c816.h"
#include "messages.h"

#if defined(USE_GLIDE) && !defined(GFX_MULTI_FORMAT)
#define GFX_MULTI_FORMAT
#endif

#define ROM_NAME_LEN 23

#ifdef ZLIB
#ifndef __WIN32__
#include "zlib.h"
#endif
#define STREAM gzFile
#define READ_STREAM(p,l,s) gzread (s,p,l)
#define WRITE_STREAM(p,l,s) gzwrite (s,p,l)
#define OPEN_STREAM(f,m) gzopen (f,m)
#define REOPEN_STREAM(f,m) gzdopen (f,m)
#define FIND_STREAM(f)	gztell(f)
#define REVERT_STREAM(f,o,s)  gzseek(f,o,s)
#define CLOSE_STREAM(s) gzclose (s)
#else
#define STREAM FILE *
#define READ_STREAM(p,l,s) fread (p,1,l,s)
#define WRITE_STREAM(p,l,s) fwrite (p,1,l,s)
#define OPEN_STREAM(f,m) fopen (f,m)
#define REOPEN_STREAM(f,m) fdopen (f,m)
#define FIND_STREAM(f)	ftell(f)
#define REVERT_STREAM(f,o,s)	 fseek(f,o,s)
#define CLOSE_STREAM(s) fclose (s)
#endif

FILE *fopenHelper(const char *filename, const char *mode);
void removeFileHelper(const char *filename);

/* SNES screen width and height */
#define SNES_WIDTH		256
#define SNES_HEIGHT		224
#define SNES_HEIGHT_EXTENDED	239
#define IMAGE_WIDTH		(Settings.SupportHiRes ? SNES_WIDTH * 2 : SNES_WIDTH)
#define IMAGE_HEIGHT		(Settings.SupportHiRes ? SNES_HEIGHT_EXTENDED * 2 : SNES_HEIGHT_EXTENDED)

#define SNES_MAX_NTSC_VCOUNTER  262
#define SNES_MAX_PAL_VCOUNTER   312
#define SNES_HCOUNTER_MAX	342
#define SPC700_TO_65C816_RATIO	2
#define AUTO_FRAMERATE		200

/* NTSC master clock signal 21.47727MHz
 * PPU: master clock / 4
 * 1 / PPU clock * 342 -> 63.695us
 * 63.695us / (1 / 3.579545MHz) -> 228 cycles per scanline
 * From Earth Worm Jim: APU executes an average of 65.14285714 cycles per
 * scanline giving an APU clock speed of 1.022731096MHz                    */

/* PAL master clock signal 21.28137MHz
 * PPU: master clock / 4
 * 1 / PPU clock * 342 -> 64.281us
 * 64.281us / (1 / 3.546895MHz) -> 228 cycles per scanline.  */

#define SNES_SCANLINE_TIME (63.695e-6)
#define SNES_CLOCK_SPEED (3579545)

#define SNES_CLOCK_LEN (1.0 / SNES_CLOCK_SPEED)

#define SNES_CYCLES_PER_SCANLINE ((uint32) ((SNES_SCANLINE_TIME / SNES_CLOCK_LEN) * 6 + 0.5))

#define SNES_APUTIMER2_CYCLEx10000 ((uint32) ((SNES_CYCLES_PER_SCANLINE * 10000L) * (1.0 / 64000.0) / SNES_SCANLINE_TIME + 0.5))

#define ONE_CYCLE 6
#define SLOW_ONE_CYCLE 8
#define TWO_CYCLES 12


#define SNES_TR_MASK	    (1 << 4)
#define SNES_TL_MASK	    (1 << 5)
#define SNES_X_MASK	    (1 << 6)
#define SNES_A_MASK	    (1 << 7)
#define SNES_RIGHT_MASK	    (1 << 8)
#define SNES_LEFT_MASK	    (1 << 9)
#define SNES_DOWN_MASK	    (1 << 10)
#define SNES_UP_MASK	    (1 << 11)
#define SNES_START_MASK	    (1 << 12)
#define SNES_SELECT_MASK    (1 << 13)
#define SNES_Y_MASK	    (1 << 14)
#define SNES_B_MASK	    (1 << 15)

enum {
    SNES_MULTIPLAYER5,
    SNES_JOYPAD,
    SNES_MOUSE_SWAPPED,
    SNES_MOUSE,
    SNES_SUPERSCOPE,
	SNES_JUSTIFIER,
	SNES_JUSTIFIER_2,
    SNES_MAX_CONTROLLER_OPTIONS
};

#define DEBUG_MODE_FLAG	    (1 << 0)
#define TRACE_FLAG			(1 << 1)
#define SINGLE_STEP_FLAG    (1 << 2)
#define BREAK_FLAG			(1 << 3)
#define SCAN_KEYS_FLAG	    (1 << 4)
#define SAVE_SNAPSHOT_FLAG  (1 << 5)
#define DELAYED_NMI_FLAG    (1 << 6)
#define NMI_FLAG			(1 << 7)
#define PROCESS_SOUND_FLAG  (1 << 8)
#define FRAME_ADVANCE_FLAG  (1 << 9)
#define DELAYED_NMI_FLAG2   (1 << 10)
#define IRQ_PENDING_FLAG    (1 << 11)

struct SCPUState{
    uint32  Flags;
    bool8   BranchSkip;
    bool8   NMIActive;
    bool8   IRQActive;
    bool8   WaitingForInterrupt;
    bool8   InDMA;
    uint8   WhichEvent;
    uint8   *PC;
    uint8   *PCBase;
    uint8   *PCAtOpcodeStart;
    uint8   *WaitAddress;
    uint32  WaitCounter;
    long   Cycles;
    long   NextEvent;
    long   V_Counter;
    long   MemSpeed;
    long   MemSpeedx2;
    long   FastROMSpeed;
    uint32 NMITriggerPoint;
    bool8  BRKTriggered;
    bool8  TriedInterleavedMode2;
    uint32 NMICycleCount;
    uint32 IRQCycleCount;
#ifdef DEBUG_MAXCOUNT
    unsigned long GlobalLoopCount;
#endif
};

#define HBLANK_START_EVENT 0
#define HBLANK_END_EVENT 1
#define HTIMER_BEFORE_EVENT 2
#define HTIMER_AFTER_EVENT 3
#define NO_EVENT 4

struct SSettings{
    /* CPU options */
    bool8  APUEnabled = 0;
    bool8  Shutdown = 0;
    static const uint8  SoundSkipMethod = 0;
    int   H_Max = (int)SNES_CYCLES_PER_SCANLINE;
    int   HBlankStart = (256 * (int)SNES_CYCLES_PER_SCANLINE) / SNES_HCOUNTER_MAX;
    static const int   CyclesPercentage = 100;
    static const bool8  DisableIRQ = 0;

		#ifdef NETPLAY_SUPPORT
    bool8  Paused;
    bool8  ForcedPause;
    bool8  StopEmulation;
		#endif

		#ifdef DEBUGGER
    /* Tracing options */
    bool8  TraceDMA;
    bool8  TraceHDMA;
    bool8  TraceVRAM;
    bool8  TraceUnknownRegisters;
    bool8  TraceDSP;
		#endif

    /* Joystick options */
    static const bool8  SwapJoypads = 0;
    //bool8  JoystickEnabled;

    /* ROM timing options (see also H_Max above) */
    bool8  ForcePAL;
    bool8  ForceNTSC;
    bool8  PAL;
    bool8  IdentifyAsPAL;
    static const uint32 FrameTimePAL = 20000;
    static const uint32 FrameTimeNTSC = 16667;
    uint32 FrameTime = FrameTimeNTSC;
    //uint32 SkipFrames;

    /* ROM image options */
    bool8  ForceLoROM;
    bool8  ForceHiROM;
    bool8  ForceHeader;
    bool8  ForceNoHeader;
    bool8  ForceInterleaved;
    bool8  ForceInterleaved2;
    bool8  ForceNotInterleaved;

    /* Peripherial options */
    bool8  ForceSuperFX;
    bool8  ForceNoSuperFX;
    bool8  ForceDSP1;
    bool8  ForceNoDSP1;
    bool8  ForceSA1;
    bool8  ForceNoSA1;
    bool8  ForceC4;
    bool8  ForceNoC4;
    bool8  ForceSDD1;
    bool8  ForceNoSDD1;
    bool8  MultiPlayer5;
    bool8  Mouse;
    bool8  SuperScope;
    bool8  SRTC;
    uint32 ControllerOption;
    
    static const bool8  ShutdownMaster = 1;
    bool8  MultiPlayer5Master;
    bool8  SuperScopeMaster;
    bool8  MouseMaster;
    bool8  SuperFX;
    bool8  DSP1Master;
    bool8  SA1;
    bool8  C4;
    bool8  SDD1;
	bool8  SPC7110;
	bool8  SPC7110RTC;
	bool8  OBC1;
    /* Sound options */
    int SoundPlaybackRate = 44100;
		#ifdef DEBUGGER
    bool8  TraceSoundDSP;
		#endif
    static const bool8  Stereo = 1;
    static const bool8  ReverseStereo = 0;
    static const bool8  SixteenBitSound = 1;
    //int    SoundBufferSize;
    //int    SoundMixInterval;
    static const bool8  SoundEnvelopeHeightReading = 0;
    bool8  DisableSoundEcho;
    bool8  DisableSampleCaching;
    static const bool8  DisableMasterVolume = 0;
    static const bool8  SoundSync = 0;
    static const bool8  InterpolatedSound = 1;
    //bool8  ThreadSound;
    //bool8  Mute;
    static const bool8  NextAPUEnabled = 1;
    static const uint8  AltSampleDecode = 0;
    static const bool8  FixFrequency = 1;
    
    /* Graphics options */
    static const bool8  SixteenBit = 1;
    static const bool8  Transparency = 1;
    static const bool8  SupportHiRes = 1;
    static const bool8  Mode7Interpolate = 0;

    /* SNES graphics options */
    bool8  BGLayering;
    bool8  DisableGraphicWindows;
    //bool8  ForceTransparency;
    //bool8  ForceNoTransparency;
    bool8  DisableHDMA;
    //bool8  DisplayFrameRate;
    //bool8  DisableRangeTimeOver; /* XXX: unused */

    /* Others */
		#ifdef NETPLAY_SUPPORT
    bool8  NetPlay;
    bool8  NetPlayServer;
    char   ServerName [128];
    int    Port;
		#endif
    //bool8  GlideEnable;
    static const bool8  OpenGLEnable = 1;
    static const int32  AutoSaveDelay = 30; /* Time in seconds before S-RAM auto-saved if modified. */
    static const bool8  ApplyCheats = true;
    //bool8  TurboMode;
    //uint32 TurboSkipFrames;
    //uint32 AutoMaxSkipFrames;
    
/* Fixes for individual games */
    bool8  StarfoxHack;
    bool8  WinterGold;
    bool8  BS;	/* Japanese Satellite System games. */
    bool8  DaffyDuck;
    //uint8  APURAMInitialValue;
    //bool8  SampleCatchup;
    static const bool8  JustifierMaster = 1;
	bool8  Justifier;
	bool8  SecondJustifier;
	int8   SETA;
    //bool8  TakeScreenshot;
    //int8   StretchScreenshots;
	uint16 DisplayColor;
    //int    SoundDriver;
    int    AIDOShmId;
    bool8  SDD1Pack;
	bool8  NoPatch;
	bool8  ForceInterleaveGD24;
#ifdef DEBUG_MAXCOUNT
    unsigned int MaxCount;
#endif
};

struct SSNESGameFixes
{
    uint8 alienVSpredetorFix;
    uint8 APU_OutPorts_ReturnValueFix;
    uint8 SoundEnvelopeHeightReading2;
    uint8 SRAMInitialValue;
	uint8 Uniracers;
	bool8 EchoOnlyOutput;
};

START_EXTERN_C
extern struct SSettings Settings;
extern struct SCPUState CPU;
extern struct SSNESGameFixes SNESGameFixes;
extern char String [513];

void S9xExit ();
void S9xMessage (int type, int number, const char *message);
void S9xLoadSDD1Data ();
END_EXTERN_C

enum {
    PAUSE_NETPLAY_CONNECT = (1 << 0),
    PAUSE_TOGGLE_FULL_SCREEN = (1 << 1),
    PAUSE_EXIT = (1 << 2),
    PAUSE_MENU = (1 << 3),
    PAUSE_INACTIVE_WINDOW = (1 << 4),
    PAUSE_WINDOW_ICONISED = (1 << 5),
    PAUSE_RESTORE_GUI = (1 << 6),
    PAUSE_FREEZE_FILE = (1 << 7)
};
void S9xSetPause (uint32 mask);
void S9xClearPause (uint32 mask);

#endif

