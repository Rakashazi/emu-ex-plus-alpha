/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Input/CoinDevice.c,v $
**
** $Revision: 1.2 $
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
#include "CoinDevice.h"
#include <stdlib.h>
#include "InputEvent.h"
#include "DeviceManager.h"
#include "Board.h"

struct CoinDevice {
    int deviceHandle;
    UInt32 time;
};
 
static void destroy(CoinDevice* coinDev) 
{
    deviceManagerUnregister(coinDev->deviceHandle);
    free(coinDev);
}

UInt8 coinDeviceRead(CoinDevice* coinDev) 
{
    if(coinDev->time == 0 && inputEventGetState(EC_C)) {
        coinDev->time = boardSystemTime();
    }

    if (coinDev->time != 0) {
        if ((boardSystemTime() - coinDev->time) < boardFrequency() * 2 / 10) {
            return 0;
        }
        coinDev->time = 0;
    }
    return 1;
}

CoinDevice* coinDeviceCreate()
{
    DeviceCallbacks callbacks = { destroy, NULL, NULL, NULL };
    CoinDevice* coinDev = (CoinDevice*)calloc(1, sizeof(CoinDevice));
    coinDev->time = 0;

    coinDev->deviceHandle = deviceManagerRegister(ROM_UNKNOWN, &callbacks, coinDev);

    return coinDev;
}
