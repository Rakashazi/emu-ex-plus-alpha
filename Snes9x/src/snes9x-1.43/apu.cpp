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

#ifdef __DJGPP
#include <allegro.h>
#undef TRUE
#endif

#include "snes9x.h"
#include "spc700.h"
#include "apu.h"
#include "soundux.h"
#include "cpuexec.h"

/* For note-triggered SPC dump support */
#include "snapshot.h"

// Raw SPC700 instruction cycle lengths
static const int32 S9xAPUCycleLengths [256] =
{
    /*        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, a, b, c, d, e, f, */
    /* 00 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 4, 6, 8,
    /* 10 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 6, 5, 2, 2, 4, 6,
    /* 20 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 4, 5, 4,
    /* 30 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 6, 5, 2, 2, 3, 8,
    /* 40 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 4, 6, 6,
    /* 50 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 4, 5, 2, 2, 4, 3,
    /* 60 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 4, 5, 5,
    /* 70 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2, 3, 6,
    /* 80 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 6, 5, 4, 5, 2, 4, 5,
    /* 90 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2,12, 5,
    /* a0 */  3, 8, 4, 5, 3, 4, 3, 6, 2, 6, 4, 4, 5, 2, 4, 4,
    /* b0 */  2, 8, 4, 5, 4, 5, 5, 6, 5, 5, 5, 5, 2, 2, 3, 4,
    /* c0 */  3, 8, 4, 5, 4, 5, 4, 7, 2, 5, 6, 4, 5, 2, 4, 9,
    /* d0 */  2, 8, 4, 5, 5, 6, 6, 7, 4, 5, 4, 5, 2, 2, 6, 3,
    /* e0 */  2, 8, 4, 5, 3, 4, 3, 6, 2, 4, 5, 3, 4, 3, 4, 3,
    /* f0 */  2, 8, 4, 5, 4, 5, 5, 6, 3, 4, 5, 4, 2, 2, 4, 3
};

extern "C" {const char *S9xGetFilenameInc (const char *);}

int spc_is_dumping=0;
int spc_is_dumping_temp;
uint8 spc_dump_dsp[0x100]; 

#ifdef DEBUGGER
void S9xTraceSoundDSP (const char *s, int i1 = 0, int i2 = 0, int i3 = 0,
					   int i4 = 0, int i5 = 0, int i6 = 0, int i7 = 0);
#endif

bool8 S9xInitAPU ()
{
    IAPU.RAM = (uint8 *) malloc (0x10000);
    IAPU.ShadowRAM = (uint8 *) malloc (0x10000);
    IAPU.CachedSamples = (uint8 *) malloc (0x40000);
    
    if (!IAPU.RAM || !IAPU.ShadowRAM || !IAPU.CachedSamples)
    {
		S9xDeinitAPU ();
		return (FALSE);
    }

	memset(IAPU.RAM, 0, 0x10000);
	memset(IAPU.ShadowRAM, 0, 0x10000);
	memset(IAPU.CachedSamples, 0, 0x40000);
	
    return (TRUE);
}

void S9xDeinitAPU ()
{
    if (IAPU.RAM)
    {
		free ((char *) IAPU.RAM);
		IAPU.RAM = NULL;
    }
    if (IAPU.ShadowRAM)
    {
		free ((char *) IAPU.ShadowRAM);
		IAPU.ShadowRAM = NULL;
    }
    if (IAPU.CachedSamples)
    {
		free ((char *) IAPU.CachedSamples);
		IAPU.CachedSamples = NULL;
    }
}

static const uint8 APUROM [64] =
{
    0xCD,0xEF,0xBD,0xE8,0x00,0xC6,0x1D,0xD0,0xFC,0x8F,0xAA,0xF4,0x8F,
    0xBB,0xF5,0x78,0xCC,0xF4,0xD0,0xFB,0x2F,0x19,0xEB,0xF4,0xD0,0xFC,
    0x7E,0xF4,0xD0,0x0B,0xE4,0xF5,0xCB,0xF4,0xD7,0x00,0xFC,0xD0,0xF3,
    0xAB,0x01,0x10,0xEF,0x7E,0xF4,0x10,0xEB,0xBA,0xF6,0xDA,0x00,0xBA,
    0xF4,0xC4,0xF4,0xDD,0x5D,0xD0,0xDB,0x1F,0x00,0x00,0xC0,0xFF
};

void S9xResetAPU ()
{

    int i;

    Settings.APUEnabled = Settings.NextAPUEnabled;

	ZeroMemory(spc_dump_dsp, 0x100);
	ZeroMemory(IAPU.RAM, 0x100);
	memset(IAPU.RAM+0x20, 0xFF, 0x20);
	memset(IAPU.RAM+0x60, 0xFF, 0x20);
	memset(IAPU.RAM+0xA0, 0xFF, 0x20);
	memset(IAPU.RAM+0xE0, 0xFF, 0x20);

	for(i=1;i<256;i++)
	{
		memcpy(IAPU.RAM+(i<<8), IAPU.RAM, 0x100);
	}

    memcpy (IAPU.ShadowRAM, IAPU.RAM, 0x10000);
	
    ZeroMemory (IAPU.CachedSamples, 0x40000);
    ZeroMemory (APU.OutPorts, 4);
    IAPU.DirectPage = IAPU.RAM;
    memmove (&IAPU.RAM [0xffc0], APUROM, sizeof (APUROM));
    memmove (APU.ExtraRAM, APUROM, sizeof (APUROM));
    IAPU.PC = IAPU.RAM + IAPU.RAM [0xfffe] + (IAPU.RAM [0xffff] << 8);
    APU.Cycles = 0;
    APURegisters.YA.W = 0;
    APURegisters.X = 0;
    APURegisters.S = 0xff;
    APURegisters.P = 0;
    S9xAPUUnpackStatus ();
    APURegisters.PC = 0;
    IAPU.APUExecuting = Settings.APUEnabled;
#ifdef SPC700_SHUTDOWN
    IAPU.WaitAddress1 = NULL;
    IAPU.WaitAddress2 = NULL;
    IAPU.WaitCounter = 0;
#endif
	IAPU.NextAPUTimerPos = 0;
	IAPU.APUTimerCounter = 0;
    APU.ShowROM = TRUE;
    IAPU.RAM [0xf1] = 0x80;
		
    for (i = 0; i < 3; i++)
    {
		APU.TimerEnabled [i] = FALSE;
		APU.TimerValueWritten [i] = 0;
		APU.TimerTarget [i] = 0;
		APU.Timer [i] = 0;
    }
    for (int j = 0; j < 0x80; j++)
		APU.DSP [j] = 0;
	
    IAPU.TwoCycles = IAPU.OneCycle * 2;
	
    for (i = 0; i < 256; i++)
		S9xAPUCycles [i] = S9xAPUCycleLengths [i] * IAPU.OneCycle;
	
    APU.DSP [APU_ENDX] = 0;
    APU.DSP [APU_KOFF] = 0;
    APU.DSP [APU_KON] = 0;
    APU.DSP [APU_FLG] = APU_MUTE | APU_ECHO_DISABLED;
    APU.KeyedChannels = 0;
	
    S9xResetSound (TRUE);
    S9xSetEchoEnable (0);
}

void S9xSetAPUDSP (uint8 byte)
{
    uint8 reg = IAPU.RAM [0xf2];
	static uint8 KeyOn;
	static uint8 KeyOnPrev;
    int i;

	spc_dump_dsp[reg] = byte;

    switch (reg)
    {
    case APU_FLG:
		if (byte & APU_SOFT_RESET)
		{
			APU.DSP [reg] = APU_MUTE | APU_ECHO_DISABLED | (byte & 0x1f);
			APU.DSP [APU_ENDX] = 0;
			APU.DSP [APU_KOFF] = 0;
			APU.DSP [APU_KON] = 0;
			S9xSetEchoWriteEnable (FALSE);
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] DSP reset\n", ICPU.Scanline);
#endif
			// Kill sound
			S9xResetSound (FALSE);
		}
		else
		{
			S9xSetEchoWriteEnable (!(byte & APU_ECHO_DISABLED));
			if (byte & APU_MUTE)
			{
#ifdef DEBUGGER
				if (Settings.TraceSoundDSP)
					S9xTraceSoundDSP ("[%d] Mute sound\n", ICPU.Scanline);
#endif
				S9xSetSoundMute (TRUE);
			}
			else
				S9xSetSoundMute (FALSE);
			
			SoundData.noise_hertz = NoiseFreq [byte & 0x1f];
			for (i = 0; i < 8; i++)
			{
				if (SoundData.channels [i].type == SOUND_NOISE)
					S9xSetSoundFrequency (i, SoundData.noise_hertz);
			}
		}
		break;
    case APU_NON:
		if (byte != APU.DSP [APU_NON])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Noise:", ICPU.Scanline);
#endif
			uint8 mask = 1;
			for (int c = 0; c < 8; c++, mask <<= 1)
			{
				int type;
				if (byte & mask)
				{
					type = SOUND_NOISE;
#ifdef DEBUGGER
					if (Settings.TraceSoundDSP)
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d,", c);
						else
							S9xTraceSoundDSP ("%d(on),", c);
					}
#endif
				}
				else
				{
					type = SOUND_SAMPLE;
#ifdef DEBUGGER
					if (Settings.TraceSoundDSP)
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d(off),", c);
					}
#endif
				}
				S9xSetSoundType (c, type);
			}
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("\n");
#endif
		}
		break;
    case APU_MVOL_LEFT:
		if (byte != APU.DSP [APU_MVOL_LEFT])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Master volume left:%d\n", 
				ICPU.Scanline, (signed char) byte);
#endif
			S9xSetMasterVolume ((signed char) byte,
				(signed char) APU.DSP [APU_MVOL_RIGHT]);
		}
		break;
    case APU_MVOL_RIGHT:
		if (byte != APU.DSP [APU_MVOL_RIGHT])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Master volume right:%d\n",
				ICPU.Scanline, (signed char) byte);
#endif
			S9xSetMasterVolume ((signed char) APU.DSP [APU_MVOL_LEFT],
				(signed char) byte);
		}
		break;
    case APU_EVOL_LEFT:
		if (byte != APU.DSP [APU_EVOL_LEFT])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Echo volume left:%d\n",
				ICPU.Scanline, (signed char) byte);
#endif
			S9xSetEchoVolume ((signed char) byte,
				(signed char) APU.DSP [APU_EVOL_RIGHT]);
		}
		break;
    case APU_EVOL_RIGHT:
		if (byte != APU.DSP [APU_EVOL_RIGHT])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Echo volume right:%d\n",
				ICPU.Scanline, (signed char) byte);
#endif
			S9xSetEchoVolume ((signed char) APU.DSP [APU_EVOL_LEFT],
				(signed char) byte);
		}
		break;
    case APU_ENDX:
#ifdef DEBUGGER
		if (Settings.TraceSoundDSP)
			S9xTraceSoundDSP ("[%d] Reset ENDX\n", ICPU.Scanline);
#endif
		byte = 0;
		break;
		
    case APU_KOFF:
		//		if (byte)
		{
			uint8 mask = 1;
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Key off:", ICPU.Scanline);
#endif
			for (int c = 0; c < 8; c++, mask <<= 1)
			{
				if ((byte & mask) != 0)
				{
#ifdef DEBUGGER
					
					if (Settings.TraceSoundDSP)
						S9xTraceSoundDSP ("%d,", c);
#endif		    
					if (APU.KeyedChannels & mask)
					{
						{
							KeyOnPrev&=~mask;
							APU.KeyedChannels &= ~mask;
							APU.DSP [APU_KON] &= ~mask;
							//APU.DSP [APU_KOFF] |= mask;
							S9xSetSoundKeyOff (c);
						}
					}
				}
				else if((KeyOnPrev&mask)!=0)
				{
					KeyOnPrev&=~mask;
					APU.KeyedChannels |= mask;
					//APU.DSP [APU_KON] |= mask;
					APU.DSP [APU_KOFF] &= ~mask;
					APU.DSP [APU_ENDX] &= ~mask;
					S9xPlaySample (c);
				}
			}
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("\n");
#endif
		}
		//KeyOnPrev=0;
		APU.DSP [APU_KOFF] = byte;
		return;
    case APU_KON:
		if (spc_is_dumping)
		{
			if (byte & ~spc_is_dumping_temp)
			{
				APURegisters.PC = IAPU.PC - IAPU.RAM;
				S9xAPUPackStatus();
				S9xSPCDump (S9xGetFilenameInc (".spc"));
				spc_is_dumping = 0;
			}
		}
		if (byte)
		{
			uint8 mask = 1;
#ifdef DEBUGGER
			
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] Key on:", ICPU.Scanline);
#endif
			for (int c = 0; c < 8; c++, mask <<= 1)
			{
				if ((byte & mask) != 0)
				{
#ifdef DEBUGGER
					if (Settings.TraceSoundDSP)
						S9xTraceSoundDSP ("%d,", c);
#endif		    
					// Pac-In-Time requires that channels can be key-on
					// regardeless of their current state.
					if((APU.DSP [APU_KOFF] & mask) ==0)
					{
						KeyOnPrev&=~mask;
						APU.KeyedChannels |= mask;
						//APU.DSP [APU_KON] |= mask;
						//APU.DSP [APU_KOFF] &= ~mask;
						APU.DSP [APU_ENDX] &= ~mask;
						S9xPlaySample (c);
					}
					else KeyOn|=mask;
				}
			}
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("\n");
#endif
		}
		spc_is_dumping_temp = byte;
		return;
		
    case APU_VOL_LEFT + 0x00:
    case APU_VOL_LEFT + 0x10:
    case APU_VOL_LEFT + 0x20:
    case APU_VOL_LEFT + 0x30:
    case APU_VOL_LEFT + 0x40:
    case APU_VOL_LEFT + 0x50:
    case APU_VOL_LEFT + 0x60:
    case APU_VOL_LEFT + 0x70:
		// At Shin Megami Tensei suggestion 6/11/00
		//	if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d volume left: %d\n", 
				ICPU.Scanline, reg>>4, (signed char) byte);
#endif
			S9xSetSoundVolume (reg >> 4, (signed char) byte,
				(signed char) APU.DSP [reg + 1]);
		}
		break;
    case APU_VOL_RIGHT + 0x00:
    case APU_VOL_RIGHT + 0x10:
    case APU_VOL_RIGHT + 0x20:
    case APU_VOL_RIGHT + 0x30:
    case APU_VOL_RIGHT + 0x40:
    case APU_VOL_RIGHT + 0x50:
    case APU_VOL_RIGHT + 0x60:
    case APU_VOL_RIGHT + 0x70:
		// At Shin Megami Tensei suggestion 6/11/00
		//	if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d volume right: %d\n", 
				ICPU.Scanline, reg >>4, (signed char) byte);
#endif
			S9xSetSoundVolume (reg >> 4, (signed char) APU.DSP [reg - 1],
				(signed char) byte);
		}
		break;
		
    case APU_P_LOW + 0x00:
    case APU_P_LOW + 0x10:
    case APU_P_LOW + 0x20:
    case APU_P_LOW + 0x30:
    case APU_P_LOW + 0x40:
    case APU_P_LOW + 0x50:
    case APU_P_LOW + 0x60:
    case APU_P_LOW + 0x70:
#ifdef DEBUGGER
		if (Settings.TraceSoundDSP)
			S9xTraceSoundDSP ("[%d] %d freq low: %d\n",
			ICPU.Scanline, reg>>4, byte);
#endif
		S9xSetSoundHertz (reg >> 4, ((byte + (APU.DSP [reg + 1] << 8)) & FREQUENCY_MASK) * 8);
		break;
		
    case APU_P_HIGH + 0x00:
    case APU_P_HIGH + 0x10:
    case APU_P_HIGH + 0x20:
    case APU_P_HIGH + 0x30:
    case APU_P_HIGH + 0x40:
    case APU_P_HIGH + 0x50:
    case APU_P_HIGH + 0x60:
    case APU_P_HIGH + 0x70:
#ifdef DEBUGGER
		if (Settings.TraceSoundDSP)
			S9xTraceSoundDSP ("[%d] %d freq high: %d\n",
			ICPU.Scanline, reg>>4, byte);
#endif
		S9xSetSoundHertz (reg >> 4, 
			(((byte << 8) + APU.DSP [reg - 1]) & FREQUENCY_MASK) * 8);
		break;
		
    case APU_SRCN + 0x00:
    case APU_SRCN + 0x10:
    case APU_SRCN + 0x20:
    case APU_SRCN + 0x30:
    case APU_SRCN + 0x40:
    case APU_SRCN + 0x50:
    case APU_SRCN + 0x60:
    case APU_SRCN + 0x70:
		if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d sample number: %d\n",
				ICPU.Scanline, reg>>4, byte);
#endif
			//S9xSetSoundSample (reg >> 4, byte);
		}
		break;
		
    case APU_ADSR1 + 0x00:
    case APU_ADSR1 + 0x10:
    case APU_ADSR1 + 0x20:
    case APU_ADSR1 + 0x30:
    case APU_ADSR1 + 0x40:
    case APU_ADSR1 + 0x50:
    case APU_ADSR1 + 0x60:
    case APU_ADSR1 + 0x70:
		if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d adsr1: %02x\n",
				ICPU.Scanline, reg>>4, byte);
#endif
			{
				S9xFixEnvelope (reg >> 4, APU.DSP [reg + 2], byte, 
					APU.DSP [reg + 1]);
			}
		}
		break;
		
    case APU_ADSR2 + 0x00:
    case APU_ADSR2 + 0x10:
    case APU_ADSR2 + 0x20:
    case APU_ADSR2 + 0x30:
    case APU_ADSR2 + 0x40:
    case APU_ADSR2 + 0x50:
    case APU_ADSR2 + 0x60:
    case APU_ADSR2 + 0x70:
		if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d adsr2: %02x\n", 
				ICPU.Scanline, reg>>4, byte);
#endif
			{
				S9xFixEnvelope (reg >> 4, APU.DSP [reg + 1], APU.DSP [reg - 1],
					byte);
			}
		}
		break;
		
    case APU_GAIN + 0x00:
    case APU_GAIN + 0x10:
    case APU_GAIN + 0x20:
    case APU_GAIN + 0x30:
    case APU_GAIN + 0x40:
    case APU_GAIN + 0x50:
    case APU_GAIN + 0x60:
    case APU_GAIN + 0x70:
		if (byte != APU.DSP [reg])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
				S9xTraceSoundDSP ("[%d] %d gain: %02x\n",
				ICPU.Scanline, reg>>4, byte);
#endif
			{
				S9xFixEnvelope (reg >> 4, byte, APU.DSP [reg - 2],
					APU.DSP [reg - 1]);
			}
		}
		break;
		
    case APU_ENVX + 0x00:
    case APU_ENVX + 0x10:
    case APU_ENVX + 0x20:
    case APU_ENVX + 0x30:
    case APU_ENVX + 0x40:
    case APU_ENVX + 0x50:
    case APU_ENVX + 0x60:
    case APU_ENVX + 0x70:
		break;
		
    case APU_OUTX + 0x00:
    case APU_OUTX + 0x10:
    case APU_OUTX + 0x20:
    case APU_OUTX + 0x30:
    case APU_OUTX + 0x40:
    case APU_OUTX + 0x50:
    case APU_OUTX + 0x60:
    case APU_OUTX + 0x70:
		break;
		
    case APU_DIR:
#ifdef DEBUGGER
		if (Settings.TraceSoundDSP)
			S9xTraceSoundDSP ("[%d] Sample directory to: %02x\n",
			ICPU.Scanline, byte);
#endif
		break;
		
    case APU_PMON:
		if (byte != APU.DSP [APU_PMON])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
			{
				S9xTraceSoundDSP ("[%d] FreqMod:", ICPU.Scanline);
				uint8 mask = 1;
				for (int c = 0; c < 8; c++, mask <<= 1)
				{
					if (byte & mask)
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d", c);
						else
							S9xTraceSoundDSP ("%d(on),", c);
					}
					else
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d(off),", c);
					}
				}
				S9xTraceSoundDSP ("\n");
			}
#endif
			S9xSetFrequencyModulationEnable (byte);
		}
		break;
		
    case APU_EON:
		if (byte != APU.DSP [APU_EON])
		{
#ifdef DEBUGGER
			if (Settings.TraceSoundDSP)
			{
				S9xTraceSoundDSP ("[%d] Echo:", ICPU.Scanline);
				uint8 mask = 1;
				for (int c = 0; c < 8; c++, mask <<= 1)
				{
					if (byte & mask)
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d", c);
						else
							S9xTraceSoundDSP ("%d(on),", c);
					}
					else
					{
						if (APU.DSP [reg] & mask)
							S9xTraceSoundDSP ("%d(off),", c);
					}
				}
				S9xTraceSoundDSP ("\n");
			}
#endif
			S9xSetEchoEnable (byte);
		}
		break;
		
    case APU_EFB:
		S9xSetEchoFeedback ((signed char) byte);
		break;
		
    case APU_ESA:
		break;
		
    case APU_EDL:
		S9xSetEchoDelay (byte & 0xf);
		break;
		
    case APU_C0:
    case APU_C1:
    case APU_C2:
    case APU_C3:
    case APU_C4:
    case APU_C5:
    case APU_C6:
    case APU_C7:
		S9xSetFilterCoefficient (reg >> 4, (signed char) byte);
		break;
    default:
		// XXX
		//printf ("Write %02x to unknown APU register %02x\n", byte, reg);
		break;
    }
	
	KeyOnPrev|=KeyOn;
	KeyOn=0;
	
    if (reg < 0x80)
		APU.DSP [reg] = byte;
}

void S9xFixEnvelope (int channel, uint8 gain, uint8 adsr1, uint8 adsr2)
{
    if (adsr1 & 0x80)
    {
		// ADSR mode
		static unsigned AttackRate [16] = {
			4100, 2600, 1500, 1000, 640, 380, 260, 160,
				96, 64, 40, 24, 16, 10, 6, 1
		};
		static unsigned DecayRate [8] = {
			1200, 740, 440, 290, 180, 110, 74, 37
		};
		static unsigned SustainRate [32] = {
			~0U, 38000, 28000, 24000, 19000, 14000, 12000, 9400,
				7100, 5900, 4700, 3500, 2900, 2400, 1800, 1500,
				1200, 880, 740, 590, 440, 370, 290, 220,
				180, 150, 110, 92, 74, 55, 37, 18
		};
		// XXX: can DSP be switched to ADSR mode directly from GAIN/INCREASE/
		// DECREASE mode? And if so, what stage of the sequence does it start
		// at?
		if (S9xSetSoundMode (channel, MODE_ADSR))
		{
			// Hack for ROMs that use a very short attack rate, key on a 
			// channel, then switch to decay mode. e.g. Final Fantasy II.
			
			int attack = AttackRate [adsr1 & 0xf];
			
			if (attack == 1 && (!Settings.SoundSync
#ifdef __WIN32__
                || Settings.SoundDriver != WIN_SNES9X_DIRECT_SOUND_DRIVER
#endif
                ))
				attack = 0;
			
			S9xSetSoundADSR (channel, attack,
				DecayRate [(adsr1 >> 4) & 7],
				SustainRate [adsr2 & 0x1f],
				(adsr2 >> 5) & 7, 8);
		}
    }
    else
    {
		// Gain mode
		if ((gain & 0x80) == 0)
		{
			if (S9xSetSoundMode (channel, MODE_GAIN))
			{
				S9xSetEnvelopeRate (channel, 0, 0, gain & 0x7f);
				S9xSetEnvelopeHeight (channel, gain & 0x7f);
			}
		}
		else
		{
			static unsigned IncreaseRate [32] = {
				~0U, 4100, 3100, 2600, 2000, 1500, 1300, 1000,
					770, 640, 510, 380, 320, 260, 190, 160,
					130, 96, 80, 64, 48, 40, 32, 24,
					20, 16, 12, 10, 8, 6, 4, 2
			};
			static unsigned DecreaseRateExp [32] = {
				~0U, 38000, 28000, 24000, 19000, 14000, 12000, 9400,
					7100, 5900, 4700, 3500, 2900, 2400, 1800, 1500,
					1200, 880, 740, 590, 440, 370, 290, 220,
					180, 150, 110, 92, 74, 55, 37, 18
			};
			if (gain & 0x40)
			{
				// Increase mode
				if (S9xSetSoundMode (channel, (gain & 0x20) ?
MODE_INCREASE_BENT_LINE :
				MODE_INCREASE_LINEAR))
				{
					S9xSetEnvelopeRate (channel, IncreaseRate [gain & 0x1f],
						1, 127);
				}
			}
			else
			{
				uint32 rate = (gain & 0x20) ? DecreaseRateExp [gain & 0x1f] / 2 :
			IncreaseRate [gain & 0x1f];
			int mode = (gain & 0x20) ? MODE_DECREASE_EXPONENTIAL
				: MODE_DECREASE_LINEAR;
			
			if (S9xSetSoundMode (channel, mode))
				S9xSetEnvelopeRate (channel, rate, -1, 0);
			}
		}
    }
}

void S9xSetAPUControl (uint8 byte)
{
	//if (byte & 0x40)
	//printf ("*** Special SPC700 timing enabled\n");
    if ((byte & 1) != 0 && !APU.TimerEnabled [0])
    {
		APU.Timer [0] = 0;
		IAPU.RAM [0xfd] = 0;
		if ((APU.TimerTarget [0] = IAPU.RAM [0xfa]) == 0)
			APU.TimerTarget [0] = 0x100;
    }
    if ((byte & 2) != 0 && !APU.TimerEnabled [1])
    {
		APU.Timer [1] = 0;
		IAPU.RAM [0xfe] = 0;
		if ((APU.TimerTarget [1] = IAPU.RAM [0xfb]) == 0)
			APU.TimerTarget [1] = 0x100;
    }
    if ((byte & 4) != 0 && !APU.TimerEnabled [2])
    {
		APU.Timer [2] = 0;
		IAPU.RAM [0xff] = 0;
		if ((APU.TimerTarget [2] = IAPU.RAM [0xfc]) == 0)
			APU.TimerTarget [2] = 0x100;
    }
    APU.TimerEnabled [0] = byte & 1;
    APU.TimerEnabled [1] = (byte & 2) >> 1;
    APU.TimerEnabled [2] = (byte & 4) >> 2;
	
    if (byte & 0x10)
		IAPU.RAM [0xF4] = IAPU.RAM [0xF5] = 0;
	
    if (byte & 0x20)
		IAPU.RAM [0xF6] = IAPU.RAM [0xF7] = 0;
	
    if (byte & 0x80)
    {
		if (!APU.ShowROM)
		{
			memmove (&IAPU.RAM [0xffc0], APUROM, sizeof (APUROM));
			APU.ShowROM = TRUE;
		}
    }
    else
    {
		if (APU.ShowROM)
		{
			APU.ShowROM = FALSE;
			memmove (&IAPU.RAM [0xffc0], APU.ExtraRAM, sizeof (APUROM));
		}
    }
    IAPU.RAM [0xf1] = byte;
}

void S9xSetAPUTimer (uint16 Address, uint8 byte)
{
    IAPU.RAM [Address] = byte;
	
    switch (Address)
    {
    case 0xfa:
		if ((APU.TimerTarget [0] = IAPU.RAM [0xfa]) == 0)
			APU.TimerTarget [0] = 0x100;
		APU.TimerValueWritten [0] = TRUE;
		break;
    case 0xfb:
		if ((APU.TimerTarget [1] = IAPU.RAM [0xfb]) == 0)
			APU.TimerTarget [1] = 0x100;
		APU.TimerValueWritten [1] = TRUE;
		break;
    case 0xfc:
		if ((APU.TimerTarget [2] = IAPU.RAM [0xfc]) == 0)
			APU.TimerTarget [2] = 0x100;
		APU.TimerValueWritten [2] = TRUE;
		break;
    }
}

void S9xUpdateAPUTimer (void)
{
	while (CPU.Cycles * 10000L >= IAPU.NextAPUTimerPos)
	//if (CPU.Cycles * 10000L >= IAPU.NextAPUTimerPos)
	{
		//APU_EXECUTE();
		
		IAPU.NextAPUTimerPos += SNES_APUTIMER2_CYCLEx10000;		
		
		if (APU.TimerEnabled [2])
		{
			APU.Timer [2] ++;
			if (APU.Timer [2] >= APU.TimerTarget [2])
			{
			    IAPU.RAM [0xff] = (IAPU.RAM [0xff] + 1) & 0xf;
			    APU.Timer [2] = 0;
			#ifdef SPC700_SHUTDOWN		
			    IAPU.WaitCounter++;
			    IAPU.APUExecuting = TRUE;
			#endif		
			}
		}

		if (++IAPU.APUTimerCounter == 8)
		{
			IAPU.APUTimerCounter = 0;
			
			if (APU.TimerEnabled [0])
			{
			    APU.Timer [0]++;
			    if (APU.Timer [0] >= APU.TimerTarget [0])
			    {
					IAPU.RAM [0xfd] = (IAPU.RAM [0xfd] + 1) & 0xf;
					APU.Timer [0] = 0;
				#ifdef SPC700_SHUTDOWN		
					IAPU.WaitCounter++;
					IAPU.APUExecuting = TRUE;
				#endif		    
			    }
			}

			if (APU.TimerEnabled [1])
			{
			    APU.Timer [1]++;
			    if (APU.Timer [1] >= APU.TimerTarget [1])
			    {
					IAPU.RAM [0xfe] = (IAPU.RAM [0xfe] + 1) & 0xf;
					APU.Timer [1] = 0;
				#ifdef SPC700_SHUTDOWN		
					IAPU.WaitCounter++;
					IAPU.APUExecuting = TRUE;
				#endif		    
			    }
			}
		}
	}
}

uint8 S9xGetAPUDSP ()
{
    uint8 reg = IAPU.RAM [0xf2] & 0x7f;
    uint8 byte = APU.DSP [reg];
	
    switch (reg)
    {
    case APU_KON:
		break;
    case APU_KOFF:
		break;
    case APU_OUTX + 0x00:
    case APU_OUTX + 0x10:
    case APU_OUTX + 0x20:
    case APU_OUTX + 0x30:
    case APU_OUTX + 0x40:
    case APU_OUTX + 0x50:
    case APU_OUTX + 0x60:
    case APU_OUTX + 0x70:
		if (SoundData.channels [reg >> 4].state == SOUND_SILENT)
			return (0);
		return ((SoundData.channels [reg >> 4].sample >> 8) |
			(SoundData.channels [reg >> 4].sample & 0xff));
		
    case APU_ENVX + 0x00:
    case APU_ENVX + 0x10:
    case APU_ENVX + 0x20:
    case APU_ENVX + 0x30:
    case APU_ENVX + 0x40:
    case APU_ENVX + 0x50:
    case APU_ENVX + 0x60:
    case APU_ENVX + 0x70:
		return ((uint8) S9xGetEnvelopeHeight (reg >> 4));
		
    case APU_ENDX:
		// To fix speech in Magical Drop 2 6/11/00
		//	APU.DSP [APU_ENDX] = 0;
		break;
    default:
		break;
    }
    return (byte);
}

