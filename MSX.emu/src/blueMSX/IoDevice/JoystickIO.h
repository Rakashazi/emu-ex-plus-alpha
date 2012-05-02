/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/JoystickIO.h,v $
**
** $Revision: 1.7 $
**
** $Date: 2008-03-30 18:38:40 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2004 Daniel Vik
**
**  This software is provided 'as-is', without any express or implied
**  warranty.  In no event will the authors be held liable for any damages
**  arising from the use of this software.
**
**  Permission is granted to anyone to use this software for any purpose,
**  including commercial applications, and to alter it and redistribute it
**  freely, subject to the following restrictions:
**
**  1. The origin of this software must not be misrepresented; you must not
**     claim that you wrote the original software. If you use this software
**     in a product, an acknowledgment in the product documentation would be
**     appreciated but is not required.
**  2. Altered source versions must be plainly marked as such, and must not be
**     misrepresented as being the original software.
**  3. This notice may not be removed or altered from any source distribution.
**
******************************************************************************
*/
#ifndef JOYSTICK_IO_H
#define JOYSTICK_IO_H

#include "msxTypes.h"
#include "AY8910.h"

typedef enum { 
    JOYTYPE_NONE = 0, 
    JOYTYPE_JOYSTICK, 
    JOYTYPE_MOUSE,
    JOYTYPE_TETRIS2DONGLE
} JoystickIoType;

typedef struct JoystickIO JoystickIO;

JoystickIO* joystickIoCreate(AY8910* ay8910);
void joystickIoDestroy(JoystickIO* joyIO);

void joystickIoLoadState(JoystickIO* joyIO);
void joystickIoSaveState(JoystickIO* joyIO);

void joystickIoSetType(int port, JoystickIoType type, int userData);
JoystickIoType joystickIoGetType(int port, int* userData);

JoystickIO* joystickIoCreateSVI(void);
void joystickIoDestroySVI(JoystickIO* joyIO);
UInt8 joystickReadSVI(JoystickIO* joyIO);
UInt8 joystickPollSVI(JoystickIO* joyIO);
UInt8 joystickReadTriggerSVI(JoystickIO* joyIO);

JoystickIO* joystickIoCreateColeco(void);
void joystickIoDestroyColeco(JoystickIO* joyIO);
UInt8 joystickReadColeco(JoystickIO* joyIO, int port);
UInt8 joystickPollColeco(JoystickIO* joyIO, int port);

JoystickIO* joystickIoCreateSG1000(void);
void joystickIoDestroySG1000(JoystickIO* joyIO);
UInt8 joystickReadSG1000(JoystickIO* joyIO, int port);
UInt8 joystickPollSG1000(JoystickIO* joyIO, int port);

#endif

