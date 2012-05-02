/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/JoystickIO.c,v $
**
** $Revision: 1.16 $
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
#include "JoystickIO.h"
#include "Board.h"
#include "SaveState.h"
#include "Led.h"
#include "Switches.h"
#include "ArchInput.h"
#include <stdlib.h>
#include <string.h>

#define OFFSETOF(s, a) ((int)(&((s*)0)->a))


#if 0

UInt8 sviJoystickRead() {
    return ~((inputEventGetState(EK_JOY1_UP)    << 0) |
             (inputEventGetState(EK_JOY1_DOWN)  << 1) |
             (inputEventGetState(EK_JOY1_LEFT)  << 2) |
             (inputEventGetState(EK_JOY1_RIGHT) << 3) |
             (inputEventGetState(EK_JOY2_UP)    << 4) |
             (inputEventGetState(EK_JOY2_DOWN)  << 5) |
             (inputEventGetState(EK_JOY2_LEFT)  << 6) |
             (inputEventGetState(EK_JOY2_RIGHT) << 7));
}

UInt8 colecoJoystickRead(ColecoJoystick* colecoJoystick)
{
    if (colecoJoystick->port == 0) {
        return ~((inputEventGetState(EK_JOY1_UP)      << 0) |
                 (inputEventGetState(EK_JOY1_DOWN)    << 1) |
                 (inputEventGetState(EK_JOY1_LEFT)    << 2) |
                 (inputEventGetState(EK_JOY1_RIGHT)   << 3) |
                 (inputEventGetState(EK_JOY1_BUTTON1) << 4) |
                 (inputEventGetState(EK_JOY1_BUTTON2) << 5);
    }
    else {
        return ~((inputEventGetState(EK_JOY2_UP)      << 0) |
                 (inputEventGetState(EK_JOY2_DOWN)    << 1) |
                 (inputEventGetState(EK_JOY2_LEFT)    << 2) |
                 (inputEventGetState(EK_JOY2_RIGHT)   << 3) |
                 (inputEventGetState(EK_JOY2_BUTTON1) << 4) |
                 (inputEventGetState(EK_JOY2_BUTTON2) << 5);
    }
}

#endif


struct JoystickIO {
    AY8910* ay8910;
    struct {
        JoystickIoType type;
        UInt8 buttons;
        int dx;
        int dy;
        UInt8 count;
        UInt8 status;
        UInt32 joyClock;
    } controls[2];
    int mouseAsJoystick;
    UInt32 mouseClock;
    UInt32 data[4];
    UInt8 registers[2];
};

static JoystickIoType joyTypeConfigured[2];
static int joyTypeConfiguredUserData[2];
static int   isPolling = 1;

static void joystickCheckType(JoystickIO* joyIO) {
    int port;

    for (port = 0; port < 2; port++) {
        if (joyIO->controls[port].type != joyTypeConfigured[port]) {
            joyIO->controls[port].type = joyTypeConfigured[port];
            joyIO->controls[port].count = 0;
            joyIO->controls[port].status = 0x7f;
        }
    }
}

static UInt8 read(JoystickIO* joyIO, UInt16 address) 
{
    UInt32 systemTime = boardSystemTime();
    UInt8 value;
    int joyId;

    if (address & 1) {
        return joyIO->registers[1] & 0xf0;
    }

    joystickCheckType(joyIO);

    /* Number of a joystick port */
    joyId = joyIO->registers[1] & 0x40 ? 1 : 0;

    /* If no joystick, return dummy value */
    if (joyIO->controls[joyId].type == JOYTYPE_NONE) {
        return 0x7f;
    }

    if (joyIO->controls[joyId].type == JOYTYPE_TETRIS2DONGLE) {
        return joyIO->controls[joyId].status;
    }

    if (joyIO->controls[joyId].type == JOYTYPE_MOUSE) {
        int buttons = archMouseGetButtonState(0);
    
        joyIO->controls[joyId].buttons = (~buttons << 4) & 0x30;

        if (joyIO->controls[joyId].count == 0 && systemTime - joyIO->mouseClock > boardFrequency() / 120) 
        {
            static int dx;
            static int dy;

            if (!isPolling) {
                archMouseGetState(&dx, &dy);
                joyIO->mouseClock = systemTime;
            }

            joyIO->controls[joyId].dx = (dx > 127 ? 127 : (dx < -127 ? -127 : dx));
            joyIO->controls[joyId].dy = (dy > 127 ? 127 : (dy < -127 ? -127 : dy));
        }
    }

    switch (joyIO->controls[joyId].count) {
    case 0:
        if (joyIO->registers[1] & (joyId ? 0x20 : 0x10)) {
            return 0x7f;
        }

        if(joyIO->controls[joyId].type != JOYTYPE_MOUSE) {
            UInt8 state = archJoystickGetState((UInt8)joyId);
            int   renshaSpeed = switchGetRensha();
            if (renshaSpeed) {
                state &= ~((((UInt64)renshaSpeed * systemTime / boardFrequency()) & 1) << 4);
            }
            return 0x40 | (~state & 0x3f);
        }

        value =  joyIO->controls[joyId].buttons | 
            ((joyIO->controls[joyId].dx / 3)  ? 
                ((joyIO->controls[joyId].dx > 0) ? 0x08 : 0x04) : 0x0c) |
            ((joyIO->controls[joyId].dy / 3) ? 
                ((joyIO->controls[joyId].dy > 0) ? 0x02 : 0x01) : 0x03);
        return 0x40 | value;
    case 1: 
        return 0x40 | ((joyIO->controls[joyId].dx >> 4) & 0x0f) | joyIO->controls[joyId].buttons;
    case 2: 
        return 0x40 | (joyIO->controls[joyId].dx & 0x0f) | joyIO->controls[joyId].buttons;
    case 3: 
        return 0x40 | ((joyIO->controls[joyId].dy >> 4) & 0x0f) | joyIO->controls[joyId].buttons;
    case 4: 
        return 0x40 | (joyIO->controls[joyId].dy & 0x0f) | joyIO->controls[joyId].buttons;
    }

    return 0x7f;
}

static UInt8 poll(JoystickIO* joyIO, UInt16 address) 
{
    UInt8 rv;
    isPolling = 1;
    rv = read(joyIO, address);
    isPolling = 0;
    return rv;
}

static void write(JoystickIO* joyIO, UInt16 address, UInt8 value) 
{
    UInt32 systemTime = boardSystemTime();

    if (address & 1) {
        joystickCheckType(joyIO);

        if (joyIO->controls[0].type == JOYTYPE_TETRIS2DONGLE) {
            if (value & 0x02) {
        		joyIO->controls[0].status |= 0x08;
            }
            else {
        		joyIO->controls[0].status &= ~0x08;
            }
        }

        if (joyIO->controls[1].type == JOYTYPE_TETRIS2DONGLE) {
            if (value & 0x08) {
        		joyIO->controls[1].status |= 0x08;
            }
            else {
        		joyIO->controls[1].status &= ~0x08;
            }
        }

        if (joyIO->controls[1].type == JOYTYPE_MOUSE && !joyIO->mouseAsJoystick) {
            if ((value ^ joyIO->registers[1]) & 0x20) {
                if (systemTime - joyIO->controls[1].joyClock > boardFrequency() / 2500) {
                    joyIO->controls[1].count = 0;
                }
                joyIO->controls[1].joyClock = systemTime;

                joyIO->controls[1].count = 1 + (joyIO->controls[1].count & 3);
                
                if (joyIO->controls[1].count == 1) {
                    int dx;
                    int dy;
                    archMouseGetState(&dx, &dy);
                    joyIO->mouseClock = systemTime;
                    joyIO->controls[1].dx = (dx > 127 ? 127 : (dx < -127 ? -127 : dx));
                    joyIO->controls[1].dy = (dy > 127 ? 127 : (dy < -127 ? -127 : dy));
                }
            }
        }
        else {
            if ((value & 0x0c) == 0x0c) {
                joyIO->controls[1].count = 0;
            }
        }

        if (joyIO->controls[0].type == JOYTYPE_MOUSE && !joyIO->mouseAsJoystick) {
            if ((value ^ joyIO->registers[1]) & 0x10) {
                if (systemTime - joyIO->controls[0].joyClock > boardFrequency() / 2500) {
                    joyIO->controls[0].count = 0;
                }
                joyIO->controls[0].joyClock = systemTime;

                joyIO->controls[0].count = 1 + (joyIO->controls[0].count & 3);

                if (joyIO->controls[0].count == 1) {
                    int dx;
                    int dy;
                    archMouseGetState(&dx, &dy);
                    joyIO->mouseClock = systemTime;
                    joyIO->controls[0].dx = (dx > 127 ? 127 : (dx < -127 ? -127 : dx));
                    joyIO->controls[0].dy = (dy > 127 ? 127 : (dy < -127 ? -127 : dy));
                }
            }
        }
        else {
            if ((value & 0x03) == 0x03) {
                joyIO->controls[0].count = 0;
            }
        }

        ledSetKana(0 == (joyIO->registers[1] & 0x80));
    }

    joyIO->registers[address & 1] = value;
}

JoystickIO* joystickIoCreate(AY8910* ay8910)
{
    JoystickIO* joyIO;
    int buttons = archMouseGetButtonState(1);

    joyIO = calloc(1, sizeof(JoystickIO));

    joyIO->ay8910 = ay8910;
    joyIO->mouseAsJoystick = buttons & 1;

    ay8910SetIoPort(ay8910, read, poll, write, joyIO);

    ledSetKana(0);

    return joyIO;
}

void joystickIoDestroy(JoystickIO* joyIO)
{
    ay8910SetIoPort(joyIO->ay8910, NULL, NULL, NULL, NULL);

    free(joyIO);
}

void joystickIoSetType(int port, JoystickIoType type, int userData) {
    joyTypeConfiguredUserData[port] = userData;
    joyTypeConfigured[port] = type;
}

JoystickIoType joystickIoGetType(int port, int* userData) {
    *userData = joyTypeConfiguredUserData[port];
    return joyTypeConfigured[port];
}

void joystickIoSaveState(JoystickIO* joyIO)
{
    SaveState* state = saveStateOpenForWrite("joystickIo");

    joyIO->data[0] = joyTypeConfigured[0];
    joyIO->data[1] = joyTypeConfigured[1];
    joyIO->data[2] = joyTypeConfiguredUserData[0];
    joyIO->data[3] = joyTypeConfiguredUserData[1];

    saveStateSet(state, "ctrlType00",      joyIO->controls[0].type);
    saveStateSet(state, "ctrlButtons00",   joyIO->controls[0].buttons);
    saveStateSet(state, "ctrlDx00",        joyIO->controls[0].dx);
    saveStateSet(state, "ctrlDy00",        joyIO->controls[0].dy);
    saveStateSet(state, "ctrlCount00",     joyIO->controls[0].count);
    saveStateSet(state, "ctrlClock00",     joyIO->controls[0].joyClock);
    saveStateSet(state, "ctrlType01",      joyIO->controls[1].type);
    saveStateSet(state, "ctrlButtons01",   joyIO->controls[1].buttons);
    saveStateSet(state, "ctrlDx01",        joyIO->controls[1].dx);
    saveStateSet(state, "ctrlDy01",        joyIO->controls[1].dy);
    saveStateSet(state, "ctrlCount01",     joyIO->controls[1].count);
    saveStateSet(state, "ctrlClock01",     joyIO->controls[1].joyClock);
    saveStateSet(state, "mouseAsJoystick", joyIO->mouseAsJoystick);
    saveStateSet(state, "mouseClock",      joyIO->mouseClock);
    saveStateSet(state, "data00",          joyIO->data[0]);
    saveStateSet(state, "data01",          joyIO->data[1]);
    saveStateSet(state, "data02",          joyIO->data[2]);
    saveStateSet(state, "data03",          joyIO->data[3]);
    saveStateSet(state, "reg00",           joyIO->registers[0]);
    saveStateSet(state, "reg01",           joyIO->registers[1]);

    saveStateClose(state);
}

void joystickIoLoadState(JoystickIO* joyIO)
{
    SaveState* state = saveStateOpenForRead("joystickIo");

    joyIO->controls[0].type     =        saveStateGet(state, "ctrlType00",      0);
    joyIO->controls[0].buttons  = (UInt8)saveStateGet(state, "ctrlButtons00",   0);
    joyIO->controls[0].dx       =        saveStateGet(state, "ctrlDx00",        0);
    joyIO->controls[0].dy       =        saveStateGet(state, "ctrlDy00",        0);
    joyIO->controls[0].count    = (UInt8)saveStateGet(state, "ctrlCount00",     0);
    joyIO->controls[0].joyClock =        saveStateGet(state, "ctrlClock00",     0);
    joyIO->controls[1].type     =        saveStateGet(state, "ctrlType01",      0);
    joyIO->controls[1].buttons  = (UInt8)saveStateGet(state, "ctrlButtons01",   0);
    joyIO->controls[1].dx       =        saveStateGet(state, "ctrlDx01",        0);
    joyIO->controls[1].dy       =        saveStateGet(state, "ctrlDy01",        0);
    joyIO->controls[1].count    = (UInt8)saveStateGet(state, "ctrlCount01",     0);
    joyIO->controls[1].joyClock =        saveStateGet(state, "ctrlClock01",     0);
    joyIO->mouseAsJoystick      =        saveStateGet(state, "mouseAsJoystick", 0);
    joyIO->mouseClock           =        saveStateGet(state, "mouseClock",      0);
    joyIO->data[0]              =        saveStateGet(state, "data00",          0);
    joyIO->data[1]              =        saveStateGet(state, "data01",          0);
    joyIO->data[2]              =        saveStateGet(state, "data02",          0);
    joyIO->data[3]              =        saveStateGet(state, "data03",          0);
    joyIO->registers[0]         = (UInt8)saveStateGet(state, "reg00",           0);
    joyIO->registers[1]         = (UInt8)saveStateGet(state, "reg01",           0);

    saveStateClose(state);

    joyTypeConfigured[0]         = joyIO->data[0];
    joyTypeConfigured[1]         = joyIO->data[1];
    joyTypeConfiguredUserData[0] = joyIO->data[2];
    joyTypeConfiguredUserData[1] = joyIO->data[3];

    ledSetKana(0 == (joyIO->registers[1] & 0x80));
}

JoystickIO* joystickIoCreateSVI(void)
{
    JoystickIO* joyIO;
    int buttons = archMouseGetButtonState(1);

    joyIO = calloc(1, sizeof(JoystickIO));

    joyIO->mouseAsJoystick = buttons & 1;

    return joyIO;
}

void joystickIoDestroySVI(JoystickIO* joyIO)
{
    free(joyIO);
}

UInt8 joystickReadSVI(JoystickIO* joyIO) 
{
	UInt8 value;

	joyIO->registers[1] = 0x40;
	value = read(joyIO, 0) & 0x0F;
	value <<= 4;

	joyIO->registers[1] &= ~0x40;
	value |= read(joyIO, 0) & 0x0F;

	return value;
}

UInt8 joystickPollSVI(JoystickIO* joyIO) 
{
    UInt8 rv;
    isPolling = 1;
    rv = joystickReadSVI(joyIO);
    isPolling = 0;
    return rv;
}

UInt8 joystickReadTriggerSVI(JoystickIO* joyIO) 
{
	UInt8 value;

	joyIO->registers[1] = 0x40;
	value = read(joyIO, 0) & 0x10;
	value <<= 1;

	joyIO->registers[1] &= ~0x40;
	value |= read(joyIO, 0) & 0x10;

	return value;
}

JoystickIO* joystickIoCreateColeco(void)
{
    JoystickIO* joyIO;
    int buttons = archMouseGetButtonState(1);

    joyIO = calloc(1, sizeof(JoystickIO));

    joyIO->mouseAsJoystick = buttons & 1;

    return joyIO;
}

void joystickIoDestroyColeco(JoystickIO* joyIO)
{
    free(joyIO);
}

UInt8 joystickReadColeco(JoystickIO* joyIO, int port) 
{
    joyIO->registers[1] = port == 0 ? 0x40 : 0x00;

	return read(joyIO, 0) & 0x3f;
}

UInt8 joystickPollColeco(JoystickIO* joyIO, int port) 
{
    UInt8 rv;
    isPolling = 1;
    rv = joystickReadColeco(joyIO, port);
    isPolling = 0;
    return rv;
}

JoystickIO* joystickIoCreateSG1000(void)
{
    JoystickIO* joyIO;
    int buttons = archMouseGetButtonState(1);

    joyIO = calloc(1, sizeof(JoystickIO));

    joyIO->mouseAsJoystick = buttons & 1;

    return joyIO;
}

void joystickIoDestroySG1000(JoystickIO* joyIO)
{
    free(joyIO);
}

UInt8 joystickReadSG1000(JoystickIO* joyIO, int port) 
{
    joyIO->registers[1] = port == 0 ? 0x00 : 0x40;

	return read(joyIO, 0) & 0x3f;
}

UInt8 joystickPollSG1000(JoystickIO* joyIO, int port) 
{
    UInt8 rv;
    isPolling = 1;
    rv = joystickReadSG1000(joyIO, port);
    isPolling = 0;
    return rv;
}
