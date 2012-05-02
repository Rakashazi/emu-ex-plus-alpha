/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Input/MsxArkanoidPad.c,v $
**
** $Revision: 1.5 $
**
** $Date: 2009-04-15 08:56:46 $
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
#include "MsxArkanoidPad.h"
#include "InputEvent.h"
#include "ArchInput.h"
#include "Board.h"
#include "SaveState.h"

#include <stdlib.h>

struct MsxArkanoidPad {
    MsxJoystickDevice joyDevice;
    int pos;
    UInt8 oldValue;
    UInt32 shiftReg;
};

static void saveState(MsxArkanoidPad* arkPad)
{
    SaveState* state = saveStateOpenForWrite("msxArkanoidPad");

    saveStateSet(state, "pos",      arkPad->pos);
    saveStateSet(state, "oldValue", arkPad->oldValue);
    saveStateSet(state, "shiftReg", arkPad->shiftReg);

    saveStateClose(state);
}

static void loadState(MsxArkanoidPad* arkPad)
{
    SaveState* state = saveStateOpenForRead("msxArkanoidPad");

    arkPad->pos      =        saveStateGet(state, "pos",      236);
    arkPad->oldValue = (UInt8)saveStateGet(state, "oldValue", 0);
    arkPad->shiftReg =        saveStateGet(state, "shiftReg", 0);

    saveStateClose(state);
}

static UInt8 read(MsxArkanoidPad* arkPad)
{
    return 0x3c | ((arkPad->shiftReg >> 8) & 1) | ((archMouseGetButtonState(0) << 1) ^ 2);
}

static void write(MsxArkanoidPad* arkPad, UInt8 value)
{
    UInt8 edge = value & (value ^ arkPad->oldValue);

    arkPad->oldValue = value;

    if (edge & 0x04) {
        int dx;
        int dy;

        archMouseGetState(&dx, &dy);

        arkPad->pos -= dx;

        /* measurements on the real device show a minimum of about 55, and a maximum of about 325 */
        /* the values below are aimed at user friendly mouse control */
        if (arkPad->pos < 152) arkPad->pos = 152;
        if (arkPad->pos > 309) arkPad->pos = 309;

        arkPad->shiftReg = arkPad->pos;
    }

    if (edge & 0x01) {
        /* in reality, due to a 556 IC (dual timer) on the device, a small delay is required in between shifts */
        arkPad->shiftReg = arkPad->shiftReg << 1 | (arkPad->shiftReg & 1);
    }
}

static void reset(MsxArkanoidPad* arkPad) {
    arkPad->pos      = 236;
    arkPad->oldValue = 0;
}

MsxJoystickDevice* msxArkanoidPadCreate()
{
    MsxArkanoidPad* arkPad = (MsxArkanoidPad*)calloc(1, sizeof(MsxArkanoidPad));
    arkPad->joyDevice.read       = read;
    arkPad->joyDevice.write      = write;
    arkPad->joyDevice.reset      = reset;
    arkPad->joyDevice.loadState  = loadState;
    arkPad->joyDevice.saveState  = saveState;

    reset(arkPad);

    return (MsxJoystickDevice*)arkPad;
}
