/*****************************************************************************
** $Source: /cvsroot/bluemsx/blueMSX/Src/Input/MsxJoystick.c,v $
**
** $Revision: 1.5 $
**
** $Date: 2008/03/30 18:38:40 $
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
#include "MsxJoystick.h"
#include "InputEvent.h"

#include <stdlib.h>

struct MsxJoystick {
    MsxJoystickDevice joyDevice;
    int controller;
};

static UInt8 read(MsxJoystick* joystick) {
    UInt8 state;

    if (joystick->controller == 0) {
        state = (inputEventGetState(EC_JOY1_UP)      << 0) |
                (inputEventGetState(EC_JOY1_DOWN)    << 1) |
                (inputEventGetState(EC_JOY1_LEFT)    << 2) |
                (inputEventGetState(EC_JOY1_RIGHT)   << 3) |
                (inputEventGetState(EC_JOY1_BUTTON1) << 4) |
                (inputEventGetState(EC_JOY1_BUTTON2) << 5);
    }
    else {
        state = (inputEventGetState(EC_JOY2_UP)      << 0) |
                (inputEventGetState(EC_JOY2_DOWN)    << 1) |
                (inputEventGetState(EC_JOY2_LEFT)    << 2) |
                (inputEventGetState(EC_JOY2_RIGHT)   << 3) |
                (inputEventGetState(EC_JOY2_BUTTON1) << 4) |
                (inputEventGetState(EC_JOY2_BUTTON2) << 5);
    }

    return ~state & 0x3f;
}

void destroy(MsxJoystick* joystick)
{
    free(joystick);
}

MsxJoystickDevice* msxJoystickCreate(int controller)
{
    MsxJoystick* joystick = (MsxJoystick*)calloc(1, sizeof(MsxJoystick));
    joystick->joyDevice.read    = read;
    joystick->joyDevice.destroy = destroy;
    joystick->controller        = controller;
    
    return (MsxJoystickDevice*)joystick;
}
