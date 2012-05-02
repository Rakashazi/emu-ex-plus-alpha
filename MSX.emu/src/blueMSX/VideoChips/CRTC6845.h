/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/VideoChips/CRTC6845.h,v $
**
** $Revision: 1.13 $
**
** $Date: 2008-03-31 19:42:23 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, Tomas Karlsson
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
#ifndef CRTC_H
#define CRTC_H

#include "MsxTypes.h"
#include "Board.h"
#include "VideoManager.h"

typedef struct
{
    int     mode;
    UInt8   rasterStart;
    UInt8   rasterEnd;
    UInt16  addressStart;
    int     blinkrate;
    UInt32  blinkstart;
} Crtc6845Cursor;

typedef struct
{
    UInt8    address;  // AR
    UInt8    reg[18];  // R0-R17
} Crtc6845Register;

typedef struct
{
    Crtc6845Cursor   cursor;
    Crtc6845Register registers;
    UInt32           frameCounter;
    int              frameRate;
    int              deviceHandle;
    int              debugHandle;
    int              videoHandle;
    int              videoEnabled;
    BoardTimer*      timerDisplay;   
    UInt32           timeDisplay;
    FrameBufferData* frameBufferData;
    UInt8*           vram;
    UInt32           vramMask;
    UInt8*           romData;
    UInt32           romMask;
    int              charWidth;
    int              charSpace;
    int              charsPerLine;
    int              displayWidth;
} CRTC6845;

UInt8 crtcRead(CRTC6845* crtc);
void crtcWrite(CRTC6845* crtc, UInt8 value);
void crtcWriteLatch(CRTC6845* crtc, UInt8 value);

void crtcMemWrite(CRTC6845* crtc, UInt16 address, UInt8 value);
UInt8 crtcMemRead(CRTC6845* crtc, UInt16 address);

CRTC6845* crtc6845Create(int frameRate, UInt8* romData, int size, int vramSize, 
                         int charWidth, int charSpace, int charsPerLine, int borderChars);

#endif
