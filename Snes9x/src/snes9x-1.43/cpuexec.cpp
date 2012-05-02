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
#include "cpuops.h"
#include "ppu.h"
#include "cpuexec.h"
#include "debug.h"
#include "snapshot.h"
#include "gfx.h"
#include "missing.h"
#include "apu.h"
#include "dma.h"
#include "fxemu.h"
#include "sa1.h"
#include "spc7110.h"

void S9xMainLoop (void)
{
    for (;;)
    {
#ifdef DEBUG_MAXCOUNT
      CPU.GlobalLoopCount++;
      if(Settings.MaxCount && CPU.GlobalLoopCount == Settings.MaxCount) {
        fprintf(stderr, "Max loop count reached: %ld \n", Settings.MaxCount);
        S9xExit();
      }
#endif

	APU_EXECUTE ();
	if (CPU.Flags)
	{
	    if (CPU.Flags & NMI_FLAG)
	    {
		if (--CPU.NMICycleCount == 0)
		{
		    CPU.Flags &= ~NMI_FLAG;
		    if (CPU.WaitingForInterrupt)
		    {
			CPU.WaitingForInterrupt = FALSE;
			CPU.PC++;
		    }
		    S9xOpcode_NMI ();
		}
	    }

#ifdef DEBUGGER
	    if ((CPU.Flags & BREAK_FLAG) &&
		!(CPU.Flags & SINGLE_STEP_FLAG))
	    {
		for (int Break = 0; Break != 6; Break++)
		{
		    if (S9xBreakpoint[Break].Enabled &&
			S9xBreakpoint[Break].Bank == Registers.PB &&
			S9xBreakpoint[Break].Address == CPU.PC - CPU.PCBase)
		    {
			if (S9xBreakpoint[Break].Enabled == 2)
			    S9xBreakpoint[Break].Enabled = TRUE;
			else
			    CPU.Flags |= DEBUG_MODE_FLAG;
		    }
		}
	    }
#endif
	    CHECK_SOUND ();

	    if (CPU.Flags & IRQ_PENDING_FLAG)
	    {
		if (CPU.IRQCycleCount == 0)
		{
		    if (CPU.WaitingForInterrupt)
		    {
			CPU.WaitingForInterrupt = FALSE;
			CPU.PC++;
		    }
		    if (CPU.IRQActive && !Settings.DisableIRQ)
		    {
			if (!CheckFlag (IRQ))
				S9xOpcode_IRQ ();
		    }
		    else
			CPU.Flags &= ~IRQ_PENDING_FLAG;
		}
		else
                {
                    if(--CPU.IRQCycleCount==0 && CheckFlag (IRQ))
                        CPU.IRQCycleCount=1;
                }
	    }
#ifdef DEBUGGER
	    if (CPU.Flags & DEBUG_MODE_FLAG)
		break;
#endif
	    if (CPU.Flags & SCAN_KEYS_FLAG)
		break;
#ifdef DEBUGGER
	    if (CPU.Flags & TRACE_FLAG)
		S9xTrace ();

	    if (CPU.Flags & SINGLE_STEP_FLAG)
	    {
		CPU.Flags &= ~SINGLE_STEP_FLAG;
		CPU.Flags |= DEBUG_MODE_FLAG;
	    }
#endif
	}
#ifdef CPU_SHUTDOWN
	CPU.PCAtOpcodeStart = CPU.PC;
#endif
	CPU.Cycles += CPU.MemSpeed;

	(*ICPU.S9xOpcodes [*CPU.PC++].S9xOpcode) ();
	
	S9xUpdateAPUTimer();
	
	if (SA1.Executing)
	    S9xSA1MainLoop ();
	DO_HBLANK_CHECK();
    }
    Registers.PC = CPU.PC - CPU.PCBase;
    S9xPackStatus ();
    APURegisters.PC = IAPU.PC - IAPU.RAM;
    S9xAPUPackStatus ();
    if (CPU.Flags & SCAN_KEYS_FLAG)
    {
#ifdef DEBUGGER
	if (!(CPU.Flags & FRAME_ADVANCE_FLAG))
#endif
	    //S9xSyncSpeed ();
	CPU.Flags &= ~SCAN_KEYS_FLAG;
    }
#ifdef DETECT_NASTY_FX_INTERLEAVE
    if (CPU.BRKTriggered && Settings.SuperFX && !CPU.TriedInterleavedMode2)
    {
	CPU.TriedInterleavedMode2 = TRUE;
	CPU.BRKTriggered = FALSE;
	S9xDeinterleaveMode2 ();
    }
#endif
}

void S9xSetIRQ (uint32 source)
{
    CPU.IRQActive |= source;
    CPU.Flags |= IRQ_PENDING_FLAG;
    CPU.IRQCycleCount = 3;
    if (CPU.WaitingForInterrupt)
    {
	// Force IRQ to trigger immediately after WAI - 
	// Final Fantasy Mystic Quest crashes without this.
	CPU.IRQCycleCount = 0;
	CPU.WaitingForInterrupt = FALSE;
	CPU.PC++;
    }
}

void S9xClearIRQ (uint32 source)
{
    CLEAR_IRQ_SOURCE (source);
}

void S9xDoHBlankProcessing ()
{
#ifdef CPU_SHUTDOWN
    CPU.WaitCounter++;
#endif
    switch (CPU.WhichEvent)
    {
    case HBLANK_START_EVENT:
	if (IPPU.HDMA && CPU.V_Counter <= PPU.ScreenHeight)
	    IPPU.HDMA = S9xDoHDMA (IPPU.HDMA);

	break;

    case HBLANK_END_EVENT:
	S9xSuperFXExec ();

#ifndef STORM
	if (Settings.SoundSync)
	    S9xGenerateSound ();
#endif

	CPU.Cycles -= Settings.H_Max;
	IAPU.NextAPUTimerPos -= (Settings.H_Max * 10000L);
	if (IAPU.APUExecuting)
	{
	    APU.Cycles -= Settings.H_Max;
#ifdef MK_APU
		S9xCatchupCount();
#endif
	}
	else
	    APU.Cycles = 0;

	CPU.NextEvent = -1;
	ICPU.Scanline++;

	//if (++CPU.V_Counter >= (Settings.PAL ? SNES_MAX_PAL_VCOUNTER : SNES_MAX_NTSC_VCOUNTER))
	if (++CPU.V_Counter >= (SNES_MAX_NTSC_VCOUNTER))
	{
		CPU.V_Counter = 0;
		Memory.FillRAM[0x213F]^=0x80;
		PPU.RangeTimeOver = 0;
	    CPU.NMIActive = FALSE;
	    ICPU.Frame++;
	    PPU.HVBeamCounterLatched = 0;
	    CPU.Flags |= SCAN_KEYS_FLAG;
	    S9xStartHDMA ();
	}

	if (PPU.VTimerEnabled && !PPU.HTimerEnabled &&
	    CPU.V_Counter == PPU.IRQVBeamPos)
	{
	    S9xSetIRQ (PPU_V_BEAM_IRQ_SOURCE);
	}

	if (CPU.V_Counter == PPU.ScreenHeight + FIRST_VISIBLE_LINE)
	{
	    // Start of V-blank
	    S9xEndScreenRefresh ();
	    IPPU.HDMA = 0;
	    // Bits 7 and 6 of $4212 are computed when read in S9xGetPPU.
	    missing.dma_this_frame = 0;
	    IPPU.MaxBrightness = PPU.Brightness;
	    PPU.ForcedBlanking = (Memory.FillRAM [0x2100] >> 7) & 1;

		if(!PPU.ForcedBlanking){
			PPU.OAMAddr = PPU.SavedOAMAddr;
			{
				uint8 tmp = 0;
				if(PPU.OAMPriorityRotation)
					tmp = (PPU.OAMAddr&0xFE)>>1;
				if((PPU.OAMFlip&1) || PPU.FirstSprite!=tmp){
					PPU.FirstSprite=tmp;
					IPPU.OBJChanged=TRUE;
				}
			}
			PPU.OAMFlip = 0;
		}

	    Memory.FillRAM[0x4210] = 0x80 |Model->_5A22;
	    if (Memory.FillRAM[0x4200] & 0x80)
	    {
			CPU.NMIActive = TRUE;
			CPU.Flags |= NMI_FLAG;
			CPU.NMICycleCount = CPU.NMITriggerPoint;
	    }

#ifdef OLD_SNAPSHOT_CODE
	    if (CPU.Flags & SAVE_SNAPSHOT_FLAG)
		{
			CPU.Flags &= ~SAVE_SNAPSHOT_FLAG;
			Registers.PC = CPU.PC - CPU.PCBase;
			S9xPackStatus ();
			S9xAPUPackStatus ();
			Snapshot (NULL);
		}
#endif
	}

	if (CPU.V_Counter == PPU.ScreenHeight + 3)
	    S9xUpdateJoypads ();

	if (CPU.V_Counter == FIRST_VISIBLE_LINE)
	{
	    Memory.FillRAM[0x4210] = Model->_5A22;
	    CPU.Flags &= ~NMI_FLAG;
	    S9xStartScreenRefresh ();
	}
	if (CPU.V_Counter >= FIRST_VISIBLE_LINE &&
	    CPU.V_Counter < PPU.ScreenHeight + FIRST_VISIBLE_LINE)
	{
	    RenderLine (CPU.V_Counter - FIRST_VISIBLE_LINE);
	}
	break;

    case HTIMER_BEFORE_EVENT:
    case HTIMER_AFTER_EVENT:
	if (PPU.HTimerEnabled &&
	    (!PPU.VTimerEnabled || CPU.V_Counter == PPU.IRQVBeamPos))
	{
	    S9xSetIRQ (PPU_H_BEAM_IRQ_SOURCE);
	}
	break;
    }
    S9xReschedule ();
}
