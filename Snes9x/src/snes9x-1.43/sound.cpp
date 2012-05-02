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
#include <devices/ahi.h>
#include <exec/exec.h>
#include <proto/ahi.h>
#include <proto/dos.h>
#include <proto/exec.h>
#include <clib/ahippc_protos.h>
#include <stdio.h>

#define EQ ==
#define MINBUFFLEN 10000

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "snes9x.h"
#include "soundux.h"

extern SoundStatus so;

extern int AudioOpen(unsigned long freq, unsigned long bufsize, unsigned long bitrate, unsigned long stereo);
extern void AudioClose(void);

extern int OpenPrelude(ULONG Type, ULONG DefaultFreq, ULONG MinBuffSize);
extern void ClosePrelude(void);

extern int SoundSignal;
unsigned long DoubleBuffer;
//extern struct AHISampleInfo Sample0;
//extern struct AHISampleInfo Sample1;
//extern unsigned long BufferSize;

struct Library    *AHIPPCBase;
struct Library    *AHIBase;
struct MsgPort    *AHImp=NULL;
struct AHIRequest *AHIio=NULL;
BYTE               AHIDevice=-1;

struct AHIData *AHIData;

unsigned long Frequency = 0;
//unsigned long BufferSize = 0;
unsigned long BitRate = 0;
unsigned long Stereo = 0;
//unsigned long AHIError = 9;

BYTE InternSignal=-1;

int mixsamples;
extern int prelude;

#define REALSIZE (BitRate*Stereo)

struct AHIAudioModeRequester *req=NULL;
struct AHIAudioCtrl *actrl=NULL;

ULONG BufferLen=NULL;


/* this really should be dynamically allocated... */
#undef  MAX_BUFFER_SIZE
#define MAX_BUFFER_SIZE 65536
#define MIN_BUFFER_SIZE 65536

#define MODE_MONO       0
#define MODE_STEREO     1

#define QUAL_8BIT       8
#define QUAL_16BIT      16


int test=0;
int test2=0;

int AudioOpen(unsigned long freq, unsigned long minbufsize, unsigned long bitrate, unsigned long stereo)
{
        ULONG Type;

        Frequency = freq;

    so.playback_rate = Frequency;
    
    if(stereo) so.stereo = TRUE;
    else so.stereo = FALSE;

        switch(bitrate)
        {
                case 8:
            so.sixteen_bit = FALSE;
                        BitRate=1;
                        if(stereo)
                        {
                                Stereo=2;
                                Type = AHIST_S8S;
                        }
                        else
                        {
                                Stereo=1;
                                Type = AHIST_M8S;
                        }

                break;

                default:        //defaulting to 16bit, because it means it won't crash atleast
                case QUAL_16BIT:
            so.sixteen_bit = TRUE;
                        BitRate=2;
                        if(stereo)
                        {
                                Stereo=2;
                                Type = AHIST_S16S;
                        }
                        else
                        {
                                Stereo=1;
                                Type = AHIST_M16S;
                        }
                break;
        }

    if(prelude) prelude = OpenPrelude(Type, freq, minbufsize);
    
    
    if(prelude) return 1; else printf("Defaulting to AHI...\n");

    /* only 1 channel right? */
    /* NOTE: The buffersize will not always be what you requested
     * it finds the minimun AHI requires and then rounds it up to
     * nearest 32 bytes.  Check AHIData->BufferSize or Samples[n].something_Length
     */
    if(AHIData = OpenAHI(1, Type, AHI_INVALID_ID, AHI_DEFAULT_FREQ, 0, minbufsize))
    {
        printf("AHI opened\n");
        printf("BuffSize %d\n", AHIData->BufferSize);
    }
    else
    {
        printf("AHI failed to open: %d\n", AHIData);
        return 0;
    }
    
    so.buffer_size = AHIData->BufferSize; // in bytes
        if (so.buffer_size > MAX_BUFFER_SIZE) so.buffer_size = MAX_BUFFER_SIZE;

    /* Lots of useful fields in the AHIData struct, have a look */
    AHIBase = AHIData->AHIBase;
    actrl = AHIData->AudioCtrl;
    Frequency = AHIData->MixingFreq;

        printf("signal %ld\n", AHIData->SoundFuncSignal);

        Wait(AHIData->SoundFuncSignal);

        /* I don't think it should start playing until there is something
         * In the buffer, however to set off the SoundFunc it should
         * probably go through the buffer at least once, just silently.
         */
        AHI_SetFreq(0, Frequency, actrl, AHISF_IMM);

        Wait(AHIData->SoundFuncSignal);

        AHI_SetVol(0, 0x10000, 0x8000, actrl, AHISF_IMM);

        mixsamples=AHIData->BufferSamples;

        SoundSignal = AHIData->SoundFuncSignal;
    
    return 1;
}

void AudioClose( void )
{
    if(prelude) ClosePrelude();
        else ;//CloseAHI(AHIData);
}


#include <wbstartup.h>

extern int main(int argc, char **argv);

void wbmain(struct WBStartup * argmsg)
{
 char argv[1][]={"WarpSNES"};
 int argc=1;
 main(argc,(char **)argv);
}


