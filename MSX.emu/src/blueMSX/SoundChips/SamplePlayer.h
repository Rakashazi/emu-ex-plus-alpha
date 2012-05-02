/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/SamplePlayer.h,v $
**
** $Revision: 1.9 $
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
#ifndef SAMPLE_PLAYER_H
#define SAMPLE_PLAYER_H

#include <stdio.h>

#include "MsxTypes.h"
#include "AudioMixer.h"

/* Type definitions */
typedef struct SamplePlayer SamplePlayer;

/* Constructor and destructor */
SamplePlayer* samplePlayerCreate(Mixer* mixer, int mixerChannel, int bitDepth, int frequency);
void samplePlayerDestroy(SamplePlayer* samplePlayer);
void samplePlayerReset(SamplePlayer* samplePlayer);

/* Register read/write methods */
void samplePlayerWrite(SamplePlayer* samplePlayer, 
                       const void* attackBuffer, UInt32 attackBufferSize, 
                       const void* loopBuffer, UInt32 loopBufferSize);
void samplePlayerStopAfter(SamplePlayer* samplePlayer, int loops);
int samplePlayerIsIdle(SamplePlayer* samplePlayer);
int samplePlayerIsLooping(SamplePlayer* samplePlayer);
void samplePlayerSetIndex(SamplePlayer* samplePlayer, int index);
int samplePlayerGetIndex(SamplePlayer* samplePlayer);
const void* samplePlayerGetAttackBuffer(SamplePlayer* samplePlayer);
const void* samplePlayerGetLoopBuffer(SamplePlayer* samplePlayer);
UInt32 samplePlayerGetAttackBufferSize(SamplePlayer* samplePlayer);
UInt32 samplePlayerGetLoopBufferSize(SamplePlayer* samplePlayer);
void samplePlayerDoSync(SamplePlayer* samplePlayer);

#endif

