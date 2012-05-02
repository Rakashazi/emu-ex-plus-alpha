/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/SoundChips/AY8910.h,v $
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
#ifndef AY8910_H
#define AY8910_H

#include "MsxTypes.h"
#include "AudioMixer.h"

/* Type definitions */
typedef struct AY8910 AY8910;

typedef enum { AY8910_MSX, AY8910_SVI } Ay8910Connector;

typedef enum { PSGTYPE_AY8910, PSGTYPE_YM2149, PSGTYPE_SN76489 } PsgType;

/* Constructor and destructor */
AY8910* ay8910Create(Mixer* mixer, Ay8910Connector connector, PsgType type, Int32 stereo, Int32* pan);
void ay8910Destroy(AY8910* ay8910);

/* Reset chip */
void ay8910Reset(AY8910* ay8910);

/* Register read/write methods */
void ay8910WriteAddress(AY8910* ay8910, UInt16 ioPort, UInt8 address);
UInt8 ay8910PeekData(AY8910* ay8910, UInt16 ioPort);
UInt8 ay8910ReadData(AY8910* ay8910, UInt16 ioPort);
void ay8910WriteData(AY8910* ay8910, UInt16 ioPort, UInt8 data);

typedef UInt8 (*AY8910ReadCb)(void*, UInt16);
typedef void (*AY8910WriteCb)(void*, UInt16, UInt8);

void ay8910SetIoPort(AY8910* ay8910, AY8910ReadCb readCb, AY8910ReadCb pollCb, AY8910WriteCb writeCb, void* arg);

void ay8910LoadState(AY8910* ay8910);
void ay8910SaveState(AY8910* ay8910);

#endif

