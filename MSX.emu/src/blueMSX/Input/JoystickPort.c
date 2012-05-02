/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Input/JoystickPort.c,v $
**
** $Revision: 1.10 $
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
#include "JoystickPort.h"
#include "ArchInput.h"
#include "Language.h"
#include "Board.h"
#include <stdlib.h>
#include <string.h>

typedef struct
{
    int              typeEnabled[JOYSTICK_MAX_PORTS][JOYSTICK_PORT_MAX_COUNT];
    JoystickPortType defaultType[JOYSTICK_MAX_PORTS];
    int              keyboardEnabled;
} JoystickConfig;

static JoystickPortUpdateHandler updateHandler = NULL;
static void* updateHandlerRef;
static JoystickPortType inputType[JOYSTICK_MAX_PORTS];

static JoystickConfig joystickConfig;

void joystickPortUpdateBoardInfo()
{
    int i;
    BoardType boardType = boardGetType();

    memset(&joystickConfig, 0, sizeof(joystickConfig));

    switch (boardType) {
    case BOARD_MSX:
        for (i = 0; i < 2; i++) {
            joystickConfig.typeEnabled[i][JOYSTICK_PORT_NONE] = 1;
            joystickConfig.typeEnabled[i][JOYSTICK_PORT_JOYSTICK] = 1;
            joystickConfig.typeEnabled[i][JOYSTICK_PORT_MOUSE] = 1;
            joystickConfig.typeEnabled[i][JOYSTICK_PORT_TETRIS2DONGLE] = 1;
            joystickConfig.typeEnabled[i][JOYSTICK_PORT_GUNSTICK] = 1;
            joystickConfig.typeEnabled[i][JOYSTICK_PORT_MAGICKEYDONGLE] = 1;
            joystickConfig.typeEnabled[i][JOYSTICK_PORT_ASCIILASER] = 1;
            joystickConfig.typeEnabled[i][JOYSTICK_PORT_ARKANOID_PAD] = 1;
            joystickConfig.defaultType[i] = JOYSTICK_PORT_NONE;
        }
        joystickConfig.keyboardEnabled = 1;
        break;

    case BOARD_SG1000:
    case BOARD_SVI:
        for (i = 0; i < 2; i++) {
            joystickConfig.typeEnabled[i][JOYSTICK_PORT_NONE] = 1;
            joystickConfig.typeEnabled[i][JOYSTICK_PORT_JOYSTICK] = 1;
            joystickConfig.defaultType[i] = JOYSTICK_PORT_NONE;
        }
        joystickConfig.keyboardEnabled = 1;
        break;

    case BOARD_COLECO:
        for (i = 0; i < 2; i++) {
            joystickConfig.typeEnabled[i][JOYSTICK_PORT_COLECOJOYSTICK] = 1;
            joystickConfig.typeEnabled[i][JOYSTICK_PORT_SUPERACTION] = 1;
            joystickConfig.typeEnabled[i][JOYSTICK_PORT_STEERINGWHEEL] = 1;
            joystickConfig.defaultType[i] = JOYSTICK_PORT_COLECOJOYSTICK;
        }
        joystickConfig.keyboardEnabled = 0;
        break;

    case BOARD_MSX_FORTE_II:
        joystickConfig.typeEnabled[0][JOYSTICK_PORT_JOYSTICK] = 1;
        joystickConfig.defaultType[0] = JOYSTICK_PORT_JOYSTICK;
        joystickConfig.keyboardEnabled = 1;
        break;
    }

    for (i = 0; i < JOYSTICK_MAX_PORTS; i++) {
        if (!joystickPortTypeEnabled(i, inputType[i])) {
            joystickPortSetType(i, joystickConfig.defaultType[i]);
        }
    }
}

int joystickPortKeyboardEnabled()
{
    return joystickConfig.keyboardEnabled;
}

int joystickPortTypeEnabled(int port, JoystickPortType type)
{
    return joystickConfig.typeEnabled[port][type];
}



void joystickPortSetType(int port, JoystickPortType type) 
{
    AmEnableMode mode;
    if (updateHandler != NULL && inputType[port] != type) {
        updateHandler(updateHandlerRef, port, type);
    }

    inputType[port] = type;

    mode = AM_DISABLE;

    if (inputType[0] == JOYSTICK_PORT_MOUSE || 
        inputType[0] == JOYSTICK_PORT_ARKANOID_PAD || 
        inputType[0] == JOYSTICK_PORT_COLECOJOYSTICK || 
        inputType[1] == JOYSTICK_PORT_MOUSE || 
        inputType[1] == JOYSTICK_PORT_ARKANOID_PAD || 
        inputType[1] == JOYSTICK_PORT_COLECOJOYSTICK)
    {
        mode = AM_ENABLE_MOUSE;
    }

    if (inputType[0] == JOYSTICK_PORT_GUNSTICK || 
        inputType[0] == JOYSTICK_PORT_ASCIILASER ||
        inputType[1] == JOYSTICK_PORT_GUNSTICK || 
        inputType[1] == JOYSTICK_PORT_ASCIILASER)
    {
        mode = AM_ENABLE_LASER;
    }

    archMouseEmuEnable(mode);
}

JoystickPortType joystickPortGetType(int port)
{
    return inputType[port];
}

void joystickPortUpdateHandlerRegister(JoystickPortUpdateHandler fn, void* ref)
{
    int port;
    updateHandler = fn;
    updateHandlerRef = ref;

    for (port = 0; port < JOYSTICK_MAX_PORTS; port++) {
        updateHandler(updateHandlerRef, port, inputType[port]);
    }
}

void joystickPortUpdateHandlerUnregister()
{
    updateHandler = NULL;
}

int joystickPortGetTypeCount()
{
    return JOYSTICK_PORT_MAX_COUNT;
}

char* joystickPortGetDescription(JoystickPortType type, int translate) 
{
    if (translate) {
        switch(type) {
        default:
        case JOYSTICK_PORT_NONE:            return langEnumControlsJoyNone();
        case JOYSTICK_PORT_JOYSTICK:        return langEnumControlsJoy2Button();
        case JOYSTICK_PORT_MOUSE:           return langEnumControlsJoyMouse();
        case JOYSTICK_PORT_TETRIS2DONGLE:   return langEnumControlsJoyTetrisDongle();
        case JOYSTICK_PORT_GUNSTICK:        return langEnumControlsJoyGunStick();
        case JOYSTICK_PORT_COLECOJOYSTICK:  return langEnumControlsJoyColeco();
        case JOYSTICK_PORT_MAGICKEYDONGLE:  return langEnumControlsJoyMagicKeyDongle();
        case JOYSTICK_PORT_ASCIILASER:      return langEnumControlsJoyAsciiLaser();
        case JOYSTICK_PORT_ARKANOID_PAD:    return langEnumControlsJoyArkanoidPad();
        case JOYSTICK_PORT_SUPERACTION:     return "Super Action Controller";
        case JOYSTICK_PORT_STEERINGWHEEL:   return "Expansion Module #2";
        }

        return langTextUnknown();
    }

    switch(type) {
    default:
    case JOYSTICK_PORT_NONE:            return "none";
    case JOYSTICK_PORT_JOYSTICK:        return "joystick";
    case JOYSTICK_PORT_MOUSE:           return "mouse";
    case JOYSTICK_PORT_TETRIS2DONGLE:   return "tetris2 dongle";
    case JOYSTICK_PORT_GUNSTICK:        return "gunstick";
    case JOYSTICK_PORT_COLECOJOYSTICK:  return "coleco joystick";
    case JOYSTICK_PORT_MAGICKEYDONGLE:  return "magic key dongle";
    case JOYSTICK_PORT_ASCIILASER:      return "ascii laser";
    case JOYSTICK_PORT_ARKANOID_PAD:    return "arkanoid pad";
    case JOYSTICK_PORT_SUPERACTION:     return "Super Action Controller";
    case JOYSTICK_PORT_STEERINGWHEEL:   return "Expansion Module #2";
    }

    return "unknown";
}

char* joystickPortTypeToName(int port, int translate)
{
    return joystickPortGetDescription(inputType[port], translate);
}

JoystickPortType joystickPortNameToType(int port, char* name, int translate)
{
    if (translate) {
        if (0 == strcmp(name, langEnumControlsJoy2Button())) return JOYSTICK_PORT_JOYSTICK;
        if (0 == strcmp(name, langEnumControlsJoyMouse())) return JOYSTICK_PORT_MOUSE;
        if (0 == strcmp(name, langEnumControlsJoyTetrisDongle())) return JOYSTICK_PORT_TETRIS2DONGLE;
        if (0 == strcmp(name, langEnumControlsJoyGunStick())) return JOYSTICK_PORT_GUNSTICK;
        if (0 == strcmp(name, langEnumControlsJoyColeco())) return JOYSTICK_PORT_COLECOJOYSTICK;
        if (0 == strcmp(name, langEnumControlsJoyMagicKeyDongle())) return JOYSTICK_PORT_MAGICKEYDONGLE;
        if (0 == strcmp(name, langEnumControlsJoyAsciiLaser())) return JOYSTICK_PORT_ASCIILASER;
        if (0 == strcmp(name, langEnumControlsJoyArkanoidPad())) return JOYSTICK_PORT_ARKANOID_PAD;
        if (0 == strcmp(name, "Super Action Controller")) return JOYSTICK_PORT_SUPERACTION;
        if (0 == strcmp(name, "Expansion Module #2")) return JOYSTICK_PORT_STEERINGWHEEL;

        return JOYSTICK_PORT_NONE;
    }

    if (0 == strcmp(name, "joystick")) return JOYSTICK_PORT_JOYSTICK;
    if (0 == strcmp(name, "mouse")) return JOYSTICK_PORT_MOUSE;
    if (0 == strcmp(name, "tetris2 dongle")) return JOYSTICK_PORT_TETRIS2DONGLE;
    if (0 == strcmp(name, "gunstick")) return JOYSTICK_PORT_GUNSTICK;
    if (0 == strcmp(name, "coleco joystick")) return JOYSTICK_PORT_COLECOJOYSTICK;
    if (0 == strcmp(name, "magic key dongle")) return JOYSTICK_PORT_MAGICKEYDONGLE;
    if (0 == strcmp(name, "ascii laser")) return JOYSTICK_PORT_ASCIILASER;
    if (0 == strcmp(name, "arkanoid pad")) return JOYSTICK_PORT_ARKANOID_PAD;
    if (0 == strcmp(name, "Super Action Controller")) return JOYSTICK_PORT_SUPERACTION;
    if (0 == strcmp(name, "Expansion Module #2")) return JOYSTICK_PORT_STEERINGWHEEL;

    return JOYSTICK_PORT_NONE;
}
