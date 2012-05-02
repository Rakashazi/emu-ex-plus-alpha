/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/SamplePlayer.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2008-05-17 04:51:04 $
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
#include "SamplePlayer.h"
#include "Board.h"
#include <stdlib.h>
#include <string.h>


#define OFFSETOF(s, a) ((int)(&((s*)0)->a))

static Int32* samplePlayerSync(SamplePlayer* samplePlayer, UInt32 count);

struct SamplePlayer
{
    Mixer* mixer;
    Int32 handle;

    int   bitDepth;
    int   stepCnt;
    int   stepCur;

    Int32  enabled;
    const void* attackBuffer;
    UInt32 attackBufferSize;
    const void* loopBuffer;
    UInt32 loopBufferSize;
    int index;
    int playAttack;
    int stopCount;
    Int32  ctrlVolume;
    Int32  daVolume;

    Int32  defaultBuffer[AUDIO_MONO_BUFFER_SIZE];
    Int32  buffer[AUDIO_MONO_BUFFER_SIZE];
};

void samplePlayerReset(SamplePlayer* samplePlayer) {
    samplePlayer->ctrlVolume      = 0;
    samplePlayer->daVolume        = 0;
    samplePlayer->enabled         = 0;
}

SamplePlayer* samplePlayerCreate(Mixer* mixer, int mixerChannel, int bitDepth, int frequency)
{
    SamplePlayer* samplePlayer = (SamplePlayer*)calloc(1, sizeof(SamplePlayer));

    samplePlayer->mixer    = mixer;
    samplePlayer->bitDepth = bitDepth;
    samplePlayer->stepCur  = 0;
    samplePlayer->stepCnt  = 44100 / frequency;

    samplePlayerReset(samplePlayer);

    samplePlayer->handle = mixerRegisterChannel(mixer, mixerChannel, 0, samplePlayerSync, NULL, samplePlayer);

    return samplePlayer;
}

void samplePlayerDestroy(SamplePlayer* samplePlayer)
{
    mixerUnregisterChannel(samplePlayer->mixer, samplePlayer->handle);
    free(samplePlayer);
}


void samplePlayerStopAfter(SamplePlayer* samplePlayer, int loops)
{
    samplePlayer->stopCount = loops;
}

void samplePlayerSetIndex(SamplePlayer* samplePlayer, int index)
{
	if (samplePlayer->playAttack&&index>=(int)samplePlayer->attackBufferSize) index=0;
	else if (index>=(int)samplePlayer->loopBufferSize) index=0;
	samplePlayer->index=index;
}

int samplePlayerGetIndex(SamplePlayer* samplePlayer)
{
	return samplePlayer->index;
}

const void* samplePlayerGetAttackBuffer(SamplePlayer* samplePlayer)
{
	return samplePlayer->attackBuffer;
}

const void* samplePlayerGetLoopBuffer(SamplePlayer* samplePlayer)
{
	return samplePlayer->loopBuffer;
}

UInt32 samplePlayerGetAttackBufferSize(SamplePlayer* samplePlayer)
{
	return samplePlayer->attackBufferSize;
}

UInt32 samplePlayerGetLoopBufferSize(SamplePlayer* samplePlayer)
{
	return samplePlayer->loopBufferSize;
}

int samplePlayerIsIdle(SamplePlayer* samplePlayer)
{
    return !samplePlayer->enabled;
}

int samplePlayerIsLooping(SamplePlayer* samplePlayer)
{
    return !samplePlayer->playAttack;
}

void samplePlayerDoSync(SamplePlayer* samplePlayer)
{
	mixerSync(samplePlayer->mixer);
}

void samplePlayerWrite(SamplePlayer* samplePlayer, 
                       const void* attackBuffer, UInt32 attackBufferSize, 
                       const void* loopBuffer, UInt32 loopBufferSize)
{
    if (attackBuffer == NULL) {
        attackBuffer = loopBuffer;
        attackBufferSize = loopBufferSize;
    }

    samplePlayer->enabled = attackBuffer != NULL && attackBufferSize > 0;
    if (samplePlayer->enabled) {
        samplePlayer->attackBuffer = attackBuffer;
        samplePlayer->attackBufferSize = attackBufferSize;
        samplePlayer->loopBuffer = loopBuffer;
        samplePlayer->loopBufferSize = loopBufferSize;
        samplePlayer->playAttack = 1;
        samplePlayer->index = 0;
        samplePlayer->stopCount = loopBuffer != NULL ? 1 << 30 : 0;
    }
}

static Int32* samplePlayerSync(SamplePlayer* samplePlayer, UInt32 count)
{
    UInt32 index = 0;

    if (!samplePlayer->enabled) {
        return samplePlayer->defaultBuffer;
    }

    for (index = 0; index < count; index++) {
        Int32 sample = 0;
        if (samplePlayer->playAttack) {
            if (samplePlayer->bitDepth == 8)  sample = ((Int32)((UInt8*)samplePlayer->attackBuffer)[samplePlayer->index] - 0x80) * 256 * 9 / 10;
            if (samplePlayer->bitDepth == 16) sample = ((Int32)((Int16*)samplePlayer->attackBuffer)[samplePlayer->index]) * 9 / 10;
            if (++samplePlayer->stepCur >= samplePlayer->stepCnt) {
                if (++samplePlayer->index == samplePlayer->attackBufferSize) {
                    samplePlayer->index = 0;
                    samplePlayer->playAttack = 0;
                }
                samplePlayer->stepCur = 0;
            }
        }
        else if (samplePlayer->stopCount > 0) {
            if (samplePlayer->bitDepth == 8)  sample = ((Int32)((UInt8*)samplePlayer->loopBuffer)[samplePlayer->index] - 0x80) * 256 * 9 / 10;
            if (samplePlayer->bitDepth == 16) sample = ((Int32)((Int16*)samplePlayer->loopBuffer)[samplePlayer->index]) * 9 / 10;
            if (++samplePlayer->stepCur >= samplePlayer->stepCnt) {
                if (++samplePlayer->index == samplePlayer->loopBufferSize) {
                    samplePlayer->index = 0;
                    if (--samplePlayer->stopCount <= 0) {
                        samplePlayer->enabled = 0;
                    }
                }
                samplePlayer->stepCur = 0;
            }
        }
        else {
            samplePlayer->enabled = 0;
        }

        /* Perform simple 1 pole low pass IIR filtering */
        samplePlayer->daVolume += 2 * (sample - samplePlayer->daVolume) / 3;
        samplePlayer->buffer[index] = 6 * 9 * samplePlayer->daVolume / 10;
    }

    return samplePlayer->buffer;
}


