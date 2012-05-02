/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Input/SviJoyIo.c,v $
**
** $Revision: 1.5 $
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
#include "SviJoyIo.h"
#include "JoystickPort.h"
#include "InputEvent.h"
#include "DeviceManager.h"
#include "SaveState.h"
#include "MediaDb.h"

#include "SviJoystick.h"

#include <stdlib.h>

struct SviJoyIo {
    SviJoystickDevice* joyDevice[2];
    int joyDeviceHandle;
    UInt8 lastReadValue;
    int deviceHandle;
};

static void joyIoHandler(SviJoyIo* joyIo, int port, JoystickPortType type)
{
    if (port >= 2) {
        return;
    }

    if (joyIo->joyDevice[port] != NULL && joyIo->joyDevice[port]->destroy != NULL) {
        joyIo->joyDevice[port]->destroy(joyIo->joyDevice[port]);
    }
    
    switch (type) {
    default:
    case JOYSTICK_PORT_NONE:
        joyIo->joyDevice[port] = NULL;
        break;
    case JOYSTICK_PORT_JOYSTICK:
        joyIo->joyDevice[port] = sviJoystickCreate(port);
        break;
    }
}

static void loadState(SviJoyIo* joyIo)
{
    int i;
    for (i = 0; i < 2; i++) {
        if (joyIo->joyDevice[i] != NULL && joyIo->joyDevice[i]->loadState != NULL) {
            joyIo->joyDevice[i]->loadState(joyIo->joyDevice[i]);
        }
    }
}

static void saveState(SviJoyIo* joyIo)
{
    int i;
    for (i = 0; i < 2; i++) {
        if (joyIo->joyDevice[i] != NULL && joyIo->joyDevice[i]->saveState != NULL) {
            joyIo->joyDevice[i]->saveState(joyIo->joyDevice[i]);
        }
    }
}

static void reset(SviJoyIo* joyIo)
{
    int i;
    for (i = 0; i < 2; i++) {
        if (joyIo->joyDevice[i] != NULL && joyIo->joyDevice[i]->reset != NULL) {
            joyIo->joyDevice[i]->reset(joyIo->joyDevice[i]);
        }
    }
}

static void destroy(SviJoyIo* joyIo)
{
    int i;
    for (i = 0; i < 2; i++) {
        if (joyIo->joyDevice[i] != NULL && joyIo->joyDevice[i]->destroy != NULL) {
            joyIo->joyDevice[i]->destroy(joyIo->joyDevice[i]);
        }
    }

    joystickPortUpdateHandlerUnregister();

    deviceManagerUnregister(joyIo->deviceHandle);
}

UInt8 sviJoyIoRead(SviJoyIo* joyIo) {
    UInt8 value = 0xff;
    if (joyIo->joyDevice[0] != NULL && joyIo->joyDevice[0]->read != NULL) {
        value = (value & 0xf0) | (joyIo->joyDevice[0]->read(joyIo->joyDevice[0]) << 0);
    }
    if (joyIo->joyDevice[1] != NULL && joyIo->joyDevice[1]->read != NULL) {
        value = (value & 0x0f) | (joyIo->joyDevice[1]->read(joyIo->joyDevice[1]) << 4);
    }
    joyIo->lastReadValue = value;
    return value;
}

UInt8 sviJoyIoReadTrigger(SviJoyIo* joyIo) {
    UInt8 value = 0x3f;
    if (joyIo->joyDevice[0] != NULL && joyIo->joyDevice[0]->readTrigger != NULL) {
        value &= ~(joyIo->joyDevice[0]->readTrigger(joyIo->joyDevice[0]) << 4);
    }
    if (joyIo->joyDevice[1] != NULL && joyIo->joyDevice[1]->readTrigger != NULL) {
        value &= ~(joyIo->joyDevice[1]->readTrigger(joyIo->joyDevice[1]) << 5);
    }
    return value;
}

SviJoyIo* sviJoyIoCreate() 
{
    DeviceCallbacks callbacks = { destroy, reset, saveState, loadState };
    SviJoyIo* sviJoyIo = (SviJoyIo*)calloc(1, sizeof(SviJoyIo));

    joystickPortUpdateHandlerRegister(joyIoHandler, sviJoyIo);

    sviJoyIo->deviceHandle = deviceManagerRegister(ROM_UNKNOWN, &callbacks, sviJoyIo);

    return sviJoyIo;
}

