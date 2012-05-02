/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Input/MsxGunstick.c,v $
**
** $Revision: 1.9 $
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
#include "MsxGunstick.h"
#include "InputEvent.h"
#include "ArchInput.h"
#include "FrameBuffer.h"
#include "VDP.h"

#include <stdlib.h>

#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define MAX(a,b) ((a) > (b) ? (a) : (b))

struct MsxGunstick {
    MsxJoystickDevice joyDevice;
    int scanlines;
};

#define SENSITIVIY 24
#define DELAY      64
#define TRESHOLD   128

static UInt8 read(MsxGunstick* joystick) {
    FrameBuffer* frameBuffer;
    UInt8 state = (archMouseGetButtonState(0) & 1) << 4;
    int mx, my;

    vdpForceSync();

    archMouseGetState(&mx, &my);

    my = my * joystick->scanlines / 0x10000;
    
    frameBuffer = frameBufferGetDrawFrame(my);

    if (frameBuffer != NULL) {
        int scanline = frameBufferGetScanline();
        int myLow  = MAX(scanline - DELAY, my - SENSITIVIY);
        int myHigh = MIN(scanline, my);
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
                state |= 1 << 1;
                break;
            }
        }
    }

    return ~state & 0x3f;
}

MsxJoystickDevice* msxGunstickCreate()
{
    MsxGunstick* joystick = (MsxGunstick*)calloc(1, sizeof(MsxGunstick));
    joystick->joyDevice.read   = read;
    
    return (MsxJoystickDevice*)joystick;
}