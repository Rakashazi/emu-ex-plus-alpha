/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/DAC.c,v $
**
** $Revision: 1.9 $
**
** $Date: 2008-03-30 18:38:45 $
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
#include "DAC.h"
#include "Board.h"
#include <stdlib.h>
#include <string.h>


#define OFFSETOF(s, a) ((int)(&((s*)0)->a))

static Int32* dacSyncMono(DAC* dac, UInt32 count);
static Int32* dacSyncStereo(DAC* dac, UInt32 count);
static void dacSyncChannel(DAC* dac, UInt32 count, int ch, UInt32 index, UInt32 delta);

struct DAC
{
    Mixer*  mixer;
    Int32   handle;
    DacMode mode;

    Int32   enabled;
    Int32   sampleVolume[2];
    Int32   oldSampleVolume[2];
    Int32   sampleVolumeSum[2];
    Int32   count[2];
    Int32   ctrlVolume[2];
    Int32   daVolume[2];

    Int32   defaultBuffer[AUDIO_STEREO_BUFFER_SIZE];
    Int32   buffer[AUDIO_STEREO_BUFFER_SIZE];
};

void dacReset(DAC* dac) {
    dac->oldSampleVolume[DAC_CH_LEFT]  = 0;
    dac->sampleVolume[DAC_CH_LEFT]     = 0;
    dac->ctrlVolume[DAC_CH_LEFT]       = 0;
    dac->daVolume[DAC_CH_LEFT]         = 0;
    dac->oldSampleVolume[DAC_CH_RIGHT] = 0;
    dac->sampleVolume[DAC_CH_RIGHT]    = 0;
    dac->ctrlVolume[DAC_CH_RIGHT]      = 0;
    dac->daVolume[DAC_CH_RIGHT]        = 0;
}

DAC* dacCreate(Mixer* mixer, DacMode mode)
{
    DAC* dac = (DAC*)calloc(1, sizeof(DAC));

    dac->mixer = mixer;
    dac->mode  = mode;

    dacReset(dac);

    if (mode == DAC_MONO) {
        dac->handle = mixerRegisterChannel(mixer, MIXER_CHANNEL_PCM, 0, dacSyncMono, NULL, dac);
    }
    else {
        dac->handle = mixerRegisterChannel(mixer, MIXER_CHANNEL_PCM, 1, dacSyncStereo, NULL, dac);
    }
    return dac;
}

void dacDestroy(DAC* dac)
{
    mixerUnregisterChannel(dac->mixer, dac->handle);
    free(dac);
}

void dacWrite(DAC* dac, DacChannel channel, UInt8 value)
{
    if (channel == DAC_CH_LEFT || channel == DAC_CH_RIGHT) {
        Int32 sampleVolume = ((Int32)value - 0x80) * 256;
        mixerSync(dac->mixer);
        dac->sampleVolume[channel]     = sampleVolume;
        dac->sampleVolumeSum[channel] += sampleVolume;
        dac->count[channel]++;
        dac->enabled = 1;
    }
}

static Int32* dacSyncMono(DAC* dac, UInt32 count)
{
    if (!dac->enabled || count == 0) {
        return dac->defaultBuffer;
    }

    dacSyncChannel(dac, count, DAC_CH_MONO, 0, 1);

    dac->enabled = dac->buffer[count - 1] != 0;

    return dac->buffer;
}

static Int32* dacSyncStereo(DAC* dac, UInt32 count)
{
    if (!dac->enabled || count == 0) {
        return dac->defaultBuffer;
    }

    dacSyncChannel(dac, count, DAC_CH_LEFT,  0, 2);
    dacSyncChannel(dac, count, DAC_CH_RIGHT, 1, 2);

    dac->enabled = dac->buffer[2 * count - 1] != 0 ||
                   dac->buffer[2 * count - 2] != 0;

    return dac->buffer;
}

static void dacSyncChannel(DAC* dac, UInt32 count, int ch, UInt32 index, UInt32 delta)
{
    count *= delta;

    if (dac->count[ch] > 0) {
        Int32 sampleVolume = dac->sampleVolumeSum[ch] / dac->count[ch];
        dac->count[ch] = 0;
        dac->sampleVolumeSum[ch] = 0;
        dac->ctrlVolume[ch] = sampleVolume - dac->oldSampleVolume[ch] + 0x3fe7 * dac->ctrlVolume[ch] / 0x4000;
        dac->oldSampleVolume[ch] = sampleVolume;
        dac->ctrlVolume[ch] = 0x3fe7 * dac->ctrlVolume[ch] / 0x4000;

        dac->daVolume[ch] += 2 * (dac->ctrlVolume[ch] - dac->daVolume[ch]) / 3;
        dac->buffer[index] = 6 * 9 * dac->daVolume[ch] / 10;
        index += delta;
    }

    dac->ctrlVolume[ch] = dac->sampleVolume[ch] - dac->oldSampleVolume[ch] + 0x3fe7 * dac->ctrlVolume[ch] / 0x4000;
    dac->oldSampleVolume[ch] = dac->sampleVolume[ch];

    for (; index < count; index += delta) {
        /* Perform DC offset filtering */
        dac->ctrlVolume[ch] = 0x3fe7 * dac->ctrlVolume[ch] / 0x4000;

        /* Perform simple 1 pole low pass IIR filtering */
        dac->daVolume[ch] += 2 * (dac->ctrlVolume[ch] - dac->daVolume[ch]) / 3;
        dac->buffer[index] = 6 * 9 * dac->daVolume[ch] / 10;
    }
}

