/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Arch/ArchMidi.h,v $
**
** $Revision: 1.7 $
**
** $Date: 2008-03-31 19:42:19 $
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
#ifndef ARCH_MIDI_H
#define ARCH_MIDI_H

#include "MsxTypes.h"

typedef struct ArchMidi ArchMidi;

typedef void (*ArchMidiInCb)(void*, UInt8*, UInt32);

void archMidiEnable(int enable);
int  archMidiGetNoteOn();
void archMidiUpdateVolume(int left, int right);

void archMidiLoadState(void);
void archMidiSaveState(void);

ArchMidi* archMidiInCreate(int device, ArchMidiInCb cb, void* ref);
void archMidiInDestroy(ArchMidi* archMidi);
int archMidiInGetNoteOn(ArchMidi* archMidi, int note);

ArchMidi* archMidiOutCreate(int device);
void archMidiOutDestroy(ArchMidi* archMidi);
void archMidiOutTransmit(ArchMidi* archMidi, UInt8 value);

#endif
