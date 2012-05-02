/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/DeviceManager.c,v $
**
** $Revision: 1.4 $
**
** $Date: 2008-03-30 18:38:42 $
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
#include "DeviceManager.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_DEVICES 64

typedef struct {
    int handle;
    DeviceCallbacks callbacks;
    void* ref;
    int type;
} DeviceInfo;

typedef struct {
    DeviceInfo di[MAX_DEVICES];
    int count;
    int lastHandle;
    int shutDown;
} DeviceManager;

static DeviceManager deviceManager;

void deviceManagerCreate() {
    deviceManager.count = 0;
    deviceManager.lastHandle = 0;
    deviceManager.shutDown = 0;
}

void deviceManagerDestroy()
{
    int i;

    deviceManager.shutDown = 1;
    
    for (i = 0; i < deviceManager.count; i++) {
        if (deviceManager.di[i].callbacks.destroy != NULL) {
            deviceManager.di[i].callbacks.destroy(deviceManager.di[i].ref);
        }
    }
}

void deviceManagerReset()
{
    int i;
    for (i = 0; i < deviceManager.count; i++) {
        if (deviceManager.di[i].callbacks.reset != NULL) {
            deviceManager.di[i].callbacks.reset(deviceManager.di[i].ref);
        }
    }
}

void deviceManagerLoadState()
{
    int i;
    for (i = 0; i < deviceManager.count; i++) {
        if (deviceManager.di[i].callbacks.loadState != NULL) {
            deviceManager.di[i].callbacks.loadState(deviceManager.di[i].ref);
        }
    }
}

void deviceManagerSaveState()
{
    int i;
    for (i = 0; i < deviceManager.count; i++) {
        if (deviceManager.di[i].callbacks.saveState != NULL) {
            deviceManager.di[i].callbacks.saveState(deviceManager.di[i].ref);
        }
    }
}

int deviceManagerRegister(int type, DeviceCallbacks* callbacks, void* ref)
{
    if (deviceManager.count >= MAX_DEVICES) {
        return 0;
    }

    deviceManager.di[deviceManager.count].handle    = ++deviceManager.lastHandle;
    deviceManager.di[deviceManager.count].type      = type;
    deviceManager.di[deviceManager.count].callbacks = *callbacks;
    deviceManager.di[deviceManager.count].ref       = ref;

    deviceManager.count++;

    return deviceManager.lastHandle;
}

void deviceManagerUnregister(int handle)
{
    int i;

    if (deviceManager.count == 0 || deviceManager.shutDown) {
        return;
    }

    for (i = 0; i < deviceManager.count; i++) {
        if (deviceManager.di[i].handle == handle) {
            break;
        }
    }

    if (i == deviceManager.count) {
        return;
    }

    deviceManager.count--;
    while (i < deviceManager.count) {
        deviceManager.di[i] = deviceManager.di[i + 1];
        i++;
    }
}
