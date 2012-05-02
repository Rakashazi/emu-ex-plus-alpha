/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/AudioMixer.h,v $
**
** $Revision: 1.14 $
**
** $Date: 2009-07-03 21:27:14 $
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
#ifndef AUDIO_MIXER_H
#define AUDIO_MIXER_H

#include "MsxTypes.h"

/* Type definitions */
typedef struct Mixer Mixer;

#define AUDIO_MONO_BUFFER_SIZE    10000
#define AUDIO_STEREO_BUFFER_SIZE  (2 * AUDIO_MONO_BUFFER_SIZE)

#define AUDIO_SAMPLERATE       44100

typedef enum { 
    MIXER_CHANNEL_PSG = 0,
    MIXER_CHANNEL_SCC,
    MIXER_CHANNEL_MSXMUSIC,
    MIXER_CHANNEL_MSXAUDIO,
    MIXER_CHANNEL_MOONSOUND,
    MIXER_CHANNEL_YAMAHA_SFG,
    MIXER_CHANNEL_KEYBOARD,
    MIXER_CHANNEL_PCM,
    MIXER_CHANNEL_IO,
    MIXER_CHANNEL_MIDI,
    MIXER_CHANNEL_TYPE_COUNT
} MixerAudioType;

typedef enum {
    MIXER_CHANNEL_LEFT = 0,
    MIXER_CHANNEL_RIGHT
} MixerChannelPan;

#define MAX_CHANNELS 16

typedef Int32* (*MixerUpdateCallback)(void*, UInt32);
typedef void (*MixerSetSampleRateCallback)(void*, UInt32);
typedef Int32 (*MixerWriteCallback)(void*, Int16*, UInt32);

/* Constructor and destructor */
Mixer* mixerCreate();
void mixerDestroy(Mixer* mixer);

Mixer* mixerGetGlobalMixer();

Int32 mixerGetMasterVolume(Mixer* mixer, int leftRight);
void mixerSetMasterVolume(Mixer* mixer, Int32 volume);
void mixerEnableMaster(Mixer* mixer, Int32 enable);
void mixerSetStereo(Mixer* mixer, Int32 stereo);
UInt32 mixerGetSampleRate(Mixer* mixer);
void mixerSetSampleRate(Mixer* mixer, UInt32 rate);

Int32 mixerGetChannelTypeVolume(Mixer* mixer, Int32 channelType, int leftRight);
void mixerSetChannelTypeVolume(Mixer* mixer, Int32 channelType, Int32 volume);
void mixerSetChannelTypePan(Mixer* mixer, Int32 channelType, Int32 pan);
void mixerEnableChannelType(Mixer* mixer, Int32 channelType, Int32 enable);
Int32 mixerIsChannelTypeActive(Mixer* mixer, Int32 channelType, Int32 reset);

/* Write callback registration for audio drivers */
void mixerSetWriteCallback(Mixer* mixer, MixerWriteCallback callback, void*, int);

/* File logging methods */
void mixerStartLog(Mixer* mixer, char* fileName);
int  mixerIsLogging(Mixer* mixer);
void mixerStopLog(Mixer* mixer);

/* Internal interface methods */
void mixerReset(Mixer* mixer);
void mixerSync(Mixer* mixer);

Int32 mixerRegisterChannel(Mixer* mixer, Int32 audioType, Int32 stereo, 
                           MixerUpdateCallback callback, MixerSetSampleRateCallback rateCallback,
                           void*param);
void mixerSetEnable(Mixer* mixer, int enable);
void mixerUnregisterChannel(Mixer* mixer, Int32 handle);

void mixerSetBoardFrequency(int CPUFrequency);
void mixerSetBoardFrequencyFixed(int CPUFrequency);

#endif

