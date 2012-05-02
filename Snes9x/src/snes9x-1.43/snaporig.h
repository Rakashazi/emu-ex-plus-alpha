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
#ifndef _SNAPORIG_H_
#define _SNAPORIG_H_

#define ORIG_SNAPSHOT_MAGIC "#!snes96"
#define ORIG_SNAPSHOT_VERSION 4

EXTERN_C bool8 S9xLoadOrigSnapshot (const char *filename);

struct SOrigCPUState{
    uint32  Flags;
    short  Cycles_old;
    short  NextEvent_old;
    uint8   CurrentFrame;
    uint8   FastROMSpeed_old_old;
    uint16   V_Counter_old;
    bool8   BranchSkip;
    bool8   NMIActive;
    bool8   IRQActive;
    bool8   WaitingForInterrupt;
    bool8   InDMA;
    uint8   WhichEvent;
    uint8   *PC;
    uint8   *PCBase;
    uint16   MemSpeed_old;
    uint16   MemSpeedx2_old;
    uint16   FastROMSpeed_old;
    bool8   FastDP;
    uint8   *PCAtOpcodeStart;
    uint8   *WaitAddress;
    uint32  WaitCounter;
    long   Cycles;
    long   NextEvent;
    long   V_Counter;
    long   MemSpeed;
    long   MemSpeedx2;
    long   FastROMSpeed;
};

struct SOrigAPU
{
    uint32 Cycles;
    bool8  ShowROM;
    uint8  Flags;
    uint8  KeyedChannels;
    uint8  OutPorts [4];
    uint8  DSP [0x80];
    uint8  ExtraRAM [64];
    uint16  Timer [3];
    uint16  TimerTarget [3];
    bool8  TimerEnabled [3];
    bool8  TimerValueWritten [3];
};

typedef union
{
#ifdef LSB_FIRST
    struct { uint8 A, Y; } B;
#else
    struct { uint8 Y, A; } B;
#endif
    uint16 W;
} OrigYAndA;

struct SOrigAPURegisters{
    uint8  P;
    OrigYAndA YA;
    uint8  X;
    uint8  S;
    uint16  PC;
};

#define ORIG_MAX_BUFFER_SIZE (1024 * 4)
#define NUM_CHANNELS    8

typedef struct {
    int state;
    int type;
    short volume_left;
    short volume_right;
    int frequency;
    int count;
    signed short wave [ORIG_MAX_BUFFER_SIZE];
    bool8 loop;
    int envx;
    short left_vol_level;
    short right_vol_level;
    short envx_target;
    unsigned long int env_error;
    unsigned long erate;
    int direction;
    unsigned long attack_rate;
    unsigned long decay_rate;
    unsigned long sustain_rate;
    unsigned long release_rate;
    unsigned long sustain_level;
    signed short sample;
    signed short decoded [16];
    signed short previous [2];
    uint16 sample_number;
    bool8 last_block;
    bool8 needs_decode;
    uint32 block_pointer;
    uint32 sample_pointer;
    int *echo_buf_ptr;
    int mode;
    uint32 dummy [8];
} OrigChannel;

typedef struct
{
    short master_volume_left;
    short master_volume_right;
    short echo_volume_left;
    short echo_volume_right;
    int echo_enable;
    int echo_feedback;
    int echo_ptr;
    int echo_buffer_size;
    int echo_write_enabled;
    int echo_channel_enable;
    int pitch_mod;
    // Just incase they are needed in the future, for snapshot compatibility.
    uint32 dummy [3];
    OrigChannel channels [NUM_CHANNELS];
} SOrigSoundData;

struct SOrigOBJ
{
    short HPos;
    uint16  VPos;
    uint16  Name;
    uint8  VFlip;
    uint8  HFlip;
    uint8  Priority;
    uint8  Palette;
    uint8  Size;
    uint8  Prev;
    uint8  Next;
};

struct SOrigPPU {
    uint8  BGMode;
    uint8  BG3Priority;
    uint8  Brightness;

    struct {
	bool8 High;
	uint8 Increment;
	uint16 Address;
	uint16 Mask1;
	uint16 FullGraphicCount;
	uint16 Shift;
    } VMA;

    struct {
	uint8 TileSize;
	uint16 TileAddress;
	uint8 Width;
	uint8 Height;
	uint16 SCBase;
	uint16 VOffset;
	uint16 HOffset;
	bool8 ThroughMain;
	bool8 ThroughSub;
	uint8 BGSize;
	uint16 NameBase;
	uint16 SCSize;
	bool8 Addition;
    } BG [4];

    bool8 CGFLIP;
    uint16 CGDATA [256]; 
    uint8 FirstSprite;
    uint8 LastSprite;
    struct SOrigOBJ OBJ [129];
    uint8 OAMPriorityRotation;
    uint16 OAMAddr;

    uint8 OAMFlip;
    uint16 OAMTileAddress;
    uint16 IRQVBeamPos;
    uint16 IRQHBeamPos;
    uint16 VBeamPosLatched;
    uint16 HBeamPosLatched;

    uint8 HBeamFlip;
    uint8 VBeamFlip;
    uint8 HVBeamCounterLatched;

    short MatrixA;
    short MatrixB;
    short MatrixC;
    short MatrixD;
    short CentreX;
    short CentreY;
    uint8  Joypad1ButtonReadPos;
    uint8  Joypad2ButtonReadPos;

    uint8  CGADD;
    uint8  FixedColourRed;
    uint8  FixedColourGreen;
    uint8  FixedColourBlue;
    uint16  SavedOAMAddr;
    uint16  ScreenHeight;
    uint32 WRAM;
    uint8  BG_Forced;
    bool8  ForcedBlanking;
    bool8  OBJThroughMain;
    bool8  OBJThroughSub;
    uint8  OBJSizeSelect;
    uint8  OBJNameSelect_old;
    uint16  OBJNameBase;
    bool8  OBJAddition;
    uint8  OAMReadFlip;
    uint8  OAMData [512 + 32];
    bool8  VTimerEnabled;
    bool8  HTimerEnabled;
    short HTimerPosition;
    uint8  Mosaic;
    bool8  BGMosaic [4];
    bool8  Mode7HFlip;
    bool8  Mode7VFlip;
    uint8  Mode7Repeat;
    uint8  Window1Left;
    uint8  Window1Right;
    uint8  Window2Left;
    uint8  Window2Right;
    uint8  ClipCounts [6];
    uint8  ClipLeftEdges [3][6];
    uint8  ClipRightEdges [3][6];
    uint8  ClipWindowOverlapLogic [6];
    uint8  ClipWindow1Enable [6];
    uint8  ClipWindow2Enable [6];
    bool8  ClipWindow1Inside [6];
    bool8  ClipWindow2Inside [6];
    bool8  RecomputeClipWindows;
    uint8  CGFLIPRead;
    uint16  OBJNameSelect;
    bool8  Need16x8Mulitply;
    uint8  Joypad3ButtonReadPos;
    uint8  MouseSpeed[2];
};

struct SOrigDMA {
    bool8  TransferDirection;
    bool8  AAddressFixed;
    bool8  AAddressDecrement;
    uint8  TransferMode;

    uint8  ABank;
    uint16  AAddress;
    uint16  Address;
    uint8  BAddress;

    // General DMA only:
    uint16  TransferBytes;

    // H-DMA only:
    bool8  HDMAIndirectAddressing;
    uint16  IndirectAddress;
    uint8  IndirectBank;
    uint8  Repeat;
    uint8  LineCount;
    uint8  FirstLine;
    bool8  JustStarted;
};

typedef union
{
#ifdef LSB_FIRST
    struct { uint8 l,h; } B;
#else
    struct { uint8 h,l; } B;
#endif
    uint16 W;
} OrigPair;

struct SOrigRegisters{
    uint8       PB;
    uint8       DB;
    OrigPair   P;
    OrigPair   A;
    OrigPair   D;
    OrigPair   S;
    OrigPair   X;
    OrigPair   Y;
    uint16       PC;
};

#endif

