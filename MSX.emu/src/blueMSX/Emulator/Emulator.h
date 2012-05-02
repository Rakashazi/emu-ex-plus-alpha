/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Emulator/Emulator.h,v $
**
** $Revision: 1.12 $
**
** $Date: 2008-03-30 18:38:40 $
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
#ifndef EMULATOR_H
#define EMULATOR_H

#include "MsxTypes.h"
#include "Properties.h"
#include "AudioMixer.h"

typedef enum { EMU_RUNNING, EMU_PAUSED, EMU_STOPPED, EMU_SUSPENDED, EMU_STEP } EmuState;

void emulatorInit(Properties* properties, Mixer* mixer);
void emulatorExit();

void emuEnableSynchronousUpdate(int enable);

void emulatorSetFrequency(int logFrequency, int* frequency);
void emulatorRestartSound();
void emulatorSuspend();
void emulatorResume();
void emulatorDoResume();
void emulatorRestart();
void emulatorStart(const char* stateName);
void emulatorStop();
void emulatorSetMaxSpeed(int enable);
int  emulatorGetMaxSpeed();
void emulatorPlayReverse(int enable);
int  emulatorGetPlayReverse();
int emulatorGetCpuOverflow();
int emulatorGetSyncPeriod();
EmuState emulatorGetState();
void emulatorSetState(EmuState state);
UInt32 emulatorGetCpuSpeed();
UInt32 emulatorGetCpuUsage();
void emulatorResetMixer();
int emulatorGetCurrentScreenMode();

#endif

