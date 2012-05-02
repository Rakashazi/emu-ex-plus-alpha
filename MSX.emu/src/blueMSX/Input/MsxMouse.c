/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Input/MsxMouse.c,v $
**
** $Revision: 1.3 $
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
#include "MsxMouse.h"
#include "InputEvent.h"
#include "ArchInput.h"
#include "Board.h"
#include "SaveState.h"

#include <stdlib.h>

struct MsxMouse {
    MsxJoystickDevice joyDevice;
    int dx;
    int dy;
    int count;
    int mouseAsJoystick;
    UInt8 oldValue;
    UInt32 clock;
};

static void saveState(MsxMouse* mouse)
{
    SaveState* state = saveStateOpenForWrite("msxMouse");
    
    saveStateSet(state, "dx",               mouse->dx);
    saveStateSet(state, "dy",               mouse->dy);
    saveStateSet(state, "count",            mouse->count);
    saveStateSet(state, "mouseAsJoystick",  mouse->mouseAsJoystick);
    saveStateSet(state, "oldValue",         mouse->oldValue);
    saveStateSet(state, "clock",            mouse->clock);

    saveStateClose(state);
}

static void loadState(MsxMouse* mouse) 
{
    SaveState* state = saveStateOpenForRead("msxMouse");

    mouse->dx              =        saveStateGet(state, "dx",              0);
    mouse->dy              =        saveStateGet(state, "dy",              0);
    mouse->count           =        saveStateGet(state, "count",           0);
    mouse->mouseAsJoystick =        saveStateGet(state, "mouseAsJoystick", 0);
    mouse->oldValue        = (UInt8)saveStateGet(state, "oldValue",        0);
    mouse->clock           =        saveStateGet(state, "clock",           0);

    saveStateClose(state);
}

static UInt8 read(MsxMouse* mouse) 
{
    UInt8 state = 0x3f;
    UInt32 systemTime = boardSystemTime();

    if (mouse->mouseAsJoystick) {
        if (systemTime - mouse->clock > boardFrequency() / 120) {
            int dx;
            int dy;

            archMouseGetState(&dx, &dy);
            mouse->clock = systemTime;

            mouse->dx = (dx > 127 ? 127 : (dx < -127 ? -127 : dx));
            mouse->dy = (dy > 127 ? 127 : (dy < -127 ? -127 : dy));
        }

        if ((mouse->oldValue & 0x04) == 0) {
            state = ((mouse->dx / 3) ? ((mouse->dx > 0) ? 0x08 : 0x04) : 0x0c) |
                    ((mouse->dy / 3) ? ((mouse->dy > 0) ? 0x02 : 0x01) : 0x03);
        }
    }
    else {
        switch (mouse->count) {
        case 0:
            state = (mouse->dx >> 4) & 0x0f;
            break;
        case 1:
            state = mouse->dx & 0x0f;
            break;
        case 2: 
            state =(mouse->dy >> 4) & 0x0f;
            break;
        case 3: 
            state = mouse->dy & 0x0f;
            break;
        }
    }

    state |= (~archMouseGetButtonState(0) << 4) & 0x30;

    return state;
}

static void write(MsxMouse* mouse, UInt8 value) 
{
    UInt32 systemTime = boardSystemTime();

    if (mouse->mouseAsJoystick) {
        return;
    }

    if ((value ^ mouse->oldValue) & 0x04) {
        if (systemTime - mouse->clock > boardFrequency() / 2500) {
            mouse->count = 0;
        }
        else {
            mouse->count = (mouse->count + 1) & 3;
        }

        mouse->clock = systemTime;
        
        if (mouse->count == 0) {
            int dx;
            int dy;
            archMouseGetState(&dx, &dy);
            mouse->clock = systemTime;
            mouse->dx = (dx > 127 ? 127 : (dx < -127 ? -127 : dx));
            mouse->dy = (dy > 127 ? 127 : (dy < -127 ? -127 : dy));
        }
    }
    mouse->oldValue = value;
}

static void reset(MsxMouse* mouse) {
    mouse->dx       = 0;
    mouse->dy       = 0;
    mouse->count    = 0;
    mouse->clock    = 0;
    mouse->oldValue = 0;
}

MsxJoystickDevice* msxMouseCreate()
{
    MsxMouse* mouse = (MsxMouse*)calloc(1, sizeof(MsxMouse));
    mouse->joyDevice.read       = read;
    mouse->joyDevice.write      = write;
    mouse->joyDevice.reset      = reset;
    mouse->joyDevice.loadState  = loadState;
    mouse->joyDevice.saveState  = saveState;

    reset(mouse);
    
    return (MsxJoystickDevice*)mouse;
}