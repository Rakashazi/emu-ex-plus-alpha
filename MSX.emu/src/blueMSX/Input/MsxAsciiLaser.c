/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Input/MsxAsciiLaser.c,v $
**
** $Revision: 1.14 $
**
** $Date: 2008-03-30 18:38:40 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik, NYYRIKKI
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


// TEST: 10 COLOR,15+STRIG(1):GOTO10

// R,G,B (MSX 512 colors, 0-7)
// 7,0,0 is not enough
// 0,4,0 is just enough
// 0,0,6 is just enough
// 7,3,5 is just enough

// 31, 0,  0
// 0,  17, 0
// 0,  0,  26
// 31, 13, 22

// r * 15 + g * 26 + b * 17 >= 442



#include "MsxAsciiLaser.h"
#include "InputEvent.h"
#include "ArchInput.h"
#include "FrameBuffer.h"
#include "Board.h"
#include "VDP.h"
#include <stdlib.h>


#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

struct MsxAsciiLaser {
    MsxJoystickDevice joyDevice;
    int scanlines;
    UInt32 lastTrigger;
};

#define RADIUS     5
#define DELAY      1
#define HOLD       9
#define AIMADJUST  (-3)
#define TRESHOLD   128

static UInt8 read(MsxAsciiLaser* joystick) {
    FrameBuffer* frameBuffer;
    UInt8 state = (~archMouseGetButtonState(0) & 1) << 5;
    int mx, my;

    vdpForceSync();

    archMouseGetState(&mx, &my);

    my = my * joystick->scanlines / 0x10000;
    
    frameBuffer = frameBufferGetDrawFrame(my);
    if (frameBuffer != NULL) {
        int scanline = frameBufferGetScanline();
        int myLow  = MAX(scanline - DELAY - HOLD - RADIUS, my + AIMADJUST - RADIUS);
        int myHigh = MIN(scanline - DELAY, my + AIMADJUST + RADIUS + HOLD);
        int y;

        joystick->scanlines = frameBufferGetLineCount(frameBuffer);

        
        myLow  = MAX(myLow, 0);
        myHigh = MIN(myHigh, frameBufferGetLineCount(frameBuffer));

        for (y = myLow; y < myHigh; y++) {
            int x = mx * (frameBufferGetDoubleWidth(frameBuffer, y) ? 2 : 1) * frameBufferGetMaxWidth(frameBuffer) / 0x10000;
            Pixel rgb = frameBufferGetLine(frameBuffer, y)[x];
            int R = 8 * ((rgb >> COLSHIFT_R) & COLMASK_R);
            int G = 8 * ((rgb >> COLSHIFT_G) & COLMASK_G);
            int B = 8 * ((rgb >> COLSHIFT_B) & COLMASK_B);
            int Y = (int)(0.2989*R + 0.5866*G + 0.1145*B);
        
            if (Y > TRESHOLD) {
                state |= 1 << 4;
                break;
            }
        }
    }

    return state;
}

MsxJoystickDevice* msxAsciiLaserCreate()
{
    MsxAsciiLaser* joystick = (MsxAsciiLaser*)calloc(1, sizeof(MsxAsciiLaser));
    joystick->joyDevice.read   = read;
    
    return (MsxJoystickDevice*)joystick;
}