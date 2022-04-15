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

#include "ppu.h"
#include "dsp1.h"
#include "cpuexec.h"
#include "sa1.h"
#include "spc7110.h"
#include "obc1.h"
#include "seta.h"
#include <emuframework/EmuSystem.hh>

extern "C"
{
	extern uint8 OpenBus;
}

INLINE uint8 S9xGetByte (uint32 Address)
{
    int block;
    uint8 *GetAddress = Memory.Map [block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK];

	if(!CPU.InDMA)
		CPU.Cycles += Memory.MemorySpeed [block];

    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
#ifdef CPU_SHUTDOWN
		if (Memory.BlockIsRAM [block])
			CPU.WaitAddress = CPU.PCAtOpcodeStart;
#endif
		return (*(GetAddress + (Address & 0xffff)));
    }
	
    switch ((pint) GetAddress)
    {
    case CMemory::MAP_PPU:
		return (S9xGetPPU (Address & 0xffff));
    case CMemory::MAP_CPU:
		return (S9xGetCPU (Address & 0xffff));
    case CMemory::MAP_DSP:
		return (S9xGetDSP (Address & 0xffff));
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
		//Address &0x7FFF -offset into bank
		//Address&0xFF0000 -bank
		//bank>>1 | offset = s-ram address, unbound
		//unbound & SRAMMask = Sram offset
		return (*(Memory.SRAM + ((((Address&0xFF0000)>>1) |(Address&0x7FFF)) &Memory.SRAMMask)));
//		return (*(Memory.SRAM + ((Address & Memory.SRAMMask))));
		
	case CMemory::MAP_RONLY_SRAM:
    case CMemory::MAP_HIROM_SRAM:
		return (*(Memory.SRAM + (((Address & 0x7fff) - 0x6000 +
			((Address & 0xf0000) >> 3)) & Memory.SRAMMask)));
		
    case CMemory::MAP_BWRAM:
		return (*(Memory.BWRAM + ((Address & 0x7fff) - 0x6000)));
		
    case CMemory::MAP_C4:
		return (S9xGetC4 (Address & 0xffff));
		
	case CMemory::MAP_SPC7110_ROM:
#ifdef SPC7110_DEBUG
		printf("reading spc7110 ROM (byte) at %06X\n", Address);
#endif
		return S9xGetSPC7110Byte(Address);

	case CMemory::MAP_SPC7110_DRAM:
#ifdef SPC7110_DEBUG
		printf("reading Bank 50 (byte)\n");
#endif
		return S9xGetSPC7110(0x4800);

	case CMemory::MAP_OBC_RAM:
		return GetOBC1(Address & 0xffff);

	case CMemory::MAP_SETA_DSP:
		return S9xGetSetaDSP(Address);
	
	case CMemory::MAP_SETA_RISC:
		return S9xGetST018(Address);


 
     case CMemory::MAP_DEBUG:
 #ifdef DEBUGGER
 		printf ("DEBUG R(B) %06x\n", Address);
 #endif
 		return OpenBus;
 


	default:
    case CMemory::MAP_NONE:
#ifdef MK_TRACE_BAD_READS
		char address[20];
		sprintf(address, TEXT("%06X"),Address);
		MessageBox(GUI.hWnd, address, TEXT("GetByte"), MB_OK);
#endif

#ifdef DEBUGGER
		printf ("R(B) %06x\n", Address);
#endif
		return OpenBus;
    }
}

INLINE uint16 S9xGetWord (uint32 Address)
{
    if ((Address & 0x0fff) == 0x0fff)
    {
		OpenBus=S9xGetByte (Address);
		return (OpenBus | (S9xGetByte (Address + 1) << 8));
    }
    int block;
    uint8 *GetAddress = Memory.Map [block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK];

	if(!CPU.InDMA)
		CPU.Cycles += (Memory.MemorySpeed [block]<<1);

 
	if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
#ifdef CPU_SHUTDOWN
		if (Memory.BlockIsRAM [block])
			CPU.WaitAddress = CPU.PCAtOpcodeStart;
#endif
#ifdef FAST_LSB_WORD_ACCESS
		return (*(uint16 *) (GetAddress + (Address & 0xffff)));
#else
		return (*(GetAddress + (Address & 0xffff)) |
			(*(GetAddress + (Address & 0xffff) + 1) << 8));
#endif	
    }

    switch ((pint) GetAddress)
    {
    case CMemory::MAP_PPU:
		return (S9xGetPPU (Address & 0xffff) |
			(S9xGetPPU ((Address + 1) & 0xffff) << 8));
    case CMemory::MAP_CPU:
		return (S9xGetCPU (Address & 0xffff) |
			(S9xGetCPU ((Address + 1) & 0xffff) << 8));
    case CMemory::MAP_DSP:
#ifdef DSP_DUMMY_LOOPS
		printf("Get DSP Word @ %06X\n", Address);
#endif
		return (S9xGetDSP (Address & 0xffff) |
			(S9xGetDSP ((Address + 1) & 0xffff) << 8));
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
		//Address &0x7FFF -offset into bank
		//Address&0xFF0000 -bank
		//bank>>1 | offset = s-ram address, unbound
		//unbound & SRAMMask = Sram offset
		/* BJ: no FAST_LSB_WORD_ACCESS here, since if Memory.SRAMMask=0x7ff
		 * then the high byte doesn't follow the low byte. */
		return 
			(*(Memory.SRAM + ((((Address&0xFF0000)>>1) |(Address&0x7FFF)) &Memory.SRAMMask)))|
			((*(Memory.SRAM + (((((Address+1)&0xFF0000)>>1) |((Address+1)&0x7FFF)) &Memory.SRAMMask)))<<8);

		//return (*(uint16*)(Memory.SRAM + ((((Address&0xFF0000)>>1)|(Address&0x7FFF)) & Memory.SRAMMask));// |
	//		(*(Memory.SRAM + ((Address + 1) & Memory.SRAMMask)) << 8));
		
	case CMemory::MAP_RONLY_SRAM:
    case CMemory::MAP_HIROM_SRAM:
		/* BJ: no FAST_LSB_WORD_ACCESS here, since if Memory.SRAMMask=0x7ff
		 * then the high byte doesn't follow the low byte. */
		return (*(Memory.SRAM +
			(((Address & 0x7fff) - 0x6000 +
			((Address & 0xf0000) >> 3)) & Memory.SRAMMask)) |
			(*(Memory.SRAM +
			((((Address + 1) & 0x7fff) - 0x6000 +
			(((Address + 1) & 0xf0000) >> 3)) & Memory.SRAMMask)) << 8));
		
    case CMemory::MAP_BWRAM:
#ifdef FAST_LSB_WORD_ACCESS
		return (*(uint16 *) (Memory.BWRAM + ((Address & 0x7fff) - 0x6000)));
#else
		return (*(Memory.BWRAM + ((Address & 0x7fff) - 0x6000)) |
			(*(Memory.BWRAM + (((Address + 1) & 0x7fff) - 0x6000)) << 8));
#endif
		
    case CMemory::MAP_C4:
		return (S9xGetC4 (Address & 0xffff) |
			(S9xGetC4 ((Address + 1) & 0xffff) << 8));
	
	case CMemory::MAP_SPC7110_ROM:
#ifdef SPC7110_DEBUG
		printf("reading spc7110 ROM (word) at %06X\n", Address);
#endif
	return (S9xGetSPC7110Byte(Address)|
			(S9xGetSPC7110Byte (Address+1))<<8);	
	case CMemory::MAP_SPC7110_DRAM:
#ifdef SPC7110_DEBUG
		printf("reading Bank 50 (word)\n");
#endif
		return (S9xGetSPC7110(0x4800)|
			(S9xGetSPC7110 (0x4800) << 8));
	case CMemory::MAP_OBC_RAM:
		return GetOBC1(Address&0xFFFF)| (GetOBC1((Address+1)&0xFFFF)<<8);

	case CMemory::MAP_SETA_DSP:
		return S9xGetSetaDSP(Address)| (S9xGetSetaDSP((Address+1))<<8);
	
	case CMemory::MAP_SETA_RISC:
		return S9xGetST018(Address)| (S9xGetST018((Address+1))<<8);

     case CMemory::MAP_DEBUG:
 #ifdef DEBUGGER
 		printf ("DEBUG R(W) %06x\n", Address);
 #endif
 		return (OpenBus | (OpenBus<<8));


    default:
    case CMemory::MAP_NONE:
#ifdef MK_TRACE_BAD_READS
		char address[20];
		sprintf(address, TEXT("%06X"),Address);
		MessageBox(GUI.hWnd, address, TEXT("GetWord"), MB_OK);
#endif

#ifdef DEBUGGER
		printf ("R(W) %06x\n", Address);
#endif
		return (OpenBus | (OpenBus<<8));
    }
}

INLINE void S9xSetByte (uint8 Byte, uint32 Address)
{
#if defined(CPU_SHUTDOWN)
    CPU.WaitAddress = NULL;
#endif
    int block;
    uint8 *SetAddress = Memory.WriteMap [block = ((Address >> MEMMAP_SHIFT) & MEMMAP_MASK)];

	if (!CPU.InDMA)
		CPU.Cycles += Memory.MemorySpeed [block];

	
    if (SetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
#ifdef CPU_SHUTDOWN
		SetAddress += Address & 0xffff;
		if (SetAddress == SA1.WaitByteAddress1 ||
			SetAddress == SA1.WaitByteAddress2)
		{
			SA1.Executing = SA1.S9xOpcodes != NULL;
			SA1.WaitCounter = 0;
		}
		*SetAddress = Byte;
#else
		*(SetAddress + (Address & 0xffff)) = Byte;
#endif
		return;
    }
	
    switch ((pint) SetAddress)
    {
    case CMemory::MAP_PPU:
		S9xSetPPU (Byte, Address & 0xffff);
		return;
		
    case CMemory::MAP_CPU:
		S9xSetCPU (Byte, Address & 0xffff);
		return;
		
    case CMemory::MAP_DSP:
#ifdef DSP_DUMMY_LOOPS
		printf("DSP Byte: %02X to %06X\n", Byte, Address);
#endif
		S9xSetDSP (Byte, Address & 0xffff);
		return;
		
    case CMemory::MAP_LOROM_SRAM:
		if (Memory.SRAMMask)
		{
			*(Memory.SRAM + ((((Address&0xFF0000)>>1)|(Address&0x7FFF))& Memory.SRAMMask))=Byte;
//			*(Memory.SRAM + (Address & Memory.SRAMMask)) = Byte;
			EmuEx::gSystem().onBackupMemoryWritten();
		}
		return;
		
    case CMemory::MAP_HIROM_SRAM:
		if (Memory.SRAMMask)
		{
			*(Memory.SRAM + (((Address & 0x7fff) - 0x6000 +
				((Address & 0xf0000) >> 3)) & Memory.SRAMMask)) = Byte;
			EmuEx::gSystem().onBackupMemoryWritten();
		}
		return;
		
    case CMemory::MAP_BWRAM:
		*(Memory.BWRAM + ((Address & 0x7fff) - 0x6000)) = Byte;
		EmuEx::gSystem().onBackupMemoryWritten();
		return;
		
    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
		printf ("W(B) %06x\n", Address);
#endif
		
    case CMemory::MAP_SA1RAM:
		*(Memory.SRAM + (Address & 0xffff)) = Byte;
		SA1.Executing = !SA1.Waiting;
		break;
		
    case CMemory::MAP_C4:
		S9xSetC4 (Byte, Address & 0xffff);
		return;
	
	case CMemory::MAP_SPC7110_DRAM:
#ifdef SPC7110_DEBUG
		printf("Writing Byte at %06X\n", Address);
#endif
		s7r.bank50[(Address & 0xffff)]= (uint8) Byte;
		break;
	
	case CMemory::MAP_OBC_RAM:
		SetOBC1(Byte, Address &0xFFFF);
		return;
	
	case CMemory::MAP_SETA_DSP:
		S9xSetSetaDSP(Byte,Address);
		return;
	
	case CMemory::MAP_SETA_RISC:
		S9xSetST018(Byte,Address);
		return;
    default:
    case CMemory::MAP_NONE:
#ifdef MK_TRACE_BAD_WRITES
		char address[20];
		sprintf(address, TEXT("%06X"),Address);
		MessageBox(GUI.hWnd, address, TEXT("SetByte"), MB_OK);
#endif

#ifdef DEBUGGER
		printf ("W(B) %06x\n", Address);
#endif
		return;
    }
}

INLINE void S9xSetWord (uint16 Word, uint32 Address)
{
	if((Address & 0x0FFF)==0x0FFF)
	{
		S9xSetByte(Word&0x00FF, Address);
		S9xSetByte(Word>>8, Address+1);
		return;
	}

#if defined(CPU_SHUTDOWN)
    CPU.WaitAddress = NULL;
#endif
    int block;
    uint8 *SetAddress = Memory.WriteMap [block = ((Address >> MEMMAP_SHIFT) & MEMMAP_MASK)];

	if (!CPU.InDMA)
		CPU.Cycles += Memory.MemorySpeed [block] << 1;


    if (SetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
#ifdef CPU_SHUTDOWN
		SetAddress += Address & 0xffff;
		if (SetAddress == SA1.WaitByteAddress1 ||
			SetAddress == SA1.WaitByteAddress2)
		{
			SA1.Executing = SA1.S9xOpcodes != NULL;
			SA1.WaitCounter = 0;
		}
#ifdef FAST_LSB_WORD_ACCESS
		*(uint16 *) SetAddress = Word;
#else
		*SetAddress = (uint8) Word;
		*(SetAddress + 1) = Word >> 8;
#endif
#else
#ifdef FAST_LSB_WORD_ACCESS
		*(uint16 *) (SetAddress + (Address & 0xffff)) = Word;
#else
		*(SetAddress + (Address & 0xffff)) = (uint8) Word;
		*(SetAddress + ((Address + 1) & 0xffff)) = Word >> 8;
#endif
#endif
		return;
    }
	
    switch ((pint) SetAddress)
    {
    case CMemory::MAP_PPU:
		S9xSetPPU ((uint8) Word, Address & 0xffff);
		S9xSetPPU (Word >> 8, (Address & 0xffff) + 1);
		return;
		
    case CMemory::MAP_CPU:
		S9xSetCPU ((uint8) Word, (Address & 0xffff));
		S9xSetCPU (Word >> 8, (Address & 0xffff) + 1);
		return;
		
    case CMemory::MAP_DSP:
#ifdef DSP_DUMMY_LOOPS
		printf("DSP Word: %04X to %06X\n", Word, Address);
#endif
		S9xSetDSP ((uint8) Word, (Address & 0xffff));
		S9xSetDSP (Word >> 8, (Address & 0xffff) + 1);
		return;
		
    case CMemory::MAP_LOROM_SRAM:
		if (Memory.SRAMMask)
		{
			/* BJ: no FAST_LSB_WORD_ACCESS here, since if Memory.SRAMMask=0x7ff
			 * then the high byte doesn't follow the low byte. */
			*(Memory.SRAM + ((((Address&0xFF0000)>>1)|(Address&0x7FFF))& Memory.SRAMMask)) = (uint8) Word;
			*(Memory.SRAM + (((((Address+1)&0xFF0000)>>1)|((Address+1)&0x7FFF))& Memory.SRAMMask)) = Word >> 8;

//			*(Memory.SRAM + (Address & Memory.SRAMMask)) = (uint8) Word;
//			*(Memory.SRAM + ((Address + 1) & Memory.SRAMMask)) = Word >> 8;
			EmuEx::gSystem().onBackupMemoryWritten();
		}
		return;
		
    case CMemory::MAP_HIROM_SRAM:
		if (Memory.SRAMMask)
		{
			/* BJ: no FAST_LSB_WORD_ACCESS here, since if Memory.SRAMMask=0x7ff
			 * then the high byte doesn't follow the low byte. */
			*(Memory.SRAM + 
				((((Address & 0x7fff) - 0x6000 +
				((Address & 0xf0000) >> 3)) & Memory.SRAMMask))) = (uint8) Word;
			*(Memory.SRAM + 
				(((((Address + 1) & 0x7fff) - 0x6000 +
				(((Address + 1) & 0xf0000) >> 3)) & Memory.SRAMMask))) = (uint8) (Word >> 8);
			EmuEx::gSystem().onBackupMemoryWritten();
		}
		return;
		
    case CMemory::MAP_BWRAM:
#ifdef FAST_LSB_WORD_ACCESS
		*(uint16 *) (Memory.BWRAM + ((Address & 0x7fff) - 0x6000)) = Word;
#else
		*(Memory.BWRAM + ((Address & 0x7fff) - 0x6000)) = (uint8) Word;
		*(Memory.BWRAM + (((Address + 1) & 0x7fff) - 0x6000)) = (uint8) (Word >> 8);
#endif
		EmuEx::gSystem().onBackupMemoryWritten();
		return;
		
    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
		printf ("W(W) %06x\n", Address);
#endif
	
	case CMemory::MAP_SPC7110_DRAM:
#ifdef SPC7110_DEBUG
		printf("Writing Word at %06X\n", Address);
#endif
		s7r.bank50[(Address & 0xffff)]= (uint8) Word;
		s7r.bank50[((Address + 1) & 0xffff)]= (uint8) Word;
		break;
    case CMemory::MAP_SA1RAM:
		*(Memory.SRAM + (Address & 0xffff)) = (uint8) Word;
		*(Memory.SRAM + ((Address + 1) & 0xffff)) = (uint8) (Word >> 8);
		SA1.Executing = !SA1.Waiting;
		break;
		
    case CMemory::MAP_C4:
		S9xSetC4 (Word & 0xff, Address & 0xffff);
		S9xSetC4 ((uint8) (Word >> 8), (Address + 1) & 0xffff);
		return;

	case CMemory::MAP_OBC_RAM:
		SetOBC1(Word & 0xff, Address &0xFFFF);
		SetOBC1 ((uint8) (Word >> 8), (Address + 1) & 0xffff);
		return;
	
	case CMemory::MAP_SETA_DSP:
		S9xSetSetaDSP (Word & 0xff, Address);
		S9xSetSetaDSP ((uint8) (Word >> 8),(Address + 1));
		return;
	
	case CMemory::MAP_SETA_RISC:
		S9xSetST018 (Word & 0xff, Address);
		S9xSetST018 ((uint8) (Word >> 8),(Address + 1));
		return;

    default:
    case CMemory::MAP_NONE:
#ifdef MK_TRACE_BAD_WRITES
		char address[20];
		sprintf(address, TEXT("%06X"),Address);
		MessageBox(GUI.hWnd, address, TEXT("SetWord"), MB_OK);
#endif

#ifdef DEBUGGER
		printf ("W(W) %06x\n", Address);
#endif
		return;
    }
}

INLINE uint8 *GetBasePointer (uint32 Address)
{
    uint8 *GetAddress = Memory.Map [(Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
		return (GetAddress);
	if(Settings.SPC7110&&((Address&0x7FFFFF)==0x4800))
	{
		return s7r.bank50;
	}
    switch ((pint) GetAddress)
    {
	case CMemory::MAP_SPC7110_DRAM:
#ifdef SPC7110_DEBUG
		printf("Getting Base pointer to DRAM\n");
#endif
		{
			return s7r.bank50;
		}
	case CMemory::MAP_SPC7110_ROM:
#ifdef SPC7110_DEBUG
		printf("Getting Base pointer to SPC7110ROM\n");
#endif
		return Get7110BasePtr(Address);
    case CMemory::MAP_PPU:
//just a guess, but it looks like this should match the CPU as a source.
		return (Memory.FillRAM);
//		return (Memory.FillRAM - 0x2000);
    case CMemory::MAP_CPU:
//fixes Ogre Battle's green lines
		return (Memory.FillRAM);
//		return (Memory.FillRAM - 0x4000);
    case CMemory::MAP_DSP:
		return (Memory.FillRAM - 0x6000);
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
		return (Memory.SRAM);
    case CMemory::MAP_BWRAM:
		return (Memory.BWRAM - 0x6000);
    case CMemory::MAP_HIROM_SRAM:
		return (Memory.SRAM - 0x6000);
    case CMemory::MAP_C4:
		return (Memory.C4RAM - 0x6000);
	case CMemory::MAP_OBC_RAM:
		return GetBasePointerOBC1(Address);
	case CMemory::MAP_SETA_DSP:
		return Memory.SRAM;
    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
		printf ("GBP %06x\n", Address);
#endif
		
    default:
    case CMemory::MAP_NONE:
#if defined(MK_TRACE_BAD_READS) || defined(MK_TRACE_BAD_WRITES)
		char fsd[12];
		sprintf(fsd, TEXT("%06X"), Address);
		MessageBox(GUI.hWnd, fsd, TEXT("Rogue DMA"), MB_OK);
#endif

#ifdef DEBUGGER
		printf ("GBP %06x\n", Address);
#endif
		return (0);
    }
}

INLINE uint8 *S9xGetMemPointer (uint32 Address)
{
    uint8 *GetAddress = Memory.Map [(Address >> MEMMAP_SHIFT) & MEMMAP_MASK];
    if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
		return (GetAddress + (Address & 0xffff));
	
	if(Settings.SPC7110&&((Address&0x7FFFFF)==0x4800))
		return s7r.bank50;

    switch ((pint) GetAddress)
    {
	case CMemory::MAP_SPC7110_DRAM:
#ifdef SPC7110_DEBUG
		printf("Getting Base pointer to DRAM\n");
#endif
		return &s7r.bank50[Address&0x0000FFFF];
    case CMemory::MAP_PPU:
		return (Memory.FillRAM + (Address & 0xffff));
    case CMemory::MAP_CPU:
		return (Memory.FillRAM + (Address & 0xffff));
    case CMemory::MAP_DSP:
		return (Memory.FillRAM - 0x6000 + (Address & 0xffff));
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
		return (Memory.SRAM + (Address & 0xffff));
    case CMemory::MAP_BWRAM:
		return (Memory.BWRAM - 0x6000 + (Address & 0xffff));
    case CMemory::MAP_HIROM_SRAM:
		return (Memory.SRAM - 0x6000 + (Address & 0xffff));
    case CMemory::MAP_C4:
		return (Memory.C4RAM - 0x6000 + (Address & 0xffff));
	case CMemory::MAP_OBC_RAM:
		return GetMemPointerOBC1(Address);
	case CMemory::MAP_SETA_DSP:
		return Memory.SRAM+ ((Address & 0xffff) & Memory.SRAMMask);
    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
		printf ("GMP %06x\n", Address);
#endif
    default:
    case CMemory::MAP_NONE:
#if defined(MK_TRACE_BAD_READS) || defined(MK_TRACE_BAD_WRITES)
		char fsd[12];
		sprintf(fsd, TEXT("%06X"), Address);
		MessageBox(GUI.hWnd, fsd, TEXT("Rogue DMA"), MB_OK);
#endif

#ifdef DEBUGGER
		printf ("GMP %06x\n", Address);
#endif
		return (0);
    }
}

INLINE void S9xSetPCBase (uint32 Address)
{
    int block;
    uint8 *GetAddress = Memory.Map [block = (Address >> MEMMAP_SHIFT) & MEMMAP_MASK];

	CPU.MemSpeed = Memory.MemorySpeed [block];
	CPU.MemSpeedx2 = CPU.MemSpeed << 1;
 
   if (GetAddress >= (uint8 *) CMemory::MAP_LAST)
    {
		CPU.PCBase = GetAddress;
		CPU.PC = GetAddress + (Address & 0xffff);
		return;
    }
	
    switch ((pint) GetAddress)
    {
    case CMemory::MAP_PPU:
		CPU.PCBase = Memory.FillRAM;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_CPU:
		CPU.PCBase = Memory.FillRAM;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_DSP:
		CPU.PCBase = Memory.FillRAM - 0x6000;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_SA1RAM:
    case CMemory::MAP_LOROM_SRAM:
		CPU.PCBase = Memory.SRAM;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_BWRAM:
		CPU.PCBase = Memory.BWRAM - 0x6000;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
    case CMemory::MAP_HIROM_SRAM:
		CPU.PCBase = Memory.SRAM - 0x6000;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_C4:
		CPU.PCBase = Memory.C4RAM - 0x6000;
		CPU.PC = CPU.PCBase + (Address & 0xffff);
		return;
		
    case CMemory::MAP_DEBUG:
#ifdef DEBUGGER
		printf ("SBP %06x\n", Address);
#endif
		
    default:
    case CMemory::MAP_NONE:
#ifdef DEBUGGER
		printf ("SBP %06x\n", Address);
#endif
		CPU.PCBase = Memory.SRAM;
		CPU.PC = Memory.SRAM + (Address & 0xffff);
		return;
    }
}
