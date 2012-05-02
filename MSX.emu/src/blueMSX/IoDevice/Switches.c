/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/IoDevice/Switches.c,v $
**
** $Revision: 1.4 $
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
#include "Switches.h"


static int frontSwitchState = 0;
static int pauseSwitchState = 0;
static int audioSwitchState = 0;
static int renshaSpeed      = 0;


void switchSetRensha(int speed)
{
    if (speed) {
        renshaSpeed = (9 * speed + 30) / 8;
    }
    else {
        renshaSpeed = 0;
    }
}

int  switchGetRensha()
{
    return renshaSpeed;
}

void switchSetFront(int state)
{
    frontSwitchState = state;
}

int switchGetFront()
{
    return frontSwitchState;
}

void switchSetPause(int state)
{
    pauseSwitchState = state;
}

int switchGetPause()
{
    return pauseSwitchState;
}

void switchSetAudio(int state)
{
    audioSwitchState = state;
}

int switchGetAudio()
{
    return audioSwitchState;
}

