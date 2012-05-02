/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Input/JoystickPort.h,v $
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
#ifndef JOYSTICK_PORT_H
#define JOYSTICK_PORT_H

#include "MsxTypes.h"

#define JOYSTICK_MAX_PORTS 2

typedef enum {
    JOYSTICK_PORT_NONE,
    JOYSTICK_PORT_JOYSTICK,
    JOYSTICK_PORT_MOUSE,
    JOYSTICK_PORT_TETRIS2DONGLE,
    JOYSTICK_PORT_GUNSTICK,
    JOYSTICK_PORT_COLECOJOYSTICK,
    JOYSTICK_PORT_MAGICKEYDONGLE,
    JOYSTICK_PORT_ASCIILASER,
    JOYSTICK_PORT_ARKANOID_PAD,
    JOYSTICK_PORT_SUPERACTION,
    JOYSTICK_PORT_STEERINGWHEEL,
    JOYSTICK_PORT_MAX_COUNT
} JoystickPortType;

typedef void (*JoystickPortUpdateHandler)(void*, int, JoystickPortType);

// Machine dependent config methods that probably belongs somewhere else...
void joystickPortUpdateBoardInfo();
int  joystickPortKeyboardEnabled();

int joystickPortGetTypeCount();

void joystickPortSetType(int port, JoystickPortType type);
JoystickPortType joystickPortGetType(int port);
int joystickPortTypeEnabled(int port, JoystickPortType type);

char* joystickPortGetDescription(JoystickPortType type, int translate);
JoystickPortType joystickPortNameToType(int port, char* name, int translate);
char* joystickPortTypeToName(int port, int translate);

void joystickPortUpdateHandlerRegister(JoystickPortUpdateHandler fn, void* ref);
void joystickPortUpdateHandlerUnregister();

#endif // JOYSTICK_PORT_H
