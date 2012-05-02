/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/KeyClick.c,v $
**
** $Revision: 1.7 $
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
#include "KeyClick.h"
#include <stdlib.h>
#include <string.h>


static Int32* audioKeyClickSync(void* ref, UInt32 count);

struct AudioKeyClick
{
    Mixer* mixer;
    Int32 handle;

    Int32 sampleVolume;
    Int32 sampleVolumeSum;
    Int32 oldSampleVolume;
    Int32 ctrlVolume;
    Int32 daVolume;
    Int32 count;

    Int32  buffer[AUDIO_MONO_BUFFER_SIZE];
};

AudioKeyClick* audioKeyClickCreate(Mixer* mixer)
{
    AudioKeyClick* keyClick = (AudioKeyClick*)calloc(1, sizeof(AudioKeyClick));

    keyClick->mixer = mixer;

    keyClick->handle = mixerRegisterChannel(mixer, MIXER_CHANNEL_KEYBOARD, 0, audioKeyClickSync, NULL, keyClick);

    return keyClick;
}

void audioKeyClickDestroy(AudioKeyClick* keyClick)
{
    mixerUnregisterChannel(keyClick->mixer, keyClick->handle);
    free(keyClick);
}

void audioKeyClick(AudioKeyClick* keyClick, UInt8 value)
{
    mixerSync(keyClick->mixer);
    keyClick->count++;
    keyClick->sampleVolumeSum += value ? 32000 : 0;
    keyClick->sampleVolume = value ? 32000 : 0;
}

static Int32* audioKeyClickSync(void* ref, UInt32 count)
{
    AudioKeyClick* keyClick = (AudioKeyClick*)ref;
    UInt32 index = 0;

    if (keyClick->count) {
        Int32 sampleVolume = keyClick->sampleVolumeSum / keyClick->count;
        keyClick->count = 0;
        keyClick->sampleVolumeSum = 0;
        keyClick->ctrlVolume = sampleVolume - keyClick->oldSampleVolume + 0x3fe7 * keyClick->ctrlVolume / 0x4000;
        keyClick->oldSampleVolume = sampleVolume;
        keyClick->ctrlVolume = 0x3fe7 * keyClick->ctrlVolume / 0x4000;

        /* Perform simple 1 pole low pass IIR filtering */
        keyClick->daVolume += 2 * (keyClick->ctrlVolume - keyClick->daVolume) / 3;
        keyClick->buffer[index++] = 8 * keyClick->daVolume;
    }

    keyClick->ctrlVolume = keyClick->sampleVolume - keyClick->oldSampleVolume + 0x3fe7 * keyClick->ctrlVolume / 0x4000;
    keyClick->oldSampleVolume = keyClick->sampleVolume;

    for (index; index < count; index++) {
        /* Perform DC offset filtering */
        keyClick->ctrlVolume = 0x3fe7 * keyClick->ctrlVolume / 0x4000;

        /* Perform simple 1 pole low pass IIR filtering */
        keyClick->daVolume += 2 * (keyClick->ctrlVolume - keyClick->daVolume) / 3;
        keyClick->buffer[index] = 7 * keyClick->daVolume;
    }

    return keyClick->buffer;
}
