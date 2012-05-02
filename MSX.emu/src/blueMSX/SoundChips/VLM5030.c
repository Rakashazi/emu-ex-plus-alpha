/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/VLM5030.c,v $
**
** $Revision: 1.5 $
**
** $Date: 2008-03-31 19:42:23 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#include "VLM5030.h"
#include "MameVLM5030.h"
#include "Board.h"
#include "SaveState.h"
#include "MediaDb.h"
#include "DeviceManager.h"
#include <stdlib.h>
#include <string.h>

#define FREQUENCY        3579545
#define FREQINCR         (FREQUENCY / 440)

struct VLM5030 {
    Mixer* mixer;
    Int32  handle;
    
    Int32 timer;
    Int32 daVolume;
    Int32 sampleVolume;
    Int32 oldSampleVolume;
    Int32 ctrlVolume;
    
    Int32  buffer[AUDIO_MONO_BUFFER_SIZE];
};

static VLM5030* theVlm5030 = NULL;

void stream_update(void* dummy1, int idx)
{
    if (theVlm5030 != NULL) {
        mixerSync(theVlm5030->mixer);
    }
}

UInt8 vlm5030Peek(VLM5030* vlm5030, UInt16 ioPort)
{
    switch (ioPort & 1) {
    case 0:
        return VLM5030_BSY() ? 0x10 : 0;
    case 1:
        break;
    }
    return  0xff;
}

UInt8 vlm5030Read(VLM5030* vlm5030, UInt16 ioPort)
{
    switch (ioPort & 1) {
    case 0:
        return VLM5030_BSY() ? 0x10 : 0;
    case 1:
        break;
    }
    return  0xff;
}

void vlm5030Write(VLM5030* vlm5030, UInt16 ioPort, UInt8 value)
{
    switch (ioPort & 1) {
    case 0:
        mixerSync(vlm5030->mixer);
        VLM5030_data_w(0, value);
        break;
    case 1:
        mixerSync(vlm5030->mixer);
	    VLM5030_RST((value & 0x01) ? 1 : 0 );
	    VLM5030_VCU((value & 0x04) ? 1 : 0 );
	    VLM5030_ST( (value & 0x02) ? 1 : 0 );
        break;
    }
}

static Int32* vlm5030Sync(VLM5030* vlm5030, UInt32 count) 
{
    UInt32 i;

    for (i = 0; i < count; i++) {
        vlm5030->timer += FREQINCR;
        if (vlm5030->timer >= 44100) {
            vlm5030_update_callback(&vlm5030->sampleVolume, 1);
            vlm5030->sampleVolume *= 10;
            vlm5030->timer -= 44100;
        }

        /* Perform DC offset filtering */
        vlm5030->ctrlVolume = vlm5030->sampleVolume - vlm5030->oldSampleVolume + 0x3fe7 * vlm5030->ctrlVolume / 0x4000;
        vlm5030->oldSampleVolume = vlm5030->sampleVolume;

        /* Perform simple 1 pole low pass IIR filtering */
        vlm5030->daVolume += 2 * (vlm5030->ctrlVolume - vlm5030->daVolume) / 3;

        vlm5030->buffer[i] = vlm5030->daVolume;
    }

    return vlm5030->buffer;
}

void vlm5030SaveState(VLM5030* vlm5030)
{
    SaveState* state = saveStateOpenForWrite("vlm5030");
    
    saveStateSet(state, "timer",           vlm5030->timer);
    saveStateSet(state, "ctrlVolume",      vlm5030->ctrlVolume);
    saveStateSet(state, "oldSampleVolume", vlm5030->oldSampleVolume);
    saveStateSet(state, "sampleVolume",    vlm5030->sampleVolume);
    saveStateSet(state, "daVolume",        vlm5030->daVolume);

    saveStateClose(state);
}

void vlm5030LoadState(VLM5030* vlm5030)
{
    SaveState* state = saveStateOpenForRead("vlm5030");
    
    vlm5030->timer            = saveStateGet(state, "timer",           0);
    vlm5030->ctrlVolume       = saveStateGet(state, "ctrlVolume",      0);
    vlm5030->oldSampleVolume  = saveStateGet(state, "oldSampleVolume", 0);
    vlm5030->sampleVolume     = saveStateGet(state, "sampleVolume",    0);
    vlm5030->daVolume         = saveStateGet(state, "daVolume",        0);

    saveStateClose(state);
}

void vlm5030Destroy(VLM5030* vlm5030) 
{
    mixerUnregisterChannel(vlm5030->mixer, vlm5030->handle);

    theVlm5030 = NULL;

    free(vlm5030);
}

void vlm5030Reset(VLM5030* vlm5030)
{
    VLM5030_RST(0);
}

VLM5030* vlm5030Create(Mixer* mixer, UInt8* voiceData, int length)
{
    VLM5030* vlm5030;
    
    vlm5030 = (VLM5030*)calloc(1, sizeof(VLM5030));

    vlm5030->mixer = mixer;

    vlm5030->handle = mixerRegisterChannel(mixer, MIXER_CHANNEL_PCM, 0, vlm5030Sync, NULL, vlm5030);

    vlm5030_start(FREQUENCY);
    VLM5030_set_rom(voiceData, length);

    theVlm5030 = vlm5030;

    return vlm5030;
}
