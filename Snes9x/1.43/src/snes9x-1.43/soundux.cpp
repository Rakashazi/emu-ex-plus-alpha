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
#ifdef __DJGPP__
#include <allegro.h>
#undef TRUE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#define CLIP16(v) \
	if ((v) < -32768) \
    (v) = -32768; \
	else \
	if ((v) > 32767) \
(v) = 32767

#define CLIP16_latch(v,l) \
	if ((v) < -32768) \
{ (v) = -32768; (l)++; }\
	else \
	if ((v) > 32767) \
{ (v) = 32767; (l)++; }

#define CLIP24(v) \
	if ((v) < -8388608) \
    (v) = -8388608; \
	else \
	if ((v) > 8388607) \
(v) = 8388607

#define CLIP8(v) \
	if ((v) < -128) \
    (v) = -128; \
	else \
	if ((v) > 127) \
(v) = 127

#include "snes9x.h"
#include "soundux.h"
#include "apu.h"
#include "memmap.h"
#include "cpuexec.h"

static int Echo [24000];
static int DummyEchoBuffer [SOUND_BUFFER_SIZE];
static int MixBuffer [SOUND_BUFFER_SIZE];
static int EchoBuffer [SOUND_BUFFER_SIZE];
static int FilterTaps [8];
static unsigned int Z = 0;
static int Loop [16];

static SoundStatus so;

static const int FilterValues[4][2] =
{
    {0, 0},
    {240, 0},
    {488, -240},
    {460, -208}
};

#undef ABS
#define ABS(a) ((a) < 0 ? -(a) : (a))

#define FIXED_POINT 0x10000UL
#define FIXED_POINT_REMAINDER 0xffffUL
#define FIXED_POINT_SHIFT 16

#define VOL_DIV8  0x8000
#define VOL_DIV16 0x0080
#define ENVX_SHIFT 24

extern "C" void DecodeBlockAsm (int8 *, int16 *, int32 *, int32 *);
extern "C" void DecodeBlockAsm2 (int8 *, int16 *, int32 *, int32 *);

// F is channel's current frequency and M is the 16-bit modulation waveform
// from the previous channel multiplied by the current envelope volume level.
#define PITCH_MOD(F,M) ((F) * ((((unsigned long) (M)) + 0x800000) >> 16) >> 7)
//#define PITCH_MOD(F,M) ((F) * ((((M) & 0x7fffff) >> 14) + 1) >> 8)

#define LAST_SAMPLE 0xffffff
#define JUST_PLAYED_LAST_SAMPLE(c) ((c)->sample_pointer >= LAST_SAMPLE)

STATIC inline uint16 *S9xGetSampleAddress (int sample_number)
{
    uint32 addr = (((APU.DSP[APU_DIR] << 8) + (sample_number << 2)) & 0xffff);
    return (uint16 *) (IAPU.RAM + addr);
}

void S9xAPUSetEndOfSample (int i, Channel *ch)
{
    ch->state = SOUND_SILENT;
    ch->mode = MODE_NONE;
    APU.DSP [APU_ENDX] |= 1 << i;
    APU.DSP [APU_KON] &= ~(1 << i);
    APU.DSP [APU_KOFF] &= ~(1 << i);
    APU.KeyedChannels &= ~(1 << i);
}
#ifdef __DJGPP
END_OF_FUNCTION (S9xAPUSetEndOfSample)
#endif

void S9xAPUSetEndX (int ch)
{
    APU.DSP [APU_ENDX] |= 1 << ch;
}
#ifdef __DJGPP
END_OF_FUNCTION (S9xAPUSetEndX)
#endif

void S9xSetEnvRate (Channel *ch, unsigned long rate, int direction, int target)
{
    ch->envx_target = target;
	
    if (rate == ~0UL)
    {
		ch->direction = 0;
		rate = 0;
    }
    else
		ch->direction = direction;
	
    static int steps [] =
    {
		//	0, 64, 1238, 1238, 256, 1, 64, 109, 64, 1238
		0, 64, 619, 619, 128, 1, 64, 55, 64, 619
    };
	
    if (rate == 0 || so.playback_rate == 0)
		ch->erate = 0;
    else
    {
		ch->erate = (unsigned long)
			(((int64) FIXED_POINT * 1000 * steps [ch->state]) /
			(rate * so.playback_rate));
    }
}

#ifdef __DJGPP
END_OF_FUNCTION(S9xSetEnvRate);
#endif

void S9xSetEnvelopeRate (int channel, unsigned int rate, int direction,
						 int target)
{
    S9xSetEnvRate (&SoundData.channels [channel], rate, direction, target);
}

#ifdef __DJGPP
END_OF_FUNCTION(S9xSetEnvelopeRate);
#endif

void S9xSetSoundVolume (int channel, short volume_left, short volume_right)
{
    Channel *ch = &SoundData.channels[channel];
    if (!so.stereo)
		volume_left = (ABS(volume_right) + ABS(volume_left)) / 2;
	
    ch->volume_left = volume_left;
    ch->volume_right = volume_right;
    ch-> left_vol_level = (ch->envx * volume_left) / 128;
    ch->right_vol_level = (ch->envx * volume_right) / 128;
}

void S9xSetMasterVolume (short volume_left, short volume_right)
{
    if (Settings.DisableMasterVolume || SNESGameFixes.EchoOnlyOutput)
    {
		SoundData.master_volume_left = 127;
		SoundData.master_volume_right = 127;
		SoundData.master_volume [0] = SoundData.master_volume [1] = 127;
    }
    else
    {
		if (!so.stereo)
			volume_left = (ABS (volume_right) + ABS (volume_left)) / 2;
		SoundData.master_volume_left = volume_left;
		SoundData.master_volume_right = volume_right;
		SoundData.master_volume [Settings.ReverseStereo] = volume_left;
		SoundData.master_volume [1 ^ Settings.ReverseStereo] = volume_right;
    }
}

void S9xSetEchoVolume (short volume_left, short volume_right)
{
    if (!so.stereo)
		volume_left = (ABS (volume_right) + ABS (volume_left)) / 2;
    SoundData.echo_volume_left = volume_left;
    SoundData.echo_volume_right = volume_right;
    SoundData.echo_volume [Settings.ReverseStereo] = volume_left;
    SoundData.echo_volume [1 ^ Settings.ReverseStereo] = volume_right;
}

void S9xSetEchoEnable (uint8 byte)
{
    SoundData.echo_channel_enable = byte;
    if (!SoundData.echo_write_enabled || Settings.DisableSoundEcho)
		byte = 0;
    if (byte && !SoundData.echo_enable)
    {
		memset (Echo, 0, sizeof (Echo));
		memset (Loop, 0, sizeof (Loop));
    }
	
    SoundData.echo_enable = byte;
    for (int i = 0; i < 8; i++)
    {
		if (byte & (1 << i))
			SoundData.channels [i].echo_buf_ptr = EchoBuffer;
		else
			SoundData.channels [i].echo_buf_ptr = DummyEchoBuffer;
    }
}

void S9xSetEchoFeedback (int feedback)
{
    CLIP8(feedback);
    SoundData.echo_feedback = feedback;
}

void S9xSetEchoDelay (int delay)
{
    SoundData.echo_buffer_size = (512 * delay * so.playback_rate) / 32000;
    if (so.stereo)
		SoundData.echo_buffer_size <<= 1;
    if (SoundData.echo_buffer_size)
		SoundData.echo_ptr %= SoundData.echo_buffer_size;
    else
		SoundData.echo_ptr = 0;
    S9xSetEchoEnable (APU.DSP [APU_EON]);
}

void S9xSetEchoWriteEnable (uint8 byte)
{
    SoundData.echo_write_enabled = byte;
    S9xSetEchoDelay (APU.DSP [APU_EDL] & 15);
}

void S9xSetFrequencyModulationEnable (uint8 byte)
{
    SoundData.pitch_mod = byte & ~1;
}

void S9xSetSoundKeyOff (int channel)
{
    Channel *ch = &SoundData.channels[channel];
	
    if (ch->state != SOUND_SILENT)
    {
		ch->state = SOUND_RELEASE;
		ch->mode = MODE_RELEASE;
		S9xSetEnvRate (ch, 8, -1, 0);
    }
}

void S9xFixSoundAfterSnapshotLoad ()
{
    SoundData.echo_write_enabled = !(APU.DSP [APU_FLG] & 0x20);
    SoundData.echo_channel_enable = APU.DSP [APU_EON];
    S9xSetEchoDelay (APU.DSP [APU_EDL] & 0xf);
    S9xSetEchoFeedback ((signed char) APU.DSP [APU_EFB]);
	
    S9xSetFilterCoefficient (0, (signed char) APU.DSP [APU_C0]);
    S9xSetFilterCoefficient (1, (signed char) APU.DSP [APU_C1]);
    S9xSetFilterCoefficient (2, (signed char) APU.DSP [APU_C2]);
    S9xSetFilterCoefficient (3, (signed char) APU.DSP [APU_C3]);
    S9xSetFilterCoefficient (4, (signed char) APU.DSP [APU_C4]);
    S9xSetFilterCoefficient (5, (signed char) APU.DSP [APU_C5]);
    S9xSetFilterCoefficient (6, (signed char) APU.DSP [APU_C6]);
    S9xSetFilterCoefficient (7, (signed char) APU.DSP [APU_C7]);
    for (int i = 0; i < 8; i++)
    {
		SoundData.channels[i].needs_decode = TRUE;
		S9xSetSoundFrequency (i, SoundData.channels[i].hertz);
		SoundData.channels [i].envxx = SoundData.channels [i].envx << ENVX_SHIFT;
		SoundData.channels [i].next_sample = 0;
		SoundData.channels [i].interpolate = 0;
		SoundData.channels [i].previous [0] = (int32) SoundData.channels [i].previous16 [0];
		SoundData.channels [i].previous [1] = (int32) SoundData.channels [i].previous16 [1];
    }
    SoundData.master_volume [Settings.ReverseStereo] = SoundData.master_volume_left;
    SoundData.master_volume [1 ^ Settings.ReverseStereo] = SoundData.master_volume_right;
    SoundData.echo_volume [Settings.ReverseStereo] = SoundData.echo_volume_left;
    SoundData.echo_volume [1 ^ Settings.ReverseStereo] = SoundData.echo_volume_right;
    IAPU.Scanline = 0;
}

void S9xSetFilterCoefficient (int tap, int value)
{
    FilterTaps [tap & 7] = value;
    SoundData.no_filter = (FilterTaps [0] == 127 || FilterTaps [0] == 0) && 
		FilterTaps [1] == 0   &&
		FilterTaps [2] == 0   &&
		FilterTaps [3] == 0   &&
		FilterTaps [4] == 0   &&
		FilterTaps [5] == 0   &&
		FilterTaps [6] == 0   &&
		FilterTaps [7] == 0;
}

void S9xSetSoundADSR (int channel, int attack_rate, int decay_rate,
					  int sustain_rate, int sustain_level, int release_rate)
{
    SoundData.channels[channel].attack_rate = attack_rate;
    SoundData.channels[channel].decay_rate = decay_rate;
    SoundData.channels[channel].sustain_rate = sustain_rate;
    SoundData.channels[channel].release_rate = release_rate;
    SoundData.channels[channel].sustain_level = sustain_level + 1;
	
    switch (SoundData.channels[channel].state)
    {
    case SOUND_ATTACK:
		S9xSetEnvelopeRate (channel, attack_rate, 1, 127);
		break;
		
    case SOUND_DECAY:
		S9xSetEnvelopeRate (channel, decay_rate, -1,
			(MAX_ENVELOPE_HEIGHT * (sustain_level + 1)) >> 3);
		break;
    case SOUND_SUSTAIN:
		S9xSetEnvelopeRate (channel, sustain_rate, -1, 0);
		break;
    }
}

void S9xSetEnvelopeHeight (int channel, int level)
{
    Channel *ch = &SoundData.channels[channel];
	
    ch->envx = level;
    ch->envxx = level << ENVX_SHIFT;
	
    ch->left_vol_level = (level * ch->volume_left) / 128;
    ch->right_vol_level = (level * ch->volume_right) / 128;
	
    if (ch->envx == 0 && ch->state != SOUND_SILENT && ch->state != SOUND_GAIN)
    {
		S9xAPUSetEndOfSample (channel, ch);
    }
}

int S9xGetEnvelopeHeight (int channel)
{
    if ((Settings.SoundEnvelopeHeightReading ||
		SNESGameFixes.SoundEnvelopeHeightReading2) &&
        SoundData.channels[channel].state != SOUND_SILENT &&
        SoundData.channels[channel].state != SOUND_GAIN)
    {
        return (SoundData.channels[channel].envx);
    }
	
    //siren fix from XPP
    if (SNESGameFixes.SoundEnvelopeHeightReading2 &&
        SoundData.channels[channel].state != SOUND_SILENT)
    {
        return (SoundData.channels[channel].envx);
    }
	
    return (0);
}

#if 1
void S9xSetSoundSample (int, uint16) 
{
}
#else
void S9xSetSoundSample (int channel, uint16 sample_number)
{
    register Channel *ch = &SoundData.channels[channel];
	
    if (ch->state != SOUND_SILENT && 
		sample_number != ch->sample_number)
    {
		int keep = ch->state;
		ch->state = SOUND_SILENT;
		ch->sample_number = sample_number;
		ch->loop = FALSE;
		ch->needs_decode = TRUE;
		ch->last_block = FALSE;
		ch->previous [0] = ch->previous[1] = 0;
		ch->block_pointer = *S9xGetSampleAddress(sample_number);
		ch->sample_pointer = 0;
		ch->state = keep;
    }
}
#endif

void S9xSetSoundFrequency (int channel, int hertz)
{
    if (so.playback_rate)
    {
		if (SoundData.channels[channel].type == SOUND_NOISE)
			hertz = NoiseFreq [APU.DSP [APU_FLG] & 0x1f];
		SoundData.channels[channel].frequency = (int)
			(((int64) hertz * FIXED_POINT) / so.playback_rate);
		if (Settings.FixFrequency)
		{
			SoundData.channels[channel].frequency = 
				(unsigned int) ((SysDDec)  SoundData.channels[channel].frequency * 0.980);
		}
    }
}

void S9xSetSoundHertz (int channel, int hertz)
{
    SoundData.channels[channel].hertz = hertz;
    S9xSetSoundFrequency (channel, hertz);
}

void S9xSetSoundType (int channel, int type_of_sound)
{
    SoundData.channels[channel].type = type_of_sound;
}

bool8 S9xSetSoundMute (bool8 mute)
{
    bool8 old = so.mute_sound;
    so.mute_sound = mute;
    return (old);
}

void AltDecodeBlock (Channel *ch)
{
    if (ch->block_pointer >= 0x10000 - 9)
    {
		ch->last_block = TRUE;
		ch->loop = FALSE;
		ch->block = ch->decoded;
		memset ((void *) ch->decoded, 0, sizeof (int16) * 16);
		return;
    }
    signed char *compressed = (signed char *) &IAPU.RAM [ch->block_pointer];
	
    unsigned char filter = *compressed;
    if ((ch->last_block = filter & 1))
		ch->loop = (filter & 2) != 0;
	
#if (defined (USE_X86_ASM) && (defined (__i386__) || defined (__i486__) ||\
               defined (__i586__) || defined (__WIN32__) || defined (__DJGPP)))
    int16 *raw = ch->block = ch->decoded;
	
    if (Settings.AltSampleDecode == 1)
		DecodeBlockAsm (compressed, raw, &ch->previous [0], &ch->previous [1]);
    else
		DecodeBlockAsm2 (compressed, raw, &ch->previous [0], &ch->previous [1]);
#else
    int32 out;
    unsigned char shift;
    signed char sample1, sample2;
    unsigned int i;
	
    compressed++;
    signed short *raw = ch->block = ch->decoded;
    
    int32 prev0 = ch->previous [0];
    int32 prev1 = ch->previous [1];
    shift = filter >> 4;
	
    switch ((filter >> 2) & 3)
    {
    case 0:
		for (i = 8; i != 0; i--)
		{
			sample1 = *compressed++;
			sample2 = sample1 << 4;
			sample2 >>= 4;
			sample1 >>= 4;
			*raw++ = ((int32) sample1 << shift);
			*raw++ = ((int32) sample2 << shift);
		}
		prev1 = *(raw - 2);
		prev0 = *(raw - 1);
		break;
    case 1:
		for (i = 8; i != 0; i--)
		{
			sample1 = *compressed++;
			sample2 = sample1 << 4;
			sample2 >>= 4;
			sample1 >>= 4;
			prev0 = (int16) prev0;
			*raw++ = prev1 = ((int32) sample1 << shift) + prev0 - (prev0 >> 4);
			prev1 = (int16) prev1;
			*raw++ = prev0 = ((int32) sample2 << shift) + prev1 - (prev1 >> 4);
		}
		break;
    case 2:
		for (i = 8; i != 0; i--)
		{
			sample1 = *compressed++;
			sample2 = sample1 << 4;
			sample2 >>= 4;
			sample1 >>= 4;
			
			out = (sample1 << shift) - prev1 + (prev1 >> 4);
			prev1 = (int16) prev0;
			prev0 &= ~3;
			*raw++ = prev0 = out + (prev0 << 1) - (prev0 >> 5) - 
				(prev0 >> 4);
			
			out = (sample2 << shift) - prev1 + (prev1 >> 4);
			prev1 = (int16) prev0;
			prev0 &= ~3;
			*raw++ = prev0 = out + (prev0 << 1) - (prev0 >> 5) -
				(prev0 >> 4);
		}
		break;
    case 3:
		for (i = 8; i != 0; i--)
		{
			sample1 = *compressed++;
			sample2 = sample1 << 4;
			sample2 >>= 4;
			sample1 >>= 4;
			out = (sample1 << shift);
			
			out = out - prev1 + (prev1 >> 3) + (prev1 >> 4);
			prev1 = (int16) prev0;
			prev0 &= ~3;
			*raw++ = prev0 = out + (prev0 << 1) - (prev0 >> 3) - 
				(prev0 >> 4) - (prev1 >> 6);
			
			out = (sample2 << shift);
			out = out - prev1 + (prev1 >> 3) + (prev1 >> 4);
			prev1 = (int16) prev0;
			prev0 &= ~3;
			*raw++ = prev0 = out + (prev0 << 1) - (prev0 >> 3) - 
				(prev0 >> 4) - (prev1 >> 6);
		}
		break;
    }
    ch->previous [0] = prev0;
    ch->previous [1] = prev1;
#endif
	
    ch->block_pointer += 9;
}

void AltDecodeBlock2 (Channel *ch)
{
    int32 out;
    unsigned char filter;
    unsigned char shift;
    signed char sample1, sample2;
    unsigned char i;
	
    if (ch->block_pointer > 0x10000 - 9)
    {
		ch->last_block = TRUE;
		ch->loop = FALSE;
		ch->block = ch->decoded;
		memset ((void *) ch->decoded, 0, sizeof (int16) * 16);
		return;
    }
	
    signed char *compressed = (signed char *) &IAPU.RAM [ch->block_pointer];
	
    filter = *compressed;
    if ((ch->last_block = filter & 1))
		ch->loop = (filter & 2) != 0;
	
    compressed++;
    signed short *raw = ch->block = ch->decoded;
    
    shift = filter >> 4;
    int32 prev0 = ch->previous [0];
    int32 prev1 = ch->previous [1];
	
    if(shift > 12)
		shift -= 4;
	
    switch ((filter >> 2) & 3)
    {
    case 0:
		for (i = 8; i != 0; i--)
		{
			sample1 = *compressed++;
			sample2 = sample1 << 4;
			//Sample 2 = Bottom Nibble, Sign Extended.
			sample2 >>= 4;
			//Sample 1 = Top Nibble, shifted down and Sign Extended.
			sample1 >>= 4;
			
			out = (int32)(sample1 << shift);
			
			prev1 = prev0;
			prev0 = out;
			CLIP16(out);
			*raw++ = (int16)out;
			
			out = (int32)(sample2 << shift);
			
			prev1 = prev0;
			prev0 = out;
			CLIP16(out);
			*raw++ = (int16)out;
		}
		break;
    case 1:
		for (i = 8; i != 0; i--)
		{
			sample1 = *compressed++;
			sample2 = sample1 << 4;
			//Sample 2 = Bottom Nibble, Sign Extended.
			sample2 >>= 4;
			//Sample 1 = Top Nibble, shifted down and Sign Extended.
			sample1 >>= 4;
			out = (int32)(sample1 << shift);
			out += (int32)((SysDDec)prev0 * 15/16);
			
			prev1 = prev0;
			prev0 = out;
			CLIP16(out);
			*raw++ = (int16)out;
			
			out = (int32)(sample2 << shift);
			out += (int32)((SysDDec)prev0 * 15/16);
			
			prev1 = prev0;
			prev0 = out;
			CLIP16(out);
			*raw++ = (int16)out;
		}
		break;
    case 2:
		for (i = 8; i != 0; i--)
		{
			sample1 = *compressed++;
			sample2 = sample1 << 4;
			//Sample 2 = Bottom Nibble, Sign Extended.
			sample2 >>= 4;
			//Sample 1 = Top Nibble, shifted down and Sign Extended.
			sample1 >>= 4;
			
			out = ((sample1 << shift) * 256 + (prev0 & ~0x2) * 488 - prev1 * 240) >> 8;
			
			prev1 = prev0;
			prev0 = (int16)out;
			*raw++ = (int16)out;
			
			out = ((sample2 << shift) * 256 + (prev0 & ~0x2) * 488 - prev1 * 240) >> 8;
			
			prev1 = prev0;
			prev0 = (int16)out;
			*raw++ = (int16)out;
		}
		break;
		
    case 3:
		for (i = 8; i != 0; i--)
		{
			sample1 = *compressed++;
			sample2 = sample1 << 4;
			//Sample 2 = Bottom Nibble, Sign Extended.
			sample2 >>= 4;
			//Sample 1 = Top Nibble, shifted down and Sign Extended.
			sample1 >>= 4;
			out = (int32)(sample1 << shift);
			out += (int32)((SysDDec)prev0 * 115/64 - (SysDDec)prev1 * 13/16);
			
			prev1 = prev0;
			prev0 = out;
			
			CLIP16(out);
			*raw++ = (int16)out;
			
			out = (int32)(sample2 << shift);
			out += (int32)((SysDDec)prev0 * 115/64 - (SysDDec)prev1 * 13/16);
			
			prev1 = prev0;
			prev0 = out;
			
			CLIP16(out);
			*raw++ = (int16)out;
		}
		break;
    }
    ch->previous [0] = prev0;
    ch->previous [1] = prev1;
    ch->block_pointer += 9;
}

void DecodeBlock (Channel *ch)
{
    int32 out;
    unsigned char filter;
    unsigned char shift;
    signed char sample1, sample2;
    unsigned char i;
    bool invalid_header;
	
    if (Settings.AltSampleDecode)
    {
		if (Settings.AltSampleDecode < 3)
			AltDecodeBlock (ch);
		else
			AltDecodeBlock2 (ch);
        return;
	}
    if (ch->block_pointer > 0x10000 - 9)
    {
		ch->last_block = TRUE;
		ch->loop = FALSE;
		ch->block = ch->decoded;
		return;
    }
    signed char *compressed = (signed char *) &IAPU.RAM [ch->block_pointer];
	
    filter = *compressed;
    if ((ch->last_block = filter & 1))
		ch->loop = (filter & 2) != 0;
	
	compressed++;
	signed short *raw = ch->block = ch->decoded;
	
	// Seperate out the header parts used for decoding

	shift = filter >> 4;
	
	// Header validity check: if range(shift) is over 12, ignore
	// all bits of the data for that block except for the sign bit of each
	invalid_header = !(shift < 0xD);

	filter = filter&0x0c;

	int32 prev0 = ch->previous [0];
	int32 prev1 = ch->previous [1];
	
	for (i = 8; i != 0; i--)
	{
		sample1 = *compressed++;
		sample2 = sample1 << 4;
		//Sample 2 = Bottom Nibble, Sign Extended.
		sample2 >>= 4;
		//Sample 1 = Top Nibble, shifted down and Sign Extended.
		sample1 >>= 4;
			if (invalid_header) { sample1>>=3; sample2>>=3; }
		
		for (int nybblesmp = 0; nybblesmp<2; nybblesmp++){
			out=(((nybblesmp) ? sample2 : sample1) << shift);
			out >>= 1;
			
			switch(filter)
			{
				case 0x00:
					// Method0 - [Smp]
					break;
				
				case 0x04:
					// Method1 - [Delta]+[Smp-1](15/16)
					out+=(prev0>>1)+((-prev0)>>5);
					break;
				
				case 0x08:
					// Method2 - [Delta]+[Smp-1](61/32)-[Smp-2](15/16)
					out+=(prev0)+((-(prev0 +(prev0>>1)))>>5)-(prev1>>1)+(prev1>>5);
					break;
				
				default:
					// Method3 - [Delta]+[Smp-1](115/64)-[Smp-2](13/16)
					out+=(prev0)+((-(prev0 + (prev0<<2) + (prev0<<3)))>>7)-(prev1>>1)+((prev1+(prev1>>1))>>4);
					break;
				
			}
			CLIP16(out);
				*raw++ = (signed short)(out<<1);
			prev1=(signed short)prev0;
			prev0=(signed short)(out<<1);
		}
	}
	ch->previous [0] = prev0;
	ch->previous [1] = prev1;
	
    ch->block_pointer += 9;
}


void MixStereo (int sample_count)
{
    static int wave[SOUND_BUFFER_SIZE];

    int pitch_mod = SoundData.pitch_mod & ~APU.DSP[APU_NON];
	
    for (uint32 J = 0; J < NUM_CHANNELS; J++) 
    {
		int32 VL, VR;
		Channel *ch = &SoundData.channels[J];
		unsigned long freq0 = ch->frequency;
		
		if (ch->state == SOUND_SILENT || !(so.sound_switch & (1 << J)))
			continue;
		
		//freq0 = (unsigned long) ((SysDDec) freq0 * 0.985);//uncommented by jonathan gevaryahu, as it is necessary for most cards in linux
		
		bool8 mod = pitch_mod & (1 << J);
		
		if (ch->needs_decode) 
		{
			DecodeBlock(ch);
			ch->needs_decode = FALSE;
			ch->sample = ch->block[0];
			ch->sample_pointer = freq0 >> FIXED_POINT_SHIFT;
			if (ch->sample_pointer == 0)
				ch->sample_pointer = 1;
			if (ch->sample_pointer > SOUND_DECODE_LENGTH)
				ch->sample_pointer = SOUND_DECODE_LENGTH - 1;

			ch->next_sample=ch->block[ch->sample_pointer];
			ch->interpolate = 0;
			  
			if (Settings.InterpolatedSound && freq0 < FIXED_POINT && !mod)
			   ch->interpolate = ((ch->next_sample - ch->sample) * 
			   (long) freq0) / (long) FIXED_POINT;
		}
		VL = (ch->sample * ch-> left_vol_level) / 128;
		VR = (ch->sample * ch->right_vol_level) / 128;
		
		for (uint32 I = 0; I < (uint32) sample_count; I += 2)
		{
			unsigned long freq = freq0;
			
			if (mod)
				freq = PITCH_MOD(freq, wave [I / 2]);
			
			ch->env_error += ch->erate;
			if (ch->env_error >= FIXED_POINT) 
			{
				uint32 step = ch->env_error >> FIXED_POINT_SHIFT;
				
				switch (ch->state)
				{
				case SOUND_ATTACK:
					ch->env_error &= FIXED_POINT_REMAINDER;
					ch->envx += step << 1;
					ch->envxx = ch->envx << ENVX_SHIFT;
					
					if (ch->envx >= 126)
					{
						ch->envx = 127;
						ch->envxx = 127 << ENVX_SHIFT;
						ch->state = SOUND_DECAY;
						if (ch->sustain_level != 8) 
						{
							S9xSetEnvRate (ch, ch->decay_rate, -1,
								(MAX_ENVELOPE_HEIGHT * ch->sustain_level)
								>> 3);
							break;
						}
						ch->state = SOUND_SUSTAIN;
						S9xSetEnvRate (ch, ch->sustain_rate, -1, 0);
					}
					break;
					
				case SOUND_DECAY:
					while (ch->env_error >= FIXED_POINT)
					{
						ch->envxx = (ch->envxx >> 8) * 255;
						ch->env_error -= FIXED_POINT;
					}
					ch->envx = ch->envxx >> ENVX_SHIFT;
					if (ch->envx <= ch->envx_target)
					{
						if (ch->envx <= 0)
						{
							S9xAPUSetEndOfSample (J, ch);
							goto stereo_exit;
						}
						ch->state = SOUND_SUSTAIN;
						S9xSetEnvRate (ch, ch->sustain_rate, -1, 0);
					}
					break;
					
				case SOUND_SUSTAIN:
					while (ch->env_error >= FIXED_POINT)
					{
						ch->envxx = (ch->envxx >> 8) * 255;
						ch->env_error -= FIXED_POINT;
					}
					ch->envx = ch->envxx >> ENVX_SHIFT;
					if (ch->envx <= 0)
					{
						S9xAPUSetEndOfSample (J, ch);
						goto stereo_exit;
					}
					break;
					
				case SOUND_RELEASE:
					while (ch->env_error >= FIXED_POINT)
					{
						ch->envxx -= (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
						ch->env_error -= FIXED_POINT;
					}
					ch->envx = ch->envxx >> ENVX_SHIFT;
					if (ch->envx <= 0)
					{
						S9xAPUSetEndOfSample (J, ch);
						goto stereo_exit;
					}
					break;
					
				case SOUND_INCREASE_LINEAR:
					ch->env_error &= FIXED_POINT_REMAINDER;
					ch->envx += step << 1;
					ch->envxx = ch->envx << ENVX_SHIFT;
					
					if (ch->envx >= 126)
					{
						ch->envx = 127;
						ch->envxx = 127 << ENVX_SHIFT;
						ch->state = SOUND_GAIN;
						ch->mode = MODE_GAIN;
						S9xSetEnvRate (ch, 0, -1, 0);
					}
					break;
					
				case SOUND_INCREASE_BENT_LINE:
					if (ch->envx >= (MAX_ENVELOPE_HEIGHT * 3) / 4)
					{
						while (ch->env_error >= FIXED_POINT)
						{
							ch->envxx += (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
							ch->env_error -= FIXED_POINT;
						}
						ch->envx = ch->envxx >> ENVX_SHIFT;
					}
					else
					{
						ch->env_error &= FIXED_POINT_REMAINDER;
						ch->envx += step << 1;
						ch->envxx = ch->envx << ENVX_SHIFT;
					}
					
					if (ch->envx >= 126)
					{
						ch->envx = 127;
						ch->envxx = 127 << ENVX_SHIFT;
						ch->state = SOUND_GAIN;
						ch->mode = MODE_GAIN;
						S9xSetEnvRate (ch, 0, -1, 0);
					}
					break;
					
				case SOUND_DECREASE_LINEAR:
					ch->env_error &= FIXED_POINT_REMAINDER;
					ch->envx -= step << 1;
					ch->envxx = ch->envx << ENVX_SHIFT;
					if (ch->envx <= 0)
					{
						S9xAPUSetEndOfSample (J, ch);
						goto stereo_exit;
					}
					break;
					
				case SOUND_DECREASE_EXPONENTIAL:
					while (ch->env_error >= FIXED_POINT)
					{
						ch->envxx = (ch->envxx >> 8) * 255;
						ch->env_error -= FIXED_POINT;
					}
					ch->envx = ch->envxx >> ENVX_SHIFT;
					if (ch->envx <= 0)
					{
						S9xAPUSetEndOfSample (J, ch);
						goto stereo_exit;
					}
					break;
					
				case SOUND_GAIN:
					S9xSetEnvRate (ch, 0, -1, 0);
					break;
		}
		ch-> left_vol_level = (ch->envx * ch->volume_left) / 128;
		ch->right_vol_level = (ch->envx * ch->volume_right) / 128;
		VL = (ch->sample * ch-> left_vol_level) / 128;
		VR = (ch->sample * ch->right_vol_level) / 128;
		}

		ch->count += freq;
		if (ch->count >= FIXED_POINT)
		{
			VL = ch->count >> FIXED_POINT_SHIFT;
			ch->sample_pointer += VL;
			ch->count &= FIXED_POINT_REMAINDER;

			ch->sample = ch->next_sample;
			if (ch->sample_pointer >= SOUND_DECODE_LENGTH)
			{
				if (JUST_PLAYED_LAST_SAMPLE(ch))
				{
					S9xAPUSetEndOfSample (J, ch);
					goto stereo_exit;
				}
				do
				{
					ch->sample_pointer -= SOUND_DECODE_LENGTH;
					if (ch->last_block)
					{
						if (!ch->loop)
						{
							ch->sample_pointer = LAST_SAMPLE;
							ch->next_sample = ch->sample;
							break;
						}
						else
						{
							S9xAPUSetEndX (J);
							ch->last_block = FALSE;
							uint16 *dir = S9xGetSampleAddress (ch->sample_number);
							ch->block_pointer = *(dir + 1);
						}
					}
					DecodeBlock (ch);
				} while (ch->sample_pointer >= SOUND_DECODE_LENGTH);
				if (!JUST_PLAYED_LAST_SAMPLE (ch))
					ch->next_sample = ch->block [ch->sample_pointer];
			}
			else
				ch->next_sample = ch->block [ch->sample_pointer];
			
			if (ch->type == SOUND_SAMPLE)
			{
				if (Settings.InterpolatedSound && freq < FIXED_POINT && !mod)
				{
					ch->interpolate = ((ch->next_sample - ch->sample) * 
					(long) freq) / (long) FIXED_POINT;
					ch->sample = (int16) (ch->sample + (((ch->next_sample - ch->sample) * 
					(long) (ch->count)) / (long) FIXED_POINT));
				}		  
				else
					ch->interpolate = 0;
			}
			else
			{
				for (;VL > 0; VL--)
					if ((so.noise_gen <<= 1) & 0x80000000L)
						so.noise_gen ^= 0x0040001L;
					ch->sample = (so.noise_gen << 17) >> 17;
					ch->interpolate = 0;
			}
			
			VL = (ch->sample * ch-> left_vol_level) / 128;
			VR = (ch->sample * ch->right_vol_level) / 128;
		}
		else
		{
			if (ch->interpolate)
			{
			int32 s = (int32) ch->sample + ch->interpolate;
			
			 CLIP16(s);
			 ch->sample = (int16) s;
			 VL = (ch->sample * ch-> left_vol_level) / 128;
			 VR = (ch->sample * ch->right_vol_level) / 128;
			 }
		}
		
		if (pitch_mod & (1 << (J + 1)))
			wave [I / 2] = ch->sample * ch->envx;
		
		MixBuffer [I      ^ Settings.ReverseStereo] += VL;
		MixBuffer [I + (1 ^ Settings.ReverseStereo)] += VR;
		ch->echo_buf_ptr [I      ^ Settings.ReverseStereo] += VL;
		ch->echo_buf_ptr [I + (1 ^ Settings.ReverseStereo)] += VR;
        }
stereo_exit: ;
    }
}

#ifdef __DJGPP
END_OF_FUNCTION(MixStereo);
#endif

void MixMono (int sample_count)
{
    static int wave[SOUND_BUFFER_SIZE];

    int pitch_mod = SoundData.pitch_mod & (~APU.DSP[APU_NON]);
	
    for (uint32 J = 0; J < NUM_CHANNELS; J++) 
    {
		Channel *ch = &SoundData.channels[J];
		unsigned long freq0 = ch->frequency;
		
		if (ch->state == SOUND_SILENT || !(so.sound_switch & (1 << J)))
			continue;
		
		//	freq0 = (unsigned long) ((double) freq0 * 0.985);
		
		bool8 mod = pitch_mod & (1 << J);
		
		if (ch->needs_decode) 
		{
			DecodeBlock(ch);
			ch->needs_decode = FALSE;
			ch->sample = ch->block[0];
			ch->sample_pointer = freq0 >> FIXED_POINT_SHIFT;
			if (ch->sample_pointer == 0)
				ch->sample_pointer = 1;
			if (ch->sample_pointer > SOUND_DECODE_LENGTH)
				ch->sample_pointer = SOUND_DECODE_LENGTH - 1;
			ch->next_sample = ch->block[ch->sample_pointer];
			ch->interpolate = 0;
			
			if (Settings.InterpolatedSound && freq0 < FIXED_POINT && !mod)
				ch->interpolate = ((ch->next_sample - ch->sample) * 
				(long) freq0) / (long) FIXED_POINT;
		}
		int32 V = (ch->sample * ch->left_vol_level) / 128;
		
		for (uint32 I = 0; I < (uint32) sample_count; I++)
		{
			unsigned long freq = freq0;
			
			if (mod)
				freq = PITCH_MOD(freq, wave [I]);
			
			ch->env_error += ch->erate;
			if (ch->env_error >= FIXED_POINT) 
			{
				uint32 step = ch->env_error >> FIXED_POINT_SHIFT;
				
				switch (ch->state)
				{
				case SOUND_ATTACK:
					ch->env_error &= FIXED_POINT_REMAINDER;
					ch->envx += step << 1;
					ch->envxx = ch->envx << ENVX_SHIFT;
					
					if (ch->envx >= 126)
					{
						ch->envx = 127;
						ch->envxx = 127 << ENVX_SHIFT;
						ch->state = SOUND_DECAY;
						if (ch->sustain_level != 8) 
						{
							S9xSetEnvRate (ch, ch->decay_rate, -1,
								(MAX_ENVELOPE_HEIGHT * ch->sustain_level)
								>> 3);
							break;
						}
						ch->state = SOUND_SUSTAIN;
						S9xSetEnvRate (ch, ch->sustain_rate, -1, 0);
					}
					break;
					
				case SOUND_DECAY:
					while (ch->env_error >= FIXED_POINT)
					{
						ch->envxx = (ch->envxx >> 8) * 255;
						ch->env_error -= FIXED_POINT;
					}
					ch->envx = ch->envxx >> ENVX_SHIFT;
					if (ch->envx <= ch->envx_target)
					{
						if (ch->envx <= 0)
						{
							S9xAPUSetEndOfSample (J, ch);
							goto mono_exit;
						}
						ch->state = SOUND_SUSTAIN;
						S9xSetEnvRate (ch, ch->sustain_rate, -1, 0);
					}
					break;
					
				case SOUND_SUSTAIN:
					while (ch->env_error >= FIXED_POINT)
					{
						ch->envxx = (ch->envxx >> 8) * 255;
						ch->env_error -= FIXED_POINT;
					}
					ch->envx = ch->envxx >> ENVX_SHIFT;
					if (ch->envx <= 0)
					{
						S9xAPUSetEndOfSample (J, ch);
						goto mono_exit;
					}
					break;
					
				case SOUND_RELEASE:
					while (ch->env_error >= FIXED_POINT)
					{
						ch->envxx -= (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
						ch->env_error -= FIXED_POINT;
					}
					ch->envx = ch->envxx >> ENVX_SHIFT;
					if (ch->envx <= 0)
					{
						S9xAPUSetEndOfSample (J, ch);
						goto mono_exit;
					}
					break;
					
				case SOUND_INCREASE_LINEAR:
					ch->env_error &= FIXED_POINT_REMAINDER;
					ch->envx += step << 1;
					ch->envxx = ch->envx << ENVX_SHIFT;
					
					if (ch->envx >= 126)
					{
						ch->envx = 127;
						ch->envxx = 127 << ENVX_SHIFT;
						ch->state = SOUND_GAIN;
						ch->mode = MODE_GAIN;
						S9xSetEnvRate (ch, 0, -1, 0);
					}
					break;
					
				case SOUND_INCREASE_BENT_LINE:
					if (ch->envx >= (MAX_ENVELOPE_HEIGHT * 3) / 4)
					{
						while (ch->env_error >= FIXED_POINT)
						{
							ch->envxx += (MAX_ENVELOPE_HEIGHT << ENVX_SHIFT) / 256;
							ch->env_error -= FIXED_POINT;
						}
						ch->envx = ch->envxx >> ENVX_SHIFT;
					}
					else
					{
						ch->env_error &= FIXED_POINT_REMAINDER;
						ch->envx += step << 1;
						ch->envxx = ch->envx << ENVX_SHIFT;
					}
					
					if (ch->envx >= 126)
					{
						ch->envx = 127;
						ch->envxx = 127 << ENVX_SHIFT;
						ch->state = SOUND_GAIN;
						ch->mode = MODE_GAIN;
						S9xSetEnvRate (ch, 0, -1, 0);
					}
					break;
					
				case SOUND_DECREASE_LINEAR:
					ch->env_error &= FIXED_POINT_REMAINDER;
					ch->envx -= step << 1;
					ch->envxx = ch->envx << ENVX_SHIFT;
					if (ch->envx <= 0)
					{
						S9xAPUSetEndOfSample (J, ch);
						goto mono_exit;
					}
					break;
					
				case SOUND_DECREASE_EXPONENTIAL:
					while (ch->env_error >= FIXED_POINT)
					{
						ch->envxx = (ch->envxx >> 8) * 255;
						ch->env_error -= FIXED_POINT;
					}
					ch->envx = ch->envxx >> ENVX_SHIFT;
					if (ch->envx <= 0)
					{
						S9xAPUSetEndOfSample (J, ch);
						goto mono_exit;
					}
					break;
					
				case SOUND_GAIN:
					S9xSetEnvRate (ch, 0, -1, 0);
					break;
		}
		ch->left_vol_level = (ch->envx * ch->volume_left) / 128;
		V = (ch->sample * ch->left_vol_level) / 128;
		}
		
		ch->count += freq;
		if (ch->count >= FIXED_POINT)
		{
			V = ch->count >> FIXED_POINT_SHIFT;
			ch->sample_pointer += V;
			ch->count &= FIXED_POINT_REMAINDER;
			
			ch->sample = ch->next_sample;
			if (ch->sample_pointer >= SOUND_DECODE_LENGTH)
			{
				if (JUST_PLAYED_LAST_SAMPLE(ch))
				{
					S9xAPUSetEndOfSample (J, ch);
					goto mono_exit;
				}
				do
				{
					ch->sample_pointer -= SOUND_DECODE_LENGTH;
					if (ch->last_block)
					{
						if (!ch->loop)
						{
							ch->sample_pointer = LAST_SAMPLE;
							ch->next_sample = ch->sample;
							break;
						}
						else
						{
							ch->last_block = FALSE;
							uint16 *dir = S9xGetSampleAddress (ch->sample_number);
							ch->block_pointer = *(dir + 1);
							S9xAPUSetEndX (J);
						}
					}
					DecodeBlock (ch);
				} while (ch->sample_pointer >= SOUND_DECODE_LENGTH);
				if (!JUST_PLAYED_LAST_SAMPLE (ch))
					ch->next_sample = ch->block [ch->sample_pointer];
			}
			else
				ch->next_sample = ch->block [ch->sample_pointer];
			
			if (ch->type == SOUND_SAMPLE)
			{
				if (Settings.InterpolatedSound && freq < FIXED_POINT && !mod)
				{
					ch->interpolate = ((ch->next_sample - ch->sample) * 
						(int) freq) / (int) FIXED_POINT;
					ch->sample = (int16) (ch->sample + (((ch->next_sample - ch->sample) * 
						(int) (ch->count)) / (int) FIXED_POINT));
				}		  
				else
					ch->interpolate = 0;
			}
			else
			{
				for (;V > 0; V--)
					if ((so.noise_gen <<= 1) & 0x80000000L)
						so.noise_gen ^= 0x0040001L;
					ch->sample = (so.noise_gen << 17) >> 17;
					ch->interpolate = 0;
			}
			V = (ch->sample * ch-> left_vol_level) / 128;
		}
		else
		{
			if (ch->interpolate)
			{
				int32 s = (int32) ch->sample + ch->interpolate;
				
				CLIP16(s);
				ch->sample = (int16) s;
				V = (ch->sample * ch-> left_vol_level) / 128;
			}
		}
		
		MixBuffer [I] += V;
		ch->echo_buf_ptr [I] += V;
		
		if (pitch_mod & (1 << (J + 1)))
			wave [I] = ch->sample * ch->envx;
        }
mono_exit: ;
    }
}
#ifdef __DJGPP
END_OF_FUNCTION(MixMono);
#endif

#ifdef __sun
extern uint8 int2ulaw (int);
#endif

// For backwards compatibility with older port specific code
void S9xMixSamplesO (uint8 *buffer, int sample_count, int byte_offset)
{
    S9xMixSamples (buffer+byte_offset, sample_count);
}
#ifdef __DJGPP
END_OF_FUNCTION(S9xMixSamplesO);
#endif

void S9xMixSamples (uint8 *buffer, int sample_count)
{
    int J;
    int I;
	
    if (!so.mute_sound)
    {
		memset (MixBuffer, 0, sample_count * sizeof (MixBuffer [0]));
		if (SoundData.echo_enable)
			memset (EchoBuffer, 0, sample_count * sizeof (EchoBuffer [0]));
		
		if (so.stereo)
			MixStereo (sample_count);
		else
			MixMono (sample_count);
    }
	
    /* Mix and convert waveforms */
    if (so.sixteen_bit)
    {
		int byte_count = sample_count << 1;
		
		// 16-bit sound
		if (so.mute_sound)
		{
            memset (buffer, 0, byte_count);
		}
		else
		{
			if (SoundData.echo_enable && SoundData.echo_buffer_size)
			{
				if (so.stereo)
				{
					// 16-bit stereo sound with echo enabled ...
					if (SoundData.no_filter)
					{
						// ... but no filter defined.
						for (J = 0; J < sample_count; J++)
						{
							int E = Echo [SoundData.echo_ptr];
							
							Echo [SoundData.echo_ptr] = (E * SoundData.echo_feedback) / 128 +
								EchoBuffer [J];
							
							if ((SoundData.echo_ptr += 1) >= SoundData.echo_buffer_size)
								SoundData.echo_ptr = 0;
							
							I = (MixBuffer [J] * 
								SoundData.master_volume [J & 1] +
								E * SoundData.echo_volume [J & 1]) / VOL_DIV16;
							
							CLIP16(I);
							((signed short *) buffer)[J] = I;
						}
					}
					else
					{
						// ... with filter defined.
						for (J = 0; J < sample_count; J++)
						{
							int E = Echo [SoundData.echo_ptr];
							
							Loop [(Z - 0) & 15] = E;
							E =  E                    * FilterTaps [0];
							E += Loop [(Z -  2) & 15] * FilterTaps [1];
							E += Loop [(Z -  4) & 15] * FilterTaps [2];
							E += Loop [(Z -  6) & 15] * FilterTaps [3];
							E += Loop [(Z -  8) & 15] * FilterTaps [4];
							E += Loop [(Z - 10) & 15] * FilterTaps [5];
							E += Loop [(Z - 12) & 15] * FilterTaps [6];
							E += Loop [(Z - 14) & 15] * FilterTaps [7];
							E /= 128;
							Z++;
							
							Echo [SoundData.echo_ptr] = (E * SoundData.echo_feedback) / 128 +
								EchoBuffer [J];
							
							if ((SoundData.echo_ptr += 1) >= SoundData.echo_buffer_size)
								SoundData.echo_ptr = 0;
							
							I = (MixBuffer [J] * 
								SoundData.master_volume [J & 1] +
								E * SoundData.echo_volume [J & 1]) / VOL_DIV16;
							
							CLIP16(I);
							((signed short *) buffer)[J] = I;
						}
					}
				}
				else
				{
					// 16-bit mono sound with echo enabled...
					if (SoundData.no_filter)
					{
						// ... no filter defined
						for (J = 0; J < sample_count; J++)
						{
							int E = Echo [SoundData.echo_ptr];
							
							Echo [SoundData.echo_ptr] = (E * SoundData.echo_feedback) / 128 +
								EchoBuffer [J];
							
							if ((SoundData.echo_ptr += 1) >= SoundData.echo_buffer_size)
								SoundData.echo_ptr = 0;
							
							I = (MixBuffer [J] *
								SoundData.master_volume [0] +
								E * SoundData.echo_volume [0]) / VOL_DIV16;
							CLIP16(I);
							((signed short *) buffer)[J] = I;
						}
					}
					else
					{
						// ... with filter defined
						for (J = 0; J < sample_count; J++)
						{
							int E = Echo [SoundData.echo_ptr];
							
							Loop [(Z - 0) & 7] = E;
							E =  E                  * FilterTaps [0];
							E += Loop [(Z - 1) & 7] * FilterTaps [1];
							E += Loop [(Z - 2) & 7] * FilterTaps [2];
							E += Loop [(Z - 3) & 7] * FilterTaps [3];
							E += Loop [(Z - 4) & 7] * FilterTaps [4];
							E += Loop [(Z - 5) & 7] * FilterTaps [5];
							E += Loop [(Z - 6) & 7] * FilterTaps [6];
							E += Loop [(Z - 7) & 7] * FilterTaps [7];
							E /= 128;
							Z++;
							
							Echo [SoundData.echo_ptr] = (E * SoundData.echo_feedback) / 128 +
								EchoBuffer [J];
							
							if ((SoundData.echo_ptr += 1) >= SoundData.echo_buffer_size)
								SoundData.echo_ptr = 0;
							
							I = (MixBuffer [J] * SoundData.master_volume [0] +
								E * SoundData.echo_volume [0]) / VOL_DIV16;
							CLIP16(I);
							((signed short *) buffer)[J] = I;
						}
					}
				}
		}
		else
		{
			// 16-bit mono or stereo sound, no echo
			for (J = 0; J < sample_count; J++)
			{
				I = (MixBuffer [J] * 
					SoundData.master_volume [J & 1]) / VOL_DIV16;
				
				CLIP16(I);
				((signed short *) buffer)[J] = I;
			}
		}
	}
    }
    else
    {
		// 8-bit sound
		if (so.mute_sound)
		{
            memset (buffer, 128, sample_count);
		}
		else
#ifdef __sun
			if (so.encoded)
			{
				for (J = 0; J < sample_count; J++)
				{
					I = (MixBuffer [J] * SoundData.master_volume_left) / VOL_DIV16;
					CLIP16(I);
					buffer[J] = int2ulaw (I);
				}
			}
			else
#endif
			{
				if (SoundData.echo_enable && SoundData.echo_buffer_size)
				{
					if (so.stereo)
					{
						// 8-bit stereo sound with echo enabled...
						if (SoundData.no_filter)
						{
							// ... but no filter
							for (J = 0; J < sample_count; J++)
							{
								int E = Echo [SoundData.echo_ptr];
								
								Echo [SoundData.echo_ptr] = (E * SoundData.echo_feedback) / 128 + 
									EchoBuffer [J];
								
								if ((SoundData.echo_ptr += 1) >= SoundData.echo_buffer_size)
									SoundData.echo_ptr = 0;
								
								I = (MixBuffer [J] * 
									SoundData.master_volume [J & 1] +
									E * SoundData.echo_volume [J & 1]) / VOL_DIV8;
								CLIP8(I);
								buffer [J] = I + 128;
							}
						}
						else
						{
							// ... with filter
							for (J = 0; J < sample_count; J++)
							{
								int E = Echo [SoundData.echo_ptr];
								
								Loop [(Z - 0) & 15] = E;
								E =  E                    * FilterTaps [0];
								E += Loop [(Z -  2) & 15] * FilterTaps [1];
								E += Loop [(Z -  4) & 15] * FilterTaps [2];
								E += Loop [(Z -  6) & 15] * FilterTaps [3];
								E += Loop [(Z -  8) & 15] * FilterTaps [4];
								E += Loop [(Z - 10) & 15] * FilterTaps [5];
								E += Loop [(Z - 12) & 15] * FilterTaps [6];
								E += Loop [(Z - 14) & 15] * FilterTaps [7];
								E /= 128;
								Z++;
								
								Echo [SoundData.echo_ptr] = (E * SoundData.echo_feedback) / 128 + 
									EchoBuffer [J];
								
								if ((SoundData.echo_ptr += 1) >= SoundData.echo_buffer_size)
									SoundData.echo_ptr = 0;
								
								I = (MixBuffer [J] * 
									SoundData.master_volume [J & 1] +
									E * SoundData.echo_volume [J & 1]) / VOL_DIV8;
								CLIP8(I);
								buffer [J] = I + 128;
							}
						}
					}
					else
					{
						// 8-bit mono sound with echo enabled...
						if (SoundData.no_filter)
						{
							// ... but no filter.
							for (J = 0; J < sample_count; J++)
							{
								int E = Echo [SoundData.echo_ptr];
								
								Echo [SoundData.echo_ptr] = (E * SoundData.echo_feedback) / 128 + 
									EchoBuffer [J];
								
								if ((SoundData.echo_ptr += 1) >= SoundData.echo_buffer_size)
									SoundData.echo_ptr = 0;
								
								I = (MixBuffer [J] * SoundData.master_volume [0] +
									E * SoundData.echo_volume [0]) / VOL_DIV8;
								CLIP8(I);
								buffer [J] = I + 128;
							}
						}
						else
						{
							// ... with filter.
							for (J = 0; J < sample_count; J++)
							{
								int E = Echo [SoundData.echo_ptr];
								
								Loop [(Z - 0) & 7] = E;
								E =  E                  * FilterTaps [0];
								E += Loop [(Z - 1) & 7] * FilterTaps [1];
								E += Loop [(Z - 2) & 7] * FilterTaps [2];
								E += Loop [(Z - 3) & 7] * FilterTaps [3];
								E += Loop [(Z - 4) & 7] * FilterTaps [4];
								E += Loop [(Z - 5) & 7] * FilterTaps [5];
								E += Loop [(Z - 6) & 7] * FilterTaps [6];
								E += Loop [(Z - 7) & 7] * FilterTaps [7];
								E /= 128;
								Z++;
								
								Echo [SoundData.echo_ptr] = (E * SoundData.echo_feedback) / 128 + 
									EchoBuffer [J];
								
								if ((SoundData.echo_ptr += 1) >= SoundData.echo_buffer_size)
									SoundData.echo_ptr = 0;
								
								I = (MixBuffer [J] * SoundData.master_volume [0] +
									E * SoundData.echo_volume [0]) / VOL_DIV8;
								CLIP8(I);
								buffer [J] = I + 128;
							}
						}
					}
		}
		else
		{
			// 8-bit mono or stereo sound, no echo
			for (J = 0; J < sample_count; J++)
			{
				I = (MixBuffer [J] * 
					SoundData.master_volume [J & 1]) / VOL_DIV8;
				CLIP8(I);
				buffer [J] = I + 128;
			}
		}
	}
    }
}

#ifdef __DJGPP
END_OF_FUNCTION(S9xMixSamples);
#endif

void S9xResetSound (bool8 full)
{
    for (int i = 0; i < 8; i++)
    {
		SoundData.channels[i].state = SOUND_SILENT;
		SoundData.channels[i].mode = MODE_NONE;
		SoundData.channels[i].type = SOUND_SAMPLE;
		SoundData.channels[i].volume_left = 0;
		SoundData.channels[i].volume_right = 0;
		SoundData.channels[i].hertz = 0;
		SoundData.channels[i].count = 0;
		SoundData.channels[i].loop = FALSE;
		SoundData.channels[i].envx_target = 0;
		SoundData.channels[i].env_error = 0;
		SoundData.channels[i].erate = 0;
		SoundData.channels[i].envx = 0;
		SoundData.channels[i].envxx = 0;
		SoundData.channels[i].left_vol_level = 0;
		SoundData.channels[i].right_vol_level = 0;
		SoundData.channels[i].direction = 0;
		SoundData.channels[i].attack_rate = 0;
		SoundData.channels[i].decay_rate = 0;
		SoundData.channels[i].sustain_rate = 0;
		SoundData.channels[i].release_rate = 0;
		SoundData.channels[i].sustain_level = 0;
		SoundData.echo_ptr = 0;
		SoundData.echo_feedback = 0;
		SoundData.echo_buffer_size = 1;
    }
    FilterTaps [0] = 127;
    FilterTaps [1] = 0;
    FilterTaps [2] = 0;
    FilterTaps [3] = 0;
    FilterTaps [4] = 0;
    FilterTaps [5] = 0;
    FilterTaps [6] = 0;
    FilterTaps [7] = 0;
    so.mute_sound = TRUE;
    so.noise_gen = 1;
    so.sound_switch = 255;
    so.samples_mixed_so_far = 0;
    so.play_position = 0;
    so.err_counter = 0;
	
    if (full)
    {
		SoundData.master_volume_left = 0;
		SoundData.master_volume_right = 0;
		SoundData.echo_volume_left = 0;
		SoundData.echo_volume_right = 0;
		SoundData.echo_enable = 0;
		SoundData.echo_write_enabled = 0;
		SoundData.echo_channel_enable = 0;
		SoundData.pitch_mod = 0;
		SoundData.dummy[0] = 0;
		SoundData.dummy[1] = 0;
		SoundData.dummy[2] = 0;
		SoundData.master_volume[0] = 0;
		SoundData.master_volume[1] = 0;
		SoundData.echo_volume[0] = 0;
		SoundData.echo_volume[1] = 0;
		SoundData.noise_hertz = 0;
    }
	
    SoundData.master_volume_left = 127;
    SoundData.master_volume_right = 127;
    SoundData.master_volume [0] = SoundData.master_volume [1] = 127;
    if (so.playback_rate)
		so.err_rate = (uint32) (FIXED_POINT * SNES_SCANLINE_TIME / (1.0 / so.playback_rate));
    else
		so.err_rate = 0;
    SoundData.no_filter = TRUE;
}

void S9xSetPlaybackRate (uint32 playback_rate)
{
    so.playback_rate = playback_rate;
    so.err_rate = (uint32) (SNES_SCANLINE_TIME * FIXED_POINT / (1.0 / (SysDDec) so.playback_rate));
    S9xSetEchoDelay (APU.DSP [APU_EDL] & 0xf);
    for (int i = 0; i < 8; i++)
		S9xSetSoundFrequency (i, SoundData.channels [i].hertz);
}

bool8 S9xInitSound (int mode, bool8 stereo, int buffer_size)
{
    so.sound_fd = -1;
    so.sound_switch = 255;
	
    so.playback_rate = 0;
    so.buffer_size = 0;
    so.stereo = stereo;
    //so.sixteen_bit = Settings.SixteenBitSound;
    so.encoded = FALSE;
    
    S9xResetSound (TRUE);
	
    if (!(mode & 7))
		return (1);
	
    S9xSetSoundMute (TRUE);
    /*if (!S9xOpenSoundDevice (mode, stereo, buffer_size))
    {
#ifdef NOSOUND
                S9xMessage (S9X_WARNING, S9X_SOUND_NOT_BUILT,
                            "No sound support compiled in");
#else
		S9xMessage (S9X_ERROR, S9X_SOUND_DEVICE_OPEN_FAILED,
                            "Sound device open failed");
#endif
		return (0);
    }*/
	
    return (1);
}

bool8 S9xSetSoundMode (int channel, int mode)
{
    Channel *ch = &SoundData.channels[channel];
	
    switch (mode)
    {
    case MODE_RELEASE:
		if (ch->mode != MODE_NONE)
		{
			ch->mode = MODE_RELEASE;
			return (TRUE);
		}
		break;
		
    case MODE_DECREASE_LINEAR:
    case MODE_DECREASE_EXPONENTIAL:
    case MODE_GAIN:
		if (ch->mode != MODE_RELEASE)
		{
			ch->mode = mode;
			if (ch->state != SOUND_SILENT)
				ch->state = mode;
			
			return (TRUE);
		}
		break;
		
    case MODE_INCREASE_LINEAR:
    case MODE_INCREASE_BENT_LINE:
		if (ch->mode != MODE_RELEASE)
		{
			ch->mode = mode;
			if (ch->state != SOUND_SILENT)
				ch->state = mode;
			
			return (TRUE);
		}
		break;
		
    case MODE_ADSR:
		if (ch->mode == MODE_NONE || ch->mode == MODE_ADSR)
		{
			ch->mode = mode;
			return (TRUE);
		}
    }
	
    return (FALSE);
}

void S9xSetSoundControl (int sound_switch)
{
    so.sound_switch = sound_switch;
}

void S9xPlaySample (int channel)
{
    Channel *ch = &SoundData.channels[channel];
    
    ch->state = SOUND_SILENT;
    ch->mode = MODE_NONE;
    ch->envx = 0;
    ch->envxx = 0;
	
    S9xFixEnvelope (channel,
		APU.DSP [APU_GAIN  + (channel << 4)], 
		APU.DSP [APU_ADSR1 + (channel << 4)],
		APU.DSP [APU_ADSR2 + (channel << 4)]);
	
    ch->sample_number = APU.DSP [APU_SRCN + channel * 0x10];
    if (APU.DSP [APU_NON] & (1 << channel))
		ch->type = SOUND_NOISE;
    else
		ch->type = SOUND_SAMPLE;
	
    S9xSetSoundFrequency (channel, ch->hertz);
    ch->loop = FALSE;
    ch->needs_decode = TRUE;
    ch->last_block = FALSE;
    ch->previous [0] = ch->previous[1] = 0;
    ch->block_pointer = *S9xGetSampleAddress (ch->sample_number);
    ch->sample_pointer = 0;
    ch->env_error = 0;
    ch->next_sample = 0;
    ch->interpolate = 0;
    switch (ch->mode)
    {
    case MODE_ADSR:
		if (ch->attack_rate == 0)
		{
			if (ch->decay_rate == 0 || ch->sustain_level == 8)
			{
				ch->state = SOUND_SUSTAIN;
				ch->envx = (MAX_ENVELOPE_HEIGHT * ch->sustain_level) >> 3;
				S9xSetEnvRate (ch, ch->sustain_rate, -1, 0);
			}
			else
			{
				ch->state = SOUND_DECAY;
				ch->envx = MAX_ENVELOPE_HEIGHT;
				S9xSetEnvRate (ch, ch->decay_rate, -1, 
					(MAX_ENVELOPE_HEIGHT * ch->sustain_level) >> 3);
			}
			ch-> left_vol_level = (ch->envx * ch->volume_left) / 128;
			ch->right_vol_level = (ch->envx * ch->volume_right) / 128;
		}
		else
		{
			ch->state = SOUND_ATTACK;
			ch->envx = 0;
			ch->left_vol_level = 0;
			ch->right_vol_level = 0;
			S9xSetEnvRate (ch, ch->attack_rate, 1, MAX_ENVELOPE_HEIGHT);
		}
		ch->envxx = ch->envx << ENVX_SHIFT;
		break;
		
    case MODE_GAIN:
		ch->state = SOUND_GAIN;
		break;
		
    case MODE_INCREASE_LINEAR:
		ch->state = SOUND_INCREASE_LINEAR;
		break;
		
    case MODE_INCREASE_BENT_LINE:
		ch->state = SOUND_INCREASE_BENT_LINE;
		break;
		
    case MODE_DECREASE_LINEAR:
		ch->state = SOUND_DECREASE_LINEAR;
		break;
		
    case MODE_DECREASE_EXPONENTIAL:
		ch->state = SOUND_DECREASE_EXPONENTIAL;
		break;
		
    default:
		break;
    }
	
    S9xFixEnvelope (channel,
		APU.DSP [APU_GAIN  + (channel << 4)], 
		APU.DSP [APU_ADSR1 + (channel << 4)],
		APU.DSP [APU_ADSR2 + (channel << 4)]);
}


