/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Memory/romMapperPlayBall.c,v $
**
** $Revision: 1.6 $
**
** $Date: 2008-05-17 04:51:04 $
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
#include "romMapperPlayBall.h"
#include "Board.h"
#include "MediaDb.h"
#include "SlotManager.h"
#include "DeviceManager.h"
#include "SamplePlayer.h"
#include "SaveState.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

typedef struct {
    SamplePlayer* samplePlayer;
    int deviceHandle;
    UInt8* romData;
    int slot;
    int sslot;
    int startPage;
    int size;
} RomMapperPlayBall;

#ifdef NO_EMBEDDED_SAMPLES

static void destroy(RomMapperPlayBall* rm)
{
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);
    free(rm->romData);
    free(rm);
}

int romMapperPlayBallCreate(const char* filename, UInt8* romData,
                          int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, NULL, NULL };
    RomMapperPlayBall* rm;

    rm = malloc(sizeof(RomMapperPlayBall));

    if (size > 0x8000) {
        size = 0x8000;
    }

    rm->deviceHandle = deviceManagerRegister(ROM_PLAYBALL, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, NULL, NULL, NULL, destroy, rm);

    rm->romData = malloc(0x8000);
    memset(rm->romData + size, 0xff, 0x8000 - size);
    memcpy(rm->romData, romData, size);
    rm->size = size;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    slotMapPage(rm->slot, rm->sslot, rm->startPage,     rm->romData + 0x0000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, rm->romData + 0x2000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 2, rm->romData + 0x4000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 3, rm->romData + 0x6000, 0, 0);

    return 1;
}

#else

#include "PlayballSamples.h"

static void destroy(RomMapperPlayBall* rm)
{
    samplePlayerDestroy(rm->samplePlayer);
    slotUnregister(rm->slot, rm->sslot, rm->startPage);
    deviceManagerUnregister(rm->deviceHandle);
    free(rm->romData);
    free(rm);
}

static UInt8 read(RomMapperPlayBall* rm, UInt16 address) 
{
    if (address == 0x7fff) {
        return samplePlayerIsIdle(rm->samplePlayer) ? 1 : 0;
    }

    return rm->romData[address];
}

static void write(RomMapperPlayBall* rm, UInt16 address, UInt8 value) 
{
    if (address == 0x7fff) {
        samplePlayerDoSync(rm->samplePlayer);
        if (samplePlayerIsIdle(rm->samplePlayer)) {
            switch (value) {
            case 0:  samplePlayerWrite(rm->samplePlayer, playball_0,  sizeof(playball_0)  / sizeof(playball_0[0]),  NULL, 0); break;
            case 1:  samplePlayerWrite(rm->samplePlayer, playball_1,  sizeof(playball_1)  / sizeof(playball_1[0]),  NULL, 0); break;
            case 2:  samplePlayerWrite(rm->samplePlayer, playball_2,  sizeof(playball_2)  / sizeof(playball_2[0]),  NULL, 0); break;
            case 3:  samplePlayerWrite(rm->samplePlayer, playball_3,  sizeof(playball_3)  / sizeof(playball_3[0]),  NULL, 0); break;
            case 4:  samplePlayerWrite(rm->samplePlayer, playball_4,  sizeof(playball_4)  / sizeof(playball_4[0]),  NULL, 0); break;
            case 5:  samplePlayerWrite(rm->samplePlayer, playball_0,  sizeof(playball_0)  / sizeof(playball_0[0]),  NULL, 0); break;
            case 6:  samplePlayerWrite(rm->samplePlayer, playball_6,  sizeof(playball_6)  / sizeof(playball_6[0]),  NULL, 0); break;
            case 7:  samplePlayerWrite(rm->samplePlayer, playball_7,  sizeof(playball_7)  / sizeof(playball_7[0]),  NULL, 0); break;
            case 8:  samplePlayerWrite(rm->samplePlayer, playball_8,  sizeof(playball_8)  / sizeof(playball_8[0]),  NULL, 0); break;
            case 9:  samplePlayerWrite(rm->samplePlayer, playball_9,  sizeof(playball_9)  / sizeof(playball_9[0]),  NULL, 0); break;
            case 10: samplePlayerWrite(rm->samplePlayer, playball_10, sizeof(playball_10) / sizeof(playball_10[0]), NULL, 0); break;
            case 11: samplePlayerWrite(rm->samplePlayer, playball_11, sizeof(playball_11) / sizeof(playball_11[0]), NULL, 0); break;
            case 12: samplePlayerWrite(rm->samplePlayer, playball_12, sizeof(playball_12) / sizeof(playball_12[0]), NULL, 0); break;
            case 13: samplePlayerWrite(rm->samplePlayer, playball_13, sizeof(playball_13) / sizeof(playball_13[0]), NULL, 0); break;
            case 14: samplePlayerWrite(rm->samplePlayer, playball_14, sizeof(playball_14) / sizeof(playball_14[0]), NULL, 0); break;
            default: break;
            }
        }
    }
}

int romMapperPlayBallCreate(const char* filename, UInt8* romData,
                          int size, int slot, int sslot, int startPage) 
{
    DeviceCallbacks callbacks = { destroy, NULL, NULL, NULL };
    RomMapperPlayBall* rm;

    rm = malloc(sizeof(RomMapperPlayBall));

    rm->samplePlayer = samplePlayerCreate(boardGetMixer(), MIXER_CHANNEL_PCM, 8, 11025);

    if (size > 0x8000) {
        size = 0x8000;
    }

    rm->deviceHandle = deviceManagerRegister(ROM_PLAYBALL, &callbacks, rm);
    slotRegister(slot, sslot, startPage, 4, read, read, write, destroy, rm);

    rm->romData = malloc(0x8000);
    memset(rm->romData + size, 0xff, 0x8000 - size);
    memcpy(rm->romData, romData, size);
    rm->size = size;
    rm->slot  = slot;
    rm->sslot = sslot;
    rm->startPage  = startPage;

    slotMapPage(rm->slot, rm->sslot, rm->startPage,     rm->romData + 0x0000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 1, rm->romData + 0x2000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 2, rm->romData + 0x4000, 1, 0);
    slotMapPage(rm->slot, rm->sslot, rm->startPage + 3, rm->romData + 0x6000, 0, 0);

    return 1;
}

#endif
