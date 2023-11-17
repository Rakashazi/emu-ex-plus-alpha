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

#ifndef _memmap_h_
#define _memmap_h_

#include "snes9x.h"
#include <string>

#ifdef FAST_LSB_WORD_ACCESS
#define READ_WORD(s) (*(uint16a *) (s))
//static uint16 READ_WORD(void *s) { uint16 temp; memcpy(&temp, s, 2); return temp; }
#define READ_DWORD(s) (*(uint32a *) (s))
#define WRITE_WORD(s, d) (*(uint16a *) (s)) = (d)
//static void WRITE_WORD(void *s, uint16 d) { memcpy(s, &d, 2); }
#define WRITE_DWORD(s, d) (*(uint32a *) (s)) = (d)

#define READ_3WORD(s) (0x00ffffff & *(uint32a *) (s))
#define WRITE_3WORD(s, d) *(uint16a *) (s) = (uint16)(d),\
                          *((uint8 *) (s) + 2) = (uint8) ((d) >> 16)


#else
#define READ_WORD(s) ( *(uint8 *) (s) |\
		      (*((uint8 *) (s) + 1) << 8))
#define READ_DWORD(s) ( *(uint8 *) (s) |\
		       (*((uint8 *) (s) + 1) << 8) |\
		       (*((uint8 *) (s) + 2) << 16) |\
		       (*((uint8 *) (s) + 3) << 24))
#define WRITE_WORD(s, d) *(uint8 *) (s) = (d), \
                         *((uint8 *) (s) + 1) = (d) >> 8
#define WRITE_DWORD(s, d) *(uint8 *) (s) = (uint8) (d), \
                          *((uint8 *) (s) + 1) = (uint8) ((d) >> 8),\
                          *((uint8 *) (s) + 2) = (uint8) ((d) >> 16),\
                          *((uint8 *) (s) + 3) = (uint8) ((d) >> 24)
#define WRITE_3WORD(s, d) *(uint8 *) (s) = (uint8) (d), \
                          *((uint8 *) (s) + 1) = (uint8) ((d) >> 8),\
                          *((uint8 *) (s) + 2) = (uint8) ((d) >> 16)
#define READ_3WORD(s) ( *(uint8 *) (s) |\
                       (*((uint8 *) (s) + 1) << 8) |\
                       (*((uint8 *) (s) + 2) << 16))
#endif

#define MEMMAP_BLOCK_SIZE (0x1000)
#define MEMMAP_NUM_BLOCKS (0x1000000 / MEMMAP_BLOCK_SIZE)
#define MEMMAP_BLOCKS_PER_BANK (0x10000 / MEMMAP_BLOCK_SIZE)
#define MEMMAP_SHIFT 12
#define MEMMAP_MASK (MEMMAP_BLOCK_SIZE - 1)
#define MEMMAP_MAX_SDD1_LOGGED_ENTRIES (0x10000 / 8)

//Extended ROM Formats
#define NOPE 0
#define YEAH 1
#define BIGFIRST 2
#define SMALLFIRST 3

//File Formats go here
enum file_formats { FILE_ZIP, FILE_RAR, FILE_JMA, FILE_DEFAULT };

class CMemory {
public:
    bool8 LoadROMMem (const uint8 *source, uint32 sourceSize, const char* optional_rom_filename = NULL);
    uint32 FileLoader (uint8* buffer, const char* filename, int32 maxsize);
    void  InitROM (bool8);
    bool8 LoadSRAM (const char *);
    bool8 SaveSRAM (const char *);
    bool8 Init ();
    void  Deinit ();
    void  FreeSDD1Data ();
    
    void WriteProtectROM ();
    void FixROMSpeed ();
    void MapRAM ();
    void MapExtraRAM ();
    char *Safe (const char *);
    
	void BSLoROMMap();
	void JumboLoROMMap (bool8);
    void LoROMMap ();
    void LoROM24MBSMap ();
    void SRAM512KLoROMMap ();
//    void SRAM1024KLoROMMap ();
    void SufamiTurboLoROMMap ();
    void HiROMMap ();
    void SuperFXROMMap ();
    void TalesROMMap (bool8);
    void AlphaROMMap ();
    void SA1ROMMap ();
    void BSHiROMMap ();
	void SPC7110HiROMMap();
	void SPC7110Sram(uint8);
	void SetaDSPMap();
    bool8 AllASCII (uint8 *b, int size);
    int  ScoreHiROM (bool8 skip_header, int32 offset=0);
    int  ScoreLoROM (bool8 skip_header, int32 offset=0);
#if 0
    void SufamiTurboAltROMMap();
#endif
    void ApplyROMFixes ();
    void CheckForIPSPatch (const char *rom_filename, bool8 header,
			   int32 &rom_size);
    
    const char *TVStandard ();
    const char *Speed ();
    const char *StaticRAMSize ();
    const char *MapType ();
    const char *MapMode ();
    const char *KartContents ();
    const char *Size ();
    const char *Headers ();
    const char *ROMID ();
    const char *CompanyID ();
    void ParseSNESHeader(uint8*);
	enum {
	MAP_PPU, MAP_CPU, MAP_DSP, MAP_LOROM_SRAM, MAP_HIROM_SRAM,
	MAP_NONE, MAP_DEBUG, MAP_C4, MAP_BWRAM, MAP_BWRAM_BITMAP,
	MAP_BWRAM_BITMAP2, MAP_SA1RAM, MAP_SPC7110_ROM, MAP_SPC7110_DRAM,
	MAP_RONLY_SRAM, MAP_OBC_RAM, MAP_SETA_DSP, MAP_SETA_RISC, MAP_LAST
    };
    static const uint MAX_ROM_SIZE = 0x800000;
    
    uint8 *RAM;
    uint8 *ROM;
    uint8 *VRAM;
    uint8 *SRAM;
    uint8 *BWRAM;
    uint8 *FillRAM;
    uint8 *C4RAM;
    bool8 HiROM;
    bool8 LoROM;
    uint32 SRAMMask;
    uint8 SRAMSize;
    uint8 *Map [MEMMAP_NUM_BLOCKS];
    uint8 *WriteMap [MEMMAP_NUM_BLOCKS];
    uint8 MemorySpeed [MEMMAP_NUM_BLOCKS];
    uint8 BlockIsRAM [MEMMAP_NUM_BLOCKS];
    uint8 BlockIsROM [MEMMAP_NUM_BLOCKS];
    char  ROMName [ROM_NAME_LEN];
    char  ROMId [5];
    char  CompanyId [3];
    uint8 ROMSpeed;
    uint8 ROMType;
    uint8 ROMSize;
    int32 ROMFramesPerSecond;
    int32 HeaderCount;
    uint32 CalculatedSize;
    uint32 CalculatedChecksum;
    uint32 ROMChecksum;
    uint32 ROMComplementChecksum;
    uint8  *SDD1Index;
    uint8  *SDD1Data;
    uint32 SDD1Entries;
    uint32 SDD1LoggedDataCountPrev;
    uint32 SDD1LoggedDataCount;
    uint8  SDD1LoggedData [MEMMAP_MAX_SDD1_LOGGED_ENTRIES];
    std::string ROMFilename;
	uint8 ROMRegion;
    uint32 ROMCRC32;
	uint8 ExtendedFormat;
#if 0
	bool8 SufamiTurbo;
	char Slot1Filename [_MAX_PATH];
	char Slot2Filename [_MAX_PATH];
	uint8* ROMOffset1;
	uint8* ROMOffset2;
	uint8* SRAMOffset1;
	uint8* SRAMOffset2;
	uint32 Slot1Size;
	uint32 Slot2Size;
	uint32 Slot1SRAMSize;
	uint32 Slot2SRAMSize;
	uint8 SlotContents;
#endif
	uint8 *BSRAM;
	void ResetSpeedMap();
#if 0
	bool8 LoadMulti (const char *,const char *,const char *);
#endif
};

START_EXTERN_C
extern CMemory Memory;
extern uint8 *SRAM;
extern uint8 *ROM;
extern uint8 *RegRAM;
void S9xDeinterleaveMode2 ();
bool8 LoadZip(const char* zipname,
	      int32 *TotalFileSize,
	      int32 *headers, 
              uint8 *buffer);
END_EXTERN_C

void S9xAutoSaveSRAM ();

#ifdef NO_INLINE_SET_GET
uint8 S9xGetByte (uint32 Address);
uint16 S9xGetWord (uint32 Address);
void S9xSetByte (uint8 Byte, uint32 Address);
void S9xSetWord (uint16 Byte, uint32 Address);
void S9xSetPCBase (uint32 Address);
uint8 *S9xGetMemPointer (uint32 Address);
uint8 *GetBasePointer (uint32 Address);

extern "C"{
extern uint8 OpenBus;
}
#else
#define INLINE inline
#include "getset.h"
#endif // NO_INLINE_SET_GET

#endif // _memmap_h_

