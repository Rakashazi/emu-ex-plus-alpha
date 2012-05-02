/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperKonamiSynth.c,v $
**
** $Revision: 1.8 $
**
** $Date: 2008-03-30 18:38:44 $
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
#include "romMapperKonamiSynth.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "DAC.h"
#include "Board.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


typedef struct {
    int deviceHandle;
    UInt8* romData;
	DAC* dac;
    int slot;
    int sslot;
    int startPage;
} RomMapperKonamiSynth;

static void destroy(RomMapperKonamiSynth* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);
    dacDestroy(rm->dac);

    free(rm->romData);
    free(rm);
}

static void write(RomMapperKonamiSynth* rm, UInt16 address, UInt8 value) 
{
	address += 0x4000;

	if ((address & 0xc010) == 0x4000) {
        dacWrite(rm->dac, DAC_CH_MONO, value);
	}
}

int romMapperKonamiSynthCreate(const char* filename, UInt8* romData,
                               int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, NULL, NULL };
    RomMapperKonamiSynth* rm;
    int i;

    if (size != 0x8000) {
        return 0;
    }

    rm = malloc(sizeof(RomMapperKonamiSynth));

    rm->deviceHandle = deviceManagerRegister(ROM_KONAMISYNTH, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, NULL, NULL, write, destroy, rm);

    rm->romData = malloc(size);
    memcpy(rm->romData, romData, size);
    rm->dac   = dacCreate(boardGetMixer(), DAC_MONO);
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    for (i = 0; i < 4; i++) {   
        slotMapPage(rm->slot, rm->sslot, rm->startPage + i, rm->romData + i * 0x2000, 1, 0);
    }

    return 1;
}

